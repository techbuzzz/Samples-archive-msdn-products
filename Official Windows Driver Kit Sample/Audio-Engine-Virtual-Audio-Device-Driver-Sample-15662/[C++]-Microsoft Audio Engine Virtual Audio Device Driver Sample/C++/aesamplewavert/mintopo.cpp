/*++

Copyright (c) 1997-2011  Microsoft Corporation All Rights Reserved

Module Name:

    mintopo.cpp

Abstract:

    Implementation of topology miniport.

--*/

#pragma warning (disable : 4127)

#include <msvad.h>
#include <common.h>
#include "simple.h"
#include "minwavert.h"
#include "mintopo.h"
#include "toptable.h"
#include "micintoptable.h"


/*********************************************************************
* Topology/Wave bridge connection for render                         *
*                                                                    *
*              +------+    +------+                                  *
*              | Wave |    | Topo |                                  *
*              |      |    |      |                                  *
* System   --->|0     |    |      |                                  *
* Offload  --->|1    3|--->|0    1|<--- Line Out                     *
* Loopback <---|2     |    |      |                                  *
*              |      |    |      |                                  *
*              +------+    +------+                                  *
*********************************************************************/
PHYSICALCONNECTIONTABLE TopologyPhysicalConnections =
{
    KSPIN_TOPO_WAVEOUT_SOURCE,  // TopologyIn
    (ULONG)-1,     // TopologyOut
    (ULONG)-1,  // WaveIn
    KSPIN_WAVE_RENDER_SOURCE    // WaveOut
};

#ifdef _SUPPORT_INDEPENDENT_MICIN
/*********************************************************************
* Topology/Wave bridge connection for mic in                         *
*                                                                    *
*              +------+    +------+                                  *
*              | Topo |    | Wave |                                  *
*              |      |    |      |                                  *
*  Mic in  --->|0    1|===>|0    1|---> Capture Host Pin             *
*              |      |    |      |                                  *
*              +------+    +------+                                  *
*********************************************************************/
PHYSICALCONNECTIONTABLE MicInTopologyPhysicalConnections =
{
    (ULONG)-1,          // ulTopologyIn
    KSPIN_TOPO_BRIDGE,  // ulTopologyOut
    KSPIN_WAVE_BRIDGE,  // ulWaveIn
    (ULONG)-1           // ulWaveOut
};
#endif

#pragma code_seg("PAGE")

//=============================================================================
NTSTATUS
CreateMiniportTopologyMSVAD
( 
    OUT PUNKNOWN *              Unknown,
    IN  REFCLSID,
    IN  PUNKNOWN,
    _When_((PoolType & NonPagedPoolMustSucceed) != 0,
       __drv_reportError("Must succeed pool allocations are forbidden. "
			 "Allocation failures cause a system crash"))
    IN  POOL_TYPE               PoolType,
    IN  eDeviceType             deviceType,
    IN  PCFILTER_DESCRIPTOR     *pDesc, 
    IN  PUNKNOWN UnknownAdapter
)
/*++

Routine Description:

    Creates a new topology miniport.

Arguments:

  Unknown - 

  RefclsId -

  UnknownOuter -

  PoolType - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(UnknownAdapter);
    ASSERT(Unknown);

    CMiniportTopology *obj = new (PoolType, MINWAVERT_POOLTAG) CMiniportTopology(deviceType, pDesc);
    if (NULL == obj)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    obj->AddRef();
    *Unknown = reinterpret_cast<IUnknown*>(obj);
    return STATUS_SUCCESS;
} // CreateMiniportTopologyMSVAD

//=============================================================================
CMiniportTopology::~CMiniportTopology
(
    void
)
/*++

Routine Description:

  Topology miniport destructor

Arguments:

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportTopology::~CMiniportTopology]"));
} // ~CMiniportTopology

//=============================================================================
NTSTATUS
CMiniportTopology::DataRangeIntersection
( 
    _In_        ULONG                   PinId,
    _In_        PKSDATARANGE            ClientDataRange,
    _In_        PKSDATARANGE            MyDataRange,
    _In_        ULONG                   OutputBufferLength,
    _Out_writes_bytes_to_opt_(OutputBufferLength, *ResultantFormatLength)
                PVOID                   ResultantFormat,
    _Out_       PULONG                  ResultantFormatLength 
)
/*++

Routine Description:

  The DataRangeIntersection function determines the highest quality 
  intersection of two data ranges.

Arguments:

  PinId - Pin for which data intersection is being determined. 

  ClientDataRange - Pointer to KSDATARANGE structure which contains the data range 
                    submitted by client in the data range intersection property 
                    request. 

  MyDataRange - Pin's data range to be compared with client's data range. 

  OutputBufferLength - Size of the buffer pointed to by the resultant format 
                       parameter. 

  ResultantFormat - Pointer to value where the resultant format should be 
                    returned. 

  ResultantFormatLength - Actual length of the resultant format that is placed 
                          at ResultantFormat. This should be less than or equal 
                          to OutputBufferLength. 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    return 
        CMiniportTopologyMSVAD::DataRangeIntersection
        (
            PinId,
            ClientDataRange,
            MyDataRange,
            OutputBufferLength,
            ResultantFormat,
            ResultantFormatLength
        );
} // DataRangeIntersection

//=============================================================================
STDMETHODIMP
CMiniportTopology::GetDescription
( 
    _Out_ PPCFILTER_DESCRIPTOR *  OutFilterDescriptor 
)
/*++

Routine Description:

  The GetDescription function gets a pointer to a filter description. 
  It provides a location to deposit a pointer in miniport's description 
  structure. This is the placeholder for the FromNode or ToNode fields in 
  connections which describe connections to the filter's pins. 

Arguments:

  OutFilterDescriptor - Pointer to the filter description. 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(OutFilterDescriptor);

    *OutFilterDescriptor = &m_FilterDesc;

    return STATUS_SUCCESS;
        
} // GetDescription

//=============================================================================
STDMETHODIMP
CMiniportTopology::Init
( 
    _In_ PUNKNOWN                 UnknownAdapter,
    _In_ PRESOURCELIST            ResourceList,
    _In_ PPORTTOPOLOGY            Port_ 
)
/*++

Routine Description:

  The Init function initializes the miniport. Callers of this function 
  should run at IRQL PASSIVE_LEVEL

Arguments:

  UnknownAdapter - A pointer to the Iuknown interface of the adapter object. 

  ResourceList - Pointer to the resource list to be supplied to the miniport 
                 during initialization. The port driver is free to examine the 
                 contents of the ResourceList. The port driver will not be 
                 modify the ResourceList contents. 

  Port - Pointer to the topology port object that is linked with this miniport. 

Return Value:

  NT status code.

--*/
{
    UNREFERENCED_PARAMETER(ResourceList);

    PAGED_CODE();

    ASSERT(UnknownAdapter);
    ASSERT(Port_);

    DPF_ENTER(("[CMiniportTopology::Init]"));

    NTSTATUS                    ntStatus;

    ntStatus = 
        CMiniportTopologyMSVAD::Init
        (
            UnknownAdapter,
            Port_
        );

    return ntStatus;
} // Init

//=============================================================================
STDMETHODIMP
CMiniportTopology::NonDelegatingQueryInterface
( 
    _In_ REFIID                  Interface,
    _COM_Outptr_ PVOID      * Object 
)
/*++

Routine Description:

  QueryInterface for MiniportTopology

Arguments:

  Interface - GUID of the interface

  Object - interface object to be returned.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniport))
    {
        *Object = PVOID(PMINIPORT(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportTopology))
    {
        *Object = PVOID(PMINIPORTTOPOLOGY(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        // We reference the interface for the caller.
        PUNKNOWN(*Object)->AddRef();
        return(STATUS_SUCCESS);
    }

    return(STATUS_INVALID_PARAMETER);
} // NonDelegatingQueryInterface

//=============================================================================
NTSTATUS
CMiniportTopology::PropertyHandlerJackDescription
( 
    IN PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Handles ( KSPROPSETID_Jack, KSPROPERTY_JACK_DESCRIPTION )

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandlerJackDescription]"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    ULONG    nPinId = (ULONG)-1;

    if (PropertyRequest->InstanceSize >= sizeof(ULONG))
    {
        nPinId = *(PULONG(PropertyRequest->Instance));

        if ((nPinId < ARRAYSIZE(JackDescriptions)) && (JackDescriptions[nPinId] != NULL))
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
            {
                ntStatus = 
                    PropertyHandler_BasicSupport
                    (
                        PropertyRequest,
                        KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET,
                        VT_ILLEGAL
                    );
            }
            else
            {
                ULONG cbNeeded = sizeof(KSMULTIPLE_ITEM) + sizeof(KSJACK_DESCRIPTION);

                if (PropertyRequest->ValueSize == 0)
                {
                    PropertyRequest->ValueSize = cbNeeded;
                    ntStatus = STATUS_BUFFER_OVERFLOW;
                }
                else if (PropertyRequest->ValueSize < cbNeeded)
                {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
                    {
                        PKSMULTIPLE_ITEM pMI = (PKSMULTIPLE_ITEM)PropertyRequest->Value;
                        PKSJACK_DESCRIPTION pDesc = (PKSJACK_DESCRIPTION)(pMI+1);

                        pMI->Size = cbNeeded;
                        pMI->Count = 1;

                        RtlCopyMemory(pDesc, JackDescriptions[nPinId], sizeof(KSJACK_DESCRIPTION));
                        ntStatus = STATUS_SUCCESS;
                    }
                }
            }
        }
    }

    return ntStatus;
}

//=============================================================================
NTSTATUS
CMiniportTopology::PropertyHandlerJackDescriptionMicIn
( 
    IN PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Handles ( KSPROPSETID_Jack, KSPROPERTY_JACK_DESCRIPTION )

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandlerJackDescriptionMicIn]"));

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    ULONG    nPinId = (ULONG)-1;

    if (PropertyRequest->InstanceSize >= sizeof(ULONG))
    {
        nPinId = *(PULONG(PropertyRequest->Instance));

        if ((nPinId < ARRAYSIZE(MicJackDescriptions)) && (MicJackDescriptions[nPinId] != NULL))
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
            {
                ntStatus = 
                    PropertyHandler_BasicSupport
                    (
                        PropertyRequest,
                        KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_GET,
                        VT_ILLEGAL
                    );
            }
            else
            {
                ULONG cbNeeded = sizeof(KSMULTIPLE_ITEM) + sizeof(KSJACK_DESCRIPTION);

                if (PropertyRequest->ValueSize == 0)
                {
                    PropertyRequest->ValueSize = cbNeeded;
                    ntStatus = STATUS_BUFFER_OVERFLOW;
                }
                else if (PropertyRequest->ValueSize < cbNeeded)
                {
                    ntStatus = STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
                    {
                        PKSMULTIPLE_ITEM pMI = (PKSMULTIPLE_ITEM)PropertyRequest->Value;
                        PKSJACK_DESCRIPTION pDesc = (PKSJACK_DESCRIPTION)(pMI+1);

                        pMI->Size = cbNeeded;
                        pMI->Count = 1;

                        RtlCopyMemory(pDesc, MicJackDescriptions[nPinId], sizeof(KSJACK_DESCRIPTION));
                        ntStatus = STATUS_SUCCESS;
                    }
                }
            }
        }
    }

    return ntStatus;
}

//=============================================================================
NTSTATUS
PropertyHandler_TopoFilter
( 
    IN PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Redirects property request to miniport object

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandler_TopoFilter]"));

    // PropertryRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    NTSTATUS            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    PCMiniportTopology  pMiniport = (PCMiniportTopology)PropertyRequest->MajorTarget;

    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Jack) &&
        (PropertyRequest->PropertyItem->Id == KSPROPERTY_JACK_DESCRIPTION))
    {
        ntStatus = 
            pMiniport->PropertyHandlerJackDescription
            (
                PropertyRequest
            );
    }

    return ntStatus;
} // PropertyHandler_TopoFilter

//=============================================================================
NTSTATUS
PropertyHandler_MicInTopoFilter
( 
    IN PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Redirects property request to miniport object

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandler_MicInTopoFilter]"));

    // PropertryRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    NTSTATUS            ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    PCMiniportTopology  pMiniport = (PCMiniportTopology)PropertyRequest->MajorTarget;

    if (IsEqualGUIDAligned(*PropertyRequest->PropertyItem->Set, KSPROPSETID_Jack) &&
        (PropertyRequest->PropertyItem->Id == KSPROPERTY_JACK_DESCRIPTION))
    {
        ntStatus = 
            pMiniport->PropertyHandlerJackDescriptionMicIn
            (
                PropertyRequest
            );
    }

    return ntStatus;
} // PropertyHandler_MicInTopoFilter

//=============================================================================
NTSTATUS
PropertyHandler_Topology
( 
    IN PPCPROPERTY_REQUEST      PropertyRequest 
)
/*++

Routine Description:

  Redirects property request to miniport object

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandler_Topology]"));

    // PropertryRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    return 
        ((PCMiniportTopology)
        (PropertyRequest->MajorTarget))->PropertyHandlerGeneric
                    (
                        PropertyRequest
                    );
} // PropertyHandler_Topology

#pragma code_seg()

