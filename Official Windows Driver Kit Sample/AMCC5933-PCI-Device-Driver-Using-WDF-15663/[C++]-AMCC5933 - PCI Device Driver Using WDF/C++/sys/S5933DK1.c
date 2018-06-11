/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    S5933DK1.c - Driver for the S5933DK1 ISA adapter.

Abstract:

Environment:

    Kernel mode

--*/

#include "common.h"
#include "public.h"
#include "S5933DK1.h"

//
// The trace message header file must be included in a source file
// before any WPP macro calls and after defining a WPP_CONTROL_GUIDS
// macro. During the compilation, WPP scans the source files for
// TraceEvents() calls and builds a .tmh file which stores a unique
// data GUID for each message, the text resource string for each message,
// and the data types of the variables passed in for each message.
// This file is automatically generated and used during post-processing.
//
#include "S5933DK1.tmh"


//
// Make sure the initialization code is removed from memory after use.
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AmccIsaEvtDeviceAdd)
#pragma alloc_text(PAGE, AmccIsaEvtDevicePrepareHardware)
#pragma alloc_text(PAGE, AmccIsaEvtDeviceReleaseHardware)
#pragma alloc_text(PAGE, AmccIsaFreeDeviceResources)
#pragma alloc_text(PAGE, AmccIsaEvtIoDeviceControl)
#pragma alloc_text(PAGE, AmccIsaWorker)
#pragma alloc_text(PAGE, AmccIsaResetDevice)
#endif



NTSTATUS
AmccIsaEvtDeviceAdd(
    _In_    WDFDRIVER        Driver,
    _Inout_ PWDFDEVICE_INIT  DeviceInit
    )
/*++

Routine Description:

Arguments:

Return Value:

    NTSTATUS

--*/
{
    WDF_PNPPOWER_EVENT_CALLBACKS    pnpPowerCallbacks;
    NTSTATUS                   status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES       fdoAttributes;
    WDFDEVICE                   device;
    WDF_IO_QUEUE_CONFIG         ioQueueConfig;
    PRDK_DEVICE_EXTENSION           devExt;
    WDFQUEUE                    hQueue;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, AMCC_TRACE_INIT,
                        "AmccIsaEvtDeviceAdd: 0x%p", Driver);

    //
    // Zero out the pnpPowerCallbacks structure.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    //
    // Set Callbacks for any of the functions we are interested in.
    // If no callback is set, Framework will take the default action
    // by itself.
    //
    pnpPowerCallbacks.EvtDevicePrepareHardware = AmccIsaEvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = AmccIsaEvtDeviceReleaseHardware;

    //
    // Register the Callbacks.
    //
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Set various attributes & characteristics for this device
    //
    WdfDeviceInitSetIoType( DeviceInit, WdfDeviceIoDirect );

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fdoAttributes, RDK_DEVICE_EXTENSION);

    status = WdfDeviceCreate( &DeviceInit, &fdoAttributes, &device );

    if ( !NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, AMCC_TRACE_INIT,
                            "WdfDeviceInitialize failed 0x%X", status);
        return status;
    }

    //
    // Device Initialization is complete.
    // Get the Device Extension and initialize it.
    //

    devExt = AmccIsaGetDevExt(device);

    devExt->Device = device;

    TraceEvents(TRACE_LEVEL_INFORMATION, AMCC_TRACE_INIT,
                "PDO 0x%p, FDO 0x%p, DevExt 0x%p",
                WdfDeviceWdmGetPhysicalDevice(device),
                WdfDeviceWdmGetDeviceObject( device ), devExt);

    //
    // Register I/O callbacks.
    //
    // Create a sequential queue so that requests are dispatched to the
    // driver one at a time. Until the driver completes a dispatched request another
    // request will be not presented to the driver.
    // A default queue gets all the requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE( &ioQueueConfig,
                              WdfIoQueueDispatchSequential);

    ioQueueConfig.EvtIoDeviceControl = AmccIsaEvtIoDeviceControl;
    
    //
    // By default, Static Driver Verifier (SDV) displays a warning if it 
    // doesn't find the EvtIoStop callback on a power-managed queue. 
    // The 'assume' below causes SDV to suppress this warning. If the driver 
    // has not explicitly set PowerManaged to WdfFalse, the framework creates
    // power-managed queues when the device is not a filter driver.  Normally 
    // the EvtIoStop is required for power-managed queues, but for this driver
    // it is not needed b/c the driver doesn't hold on to the requests for 
    // long time or forward them to other drivers. 
    // If the EvtIoStop callback is not implemented, the framework waits for
    // all driver-owned requests to be done before moving in the Dx/sleep 
    // states or before removing the device, which is the correct behavior 
    // for this type of driver. If the requests were taking an indeterminate
    // amount of time to complete, or if the driver forwarded the requests
    // to a lower driver/another stack, the queue should have an 
    // EvtIoStop/EvtIoResume.
    //
    __analysis_assume(ioQueueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate( device,
                               &ioQueueConfig,
                               WDF_NO_OBJECT_ATTRIBUTES,
                               &hQueue );
    __analysis_assume(ioQueueConfig.EvtIoStop == 0);
    
    if (!NT_SUCCESS (status)) {
        TraceEvents(TRACE_LEVEL_ERROR, AMCC_TRACE_INIT,
                            "WdfIoQueueCreate failed 0x%X\n", status);
        return status;
    }

    //
    // Register an interface so that application can find and talk to us.
    // NOTE: See the note in Public.h concerning this GUID value.
    //
    status = WdfDeviceCreateDeviceInterface( device,
                                             (LPGUID) &GUID_DEVINTERFACE_AMCC_ISA,
                                             NULL );

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, AMCC_TRACE_INIT,
                    "<-- AMCCAddDevice: WdfDeviceCreateDeviceInterface failed 0x%X", status);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, AMCC_TRACE_INIT,
                "<--exit 0x%X", status);

    return status;
}

NTSTATUS
AmccIsaEvtDevicePrepareHardware(
    _In_ WDFDEVICE       Device,
    _In_ WDFCMRESLIST   ResourcesRaw,
    _In_ WDFCMRESLIST   ResourcesTranslated
    )
/*++

Routine Description:

    EvtDevicePrepareHardware event callback performs operations that are necessary
    to make the driver's device operational. The framework calls the driver's
    EvtDevicePrepareHardware callback when the PnP manager sends an
    IRP_MN_START_DEVICE request to the driver stack.

Arguments:

    Device - Handle to a framework device object.

    Resources - Handle to a collection of framework resource objects.
                This collection identifies the raw (bus-relative) hardware
                resources that have been assigned to the device.

    ResourcesTranslated - Handle to a collection of framework resource objects.
                This collection identifies the translated (system-physical)
                hardware resources that have been assigned to the device.
                The resources appear from the CPU's point of view.
                Use this list of resources to map I/O space and
                device-accessible memory into virtual address space

Return Value:

    WDF status code.

    Let us not worry about cleaning up the resources here if we fail start,
    because the PNP manager will send a remove-request and we will free all
    the allocated resources in AmccIsaFreeDeviceResources.
--*/
{
    ULONG               i;
    PRDK_DEVICE_EXTENSION   devExt         = NULL;
    BOOLEAN             foundPort300   = FALSE;
    BOOLEAN             foundPort718   = FALSE;
    PHYSICAL_ADDRESS    port300BasePA  = {0};
    PHYSICAL_ADDRESS    port718BasePA  = {0};
    ULONG               port300Count   = 0;
    ULONG               port718Count   = 0;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR  desc;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( ResourcesRaw );


    devExt = AmccIsaGetDevExt(Device);

    //
    // Parse the resource list and save the resource information.
    //
    for (i=0; i < WdfCmResourceListGetCount(ResourcesTranslated); i++) {

        desc = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

        if(!desc) {

            TraceEvents(TRACE_LEVEL_ERROR, AMCC_TRACE_INIT,
                                "WdfResourceCmGetDescriptor failed");

            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        switch (desc->Type) {

        case CmResourceTypePort:

            TraceEvents(TRACE_LEVEL_INFORMATION, AMCC_TRACE_INIT,
                                "Found Port Resource [%X-%X]",
                                desc->u.Port.Start.LowPart,
                                desc->u.Port.Start.LowPart +
                                desc->u.Port.Length );

            //
            // First port resource should be for ports 300-340
            //
            if (foundPort300 == FALSE) {

                port300BasePA = desc->u.Port.Start;
                port300Count  = desc->u.Port.Length;
                devExt->Port300Mapped =
                    ((desc->Flags & CM_RESOURCE_PORT_IO) == CM_RESOURCE_PORT_IO) ?
                        TRUE : FALSE;

                foundPort300 = TRUE;
                break;
            }

            //
            // Second port resource should be for ports 718-720
            //
            if (foundPort718 == FALSE) {

                port718BasePA = desc->u.Port.Start;
                port718Count  = desc->u.Port.Length;
                devExt->Port718Mapped =
                    ((desc->Flags & CM_RESOURCE_PORT_IO) == CM_RESOURCE_PORT_IO) ?
                        TRUE : FALSE;

                foundPort718 = TRUE;
                break;
            }

            ASSERT(!"unexpected port resource");
            break;

            default:
                TraceEvents(TRACE_LEVEL_WARNING, AMCC_TRACE_INIT,
                                    "Used resource type %d", desc->Type);
                break;
        }
    }

    if (!foundPort300 && !foundPort718) {

        TraceEvents(TRACE_LEVEL_ERROR, AMCC_TRACE_INIT,
                            "Missing expected hardware resources");

        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    //
    // Map in the 300-340 port IO Space resource.
    //
    if (devExt->Port300Mapped) {

        devExt->Port300Base =
            (PUCHAR) MmMapIoSpace( port300BasePA, port300Count, MmNonCached );

        if (!devExt->Port300Base) {

            TraceEvents(TRACE_LEVEL_ERROR, AMCC_TRACE_INIT,
                                "Unable to map port range [%X-%X]",
                                (ULONG) (ULONG_PTR) devExt->Port300Base,
                                (ULONG) devExt->Port300Count );

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        devExt->Port300Count = port300Count;

    } else {

        devExt->Port300Base  = (PUCHAR)(ULONG_PTR) port300BasePA.QuadPart;
        devExt->Port300Count = port300Count;
    }

    //
    // Map in the 718-720 port IO Space resource.
    //
    if (devExt->Port718Mapped) {

        devExt->Port718Base =
            (PUCHAR) MmMapIoSpace( port718BasePA, port718Count, MmNonCached );

        if (!devExt->Port718Base) {

            TraceEvents(TRACE_LEVEL_ERROR, AMCC_TRACE_INIT,
                                "Unable to map port range [%X-%X]",
                                (ULONG) (ULONG_PTR) devExt->Port718Base,
                                (ULONG) devExt->Port718Count );

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        devExt->Port718Count = port718Count;

    } else {

        devExt->Port718Base  = (PUCHAR)(ULONG_PTR) port718BasePA.QuadPart;
        devExt->Port718Count = port718Count;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
AmccIsaEvtDeviceReleaseHardware(
    _In_ WDFDEVICE       Device,
    _In_ WDFCMRESLIST   ResourcesTranslated
    )
/*++

Routine Description:

    EvtDeviceReleaseHardware is called by the framework in response to
    IRP_MN_STOP_DEVICE and IRP_MN_REMOVE_DEVICE from the PnP manager.

Arguments:

    Device - Handle to a framework device object.
    ResourcesTranslated - WDF Collection of resources.

Return Value:

    NTSTATUS

--*/
{
    PRDK_DEVICE_EXTENSION   devExt = NULL;

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PAGED_CODE();

    devExt = AmccIsaGetDevExt(Device);

    //
    // Unmap the IO resource
    //
    if (devExt->Port300Base) {

        MmUnmapIoSpace( devExt->Port300Base, devExt->Port300Count );

        devExt->Port300Base = NULL;
    }

    if (devExt->Port718Base) {

        MmUnmapIoSpace( devExt->Port718Base, devExt->Port718Count );

        devExt->Port718Base = NULL;
    }

    return STATUS_SUCCESS;
}

VOID
AmccIsaEvtIoDeviceControl(
    _In_ WDFQUEUE      Queue,
    _In_ WDFREQUEST    Request,
    _In_ size_t         OutputBufferLength,
    _In_ size_t         InputBufferLength,
    _In_ ULONG         IoControlCode
    )
/*++
Routine Description:

    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    OutputBufferLength - length of the request's output buffer,
                        if an output buffer is available.
    InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.
Return Value:

    VOID

--*/
{
    PRDK_DEVICE_EXTENSION   devExt;
    size_t                  length = 0;
    NTSTATUS                status = STATUS_SUCCESS;
    PVOID                   buffer = NULL;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( InputBufferLength  );
    UNREFERENCED_PARAMETER( OutputBufferLength  );

    //
    // Get the device extension.
    //
    devExt = AmccIsaGetDevExt(WdfIoQueueGetDevice( Queue ));

    //
    // Handle this request's specific code.
    //
    switch (IoControlCode) {

    case IOCTL_GET_VERSION:          // code == 0x800
    {

        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(ULONG), &buffer, &length);
        if( !NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, AMCC_TRACE_IO,
                            "WdfRequestRetrieveInputBuffer failed 0x%x\n", status);
            break;
        }


        *(PULONG) buffer = 0x0004000A;

        status = STATUS_SUCCESS;
        length = sizeof(ULONG);
        break;
    }

    case IOCTL_RESET:               // code == 0x801
    {
        status = AmccIsaResetDevice( devExt );
        break;
    }

    case IOCTL_READ_DMA:        // code == 0x804
    case IOCTL_WRITE_DMA:       // code == 0x805
    {
        BOOLEAN  isRead = (IoControlCode == IOCTL_READ_DMA) ? TRUE:FALSE;

        status = AmccIsaReadWriteDma( devExt, Request, isRead );
        break;
    }

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    if (status != STATUS_PENDING) {

        WdfRequestCompleteWithInformation(Request, status, length);
    }
}



NTSTATUS
AmccIsaResetDevice(
    _In_ PRDK_DEVICE_EXTENSION  DevExt
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    UNREFERENCED_PARAMETER(DevExt);

    PAGED_CODE();

    //
    // Reset the device by writing "1" bits to the reset flags in the general
    // control/status register. It's not necessary to reset any of these bits
    // to "0" afterwards. Other bits in GCSTS are R/O and unaffected by this
    //
    #pragma prefast(suppress: 28138, "Legacy ISA device so we use hardcoded values")
    WRITE_PORT_ULONG(AGCSTS, GCSTS_RESET);

    //
    // Disable all add-on interrupts
    //
    #pragma prefast(suppress: 28138, "Legacy ISA device so we use hardcoded values")
    WRITE_PORT_ULONG(AINT, INT_INTERRUPT_MASK);

    return STATUS_SUCCESS;
}


NTSTATUS
AmccIsaReadWriteDma(
    _In_ PRDK_DEVICE_EXTENSION  DevExt,
    _In_ WDFREQUEST         Request,
    _In_ BOOLEAN            isRead
    )
/*++

Routine Description:

Arguments:

Return Value:

   NTSTATUS

   The request will not be completed if STATUS_PENDING is returned.

--*/
{
    NTSTATUS               status;
    size_t                  nbytes;
    PWORKITEM              item;
    WDF_OBJECT_ATTRIBUTES  attributes;
    WDF_WORKITEM_CONFIG    workitemConfig;
    WORKITEM_CALLBACK      callback;
    PVOID               buffer;

    //
    // Framework will automatically complete zero length requests, so let us make sure
    // that buffer is atleast 4-bytes long.
    //
    status = WdfRequestRetrieveOutputBuffer(Request, 4, &buffer, &nbytes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Byte count must be an integral of dwords
    //
    if (nbytes & 3) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Create a WDFWORKITEM object on which the request
    // will be executed.
    //
    ASSERT(DevExt->Worker == NULL);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, WORKITEM);
    attributes.ParentObject = DevExt->Device;

    WDF_WORKITEM_CONFIG_INIT(&workitemConfig, AmccIsaWorker);

    status = WdfWorkItemCreate( &workitemConfig,
                                &attributes,
                                &DevExt->Worker );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    item = AmccIsaGetWorkItem( DevExt->Worker );

    //
    // Set the parameters to this WorkItem
    //
    item->u.DmaWork.Buffer = buffer;

    callback = (isRead) ? (WORKITEM_CALLBACK) ReadDmaCallback :
                          (WORKITEM_CALLBACK) WriteDmaCallback;

    item->u.DmaWork.nbytes   = nbytes;
    item->u.DmaWork.numxfer  = 0;
    item->Request          = Request;
    item->Callback         = callback;
    item->DevExt           = DevExt;

    //
    // Execute this work item.
    //
    WdfWorkItemEnqueue( DevExt->Worker );

    //
    // Request will be completed in the work thread.
    //
    return STATUS_PENDING;

}


VOID
AmccIsaWorker(
    _In_  WDFWORKITEM WorkItem
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PWORKITEM  item;

    PAGED_CODE();

    item = AmccIsaGetWorkItem( WorkItem );

    //
    // Call callback until it returns FALSE
    // (e.g TRUE == more processing needed).
    //
    do {} while ((*item->Callback)( item ));

    //
    // Delete this thread
    //
    item->DevExt->Worker = NULL;

    WdfObjectDelete( WorkItem );
}



BOOLEAN
ReadDmaCallback(
    _In_ PWORKITEM  Item
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG  gcsts;
    ULONG  numxfer;
    ULONG  loops = 0;
    LARGE_INTEGER delay;

    //
    // The ISA side of this hardware evaluation board is a non-DMA,
    // non-interrupt PIO device.
    //
    // This type of hardware is not recommended because of the impact
    // on system performance since the I/O ready status must be
    // polled, and the transfer done by the processor when ready.
    //
    // Doing this kind of work within a kernel mode thread
    // (from the WDFWORKITEM) has an impact on the systems performance.
    //
    // To mitigate this, the following is done:
    //
    // - Ensure that when copying a large amount of data in the main transfer
    //   loop we check for I/O request cancelation are reasonable intervals.
    //
    // - When polling for ready data, yield the processor during
    //   intervals when data is not readily available, and only
    //   transfer data in the main loop when a previous data
    //   item was available immediately.
    //

    while (Item->u.DmaWork.nbytes) {

        //
        // This is the body of the main data transfer loop. We
        // will only retrieve data in this loop when it is immediately
        // available as indicated by the status register.
        //

        //
        // See if there is any data available to be read.
        // read general control/status register
        //
        #pragma prefast(suppress: 28138, "Legacy ISA device so we use hardcoded values")
        gcsts = READ_PORT_ULONG( AGCSTS );

        if (gcsts & GCSTS_RFIFO_EMPTY) {
            break; // no data to read.
        }

        switch (Item->u.DmaWork.nbytes) {

            case 1:
            #pragma prefast(suppress: 28138, "Legacy ISA device so we use hardcoded values")   
            *Item->u.DmaWork.Buffer = READ_PORT_UCHAR((PUCHAR) AFIFO);
            numxfer = 1;
            break;

            case 2:
            #pragma prefast(suppress: 28138, "Legacy ISA device so we use hardcoded values") 
            ((PUSHORT) (Item->u.DmaWork.Buffer))[0] =
                READ_PORT_USHORT((PUSHORT) AFIFO);
            numxfer = 2;
            break;

            case 3:
            #pragma prefast(suppress: 28138, "Legacy ISA device so we use hardcoded values")   
            READ_PORT_BUFFER_UCHAR((PUCHAR) AFIFO, Item->u.DmaWork.Buffer, 3);
            numxfer = 3;
            break;

            default:
            #pragma prefast(suppress: 28138, "Legacy ISA device so we use hardcoded values")    
            ((PULONG) (Item->u.DmaWork.Buffer))[0] =
                READ_PORT_ULONG(AFIFO);
            numxfer = 4;
            break;

        }

        Item->u.DmaWork.Buffer  += numxfer;
        Item->u.DmaWork.numxfer += numxfer;
        Item->u.DmaWork.nbytes  -= numxfer;

        //
        // If we are doing a large block transfer, we want to
        // check for I/O cancellation at reasonable intervals.
        //
        // This value is determined by the hardware itself. Currently,
        // the count of 512 is arbitrary.
        //
        if (loops++ > 512) {
            if (WdfRequestIsCanceled(Item->Request)) {
               WdfRequestComplete( Item->Request, STATUS_CANCELLED);
               return FALSE;
            }
        }
    }

    if (Item->u.DmaWork.nbytes == 0) {

        TraceEvents(TRACE_LEVEL_INFORMATION, AMCC_TRACE_IO,
                            "numxfer %I64d", Item->u.DmaWork.numxfer);

        WdfRequestCompleteWithInformation( Item->Request,
                                           STATUS_SUCCESS,
                                           Item->u.DmaWork.numxfer );

        return FALSE;   // Operation completed.
    }

    //
    // Data was not ready immediately, so we will check for
    // request cancellation, and if the request is not cancelled,
    // yield the processor before attempting to continue with
    // the transfer loop.
    //

    if (WdfRequestIsCanceled(Item->Request)) {
       //
       // Cancel the polling and exit.
       //
       WdfRequestComplete( Item->Request, STATUS_CANCELLED);
       return FALSE;
    }

    //
    // Yield the processor so we do not take up to much time.
    //
    // This gives other applications and threads a chance to
    // respond. In this case, the application thread feeding
    // data to the other side of this card may be a lower
    // priority than the system workitem, and without this
    // delay we can starve the thread not giving it a chance
    // to setup the transfer and provide the data.
    //
    // Note: Do not use KeStallExecutionProcessor since this is
    //       a spinloop, and will not yield the current thread.
    //

    delay.QuadPart = WDF_REL_TIMEOUT_IN_MS(10);
    KeDelayExecutionThread(KernelMode, FALSE, &delay);

    return TRUE;        // Operation not complete yet.
}


BOOLEAN
WriteDmaCallback(
    _In_ PWORKITEM  Item
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    ULONG    gcsts;
    ULONG    numxfer;

    while (Item->u.DmaWork.nbytes) {

        // See if there's room to write more data. Note that the S5933 always
        // moves data in 32-bit chunks through the FIFO, so finding the FIFO
        // non-full means there's room for at least 4 bytes.

        //
        // read general control/status register
        //
        #pragma prefast(suppress: 28138, "Legacy ISA device so we use hardcoded values")    
        gcsts = READ_PORT_ULONG(AGCSTS);

        if (gcsts & GCSTS_WFIFO_FULL) {
            break; // no room in FIFO for more data
        }

        switch (Item->u.DmaWork.nbytes) {

            case 1:
            #pragma prefast(suppress: 28138, "Legacy ISA device so we use hardcoded values")   
            WRITE_PORT_UCHAR((PUCHAR) AFIFO, *Item->u.DmaWork.Buffer);
            numxfer = 1;
            break;

            case 2:
            #pragma prefast(suppress: 28138, "Legacy ISA device so we use hardcoded values")                   
            WRITE_PORT_USHORT((PUSHORT) AFIFO,
                              ((PUSHORT) (Item->u.DmaWork.Buffer))[0]);
            numxfer = 2;
            break;

            case 3:
            #pragma prefast(suppress: 28138, "Legacy ISA device so we use hardcoded values")    
            WRITE_PORT_BUFFER_UCHAR((PUCHAR) AFIFO, Item->u.DmaWork.Buffer, 3);
            numxfer = 3;
            break;

            default:
            #pragma prefast(suppress: 28138, "Legacy ISA device so we use hardcoded values")    
            WRITE_PORT_ULONG(AFIFO, ((PULONG) (Item->u.DmaWork.Buffer))[0]);
            numxfer = 4;
            break;
        }                   // write some data bytes

        Item->u.DmaWork.Buffer  += numxfer;
        Item->u.DmaWork.numxfer += numxfer;
        Item->u.DmaWork.nbytes  -= numxfer;

        if(WdfRequestIsCanceled(Item->Request)) {
            //
            // Abort the polling and exit.
            //
                WdfRequestComplete( Item->Request, STATUS_CANCELLED);
            return FALSE;
        }
    }

    if (!Item->u.DmaWork.nbytes) {

        TraceEvents(TRACE_LEVEL_INFORMATION, AMCC_TRACE_IO,
                       "numxfer %I64d", Item->u.DmaWork.numxfer);

        WdfRequestCompleteWithInformation( Item->Request,
                                           STATUS_SUCCESS,
                                           Item->u.DmaWork.numxfer );

        return FALSE;   // Operation completed.
    }

    return TRUE;        // Operation not complete yet
}


