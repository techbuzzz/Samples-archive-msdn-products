/*++

Copyright (C) Microsoft Corporation, All Rights Reserved

Module Name:

    Device.h

Abstract:

    This module contains the type definitions for the UMDF Skeleton sample
    driver's device callback class.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once
#include "internal.h"

//
// Class for the iotrace driver.
//

class CMyDevice : 
    public CUnknown,
    public IPnpCallbackHardware
{

//
// Private data members.
//
private:
    //
    // Weak reference to framework device
    //    
    IWDFDevice *m_FxDevice;

    //
    // USB Device I/O Target
    //
    IWDFUsbTargetDevice *   m_pIUsbTargetDevice;


    //
    // Device Speed (Low, Full, High)
    //
    UCHAR                   m_Speed;    

//
// Private methods.
//

private:

    CMyDevice(
        VOID
        ) :
        m_FxDevice(NULL),
        m_pIUsbTargetDevice(NULL),
        m_Speed(0)
    {
    }

    ~CMyDevice(
        );

    HRESULT
    Initialize(
        _In_ IWDFDriver *FxDriver,
        _In_ IWDFDeviceInitialize *FxDeviceInit
        );

    //
    // Helper methods
    //

    HRESULT
    CreateUsbIoTargets(
        VOID
        );

    
//
// Public methods
//
public:

    //
    // The factory method used to create an instance of this driver.
    //
    
    static
    HRESULT
    CreateInstance(
        _In_ IWDFDriver *FxDriver,
        _In_ IWDFDeviceInitialize *FxDeviceInit,
        _Out_ PCMyDevice *Device
        );

    IWDFDevice *
    GetFxDevice(
        VOID
        )
    {
        return m_FxDevice;
    }

    HRESULT
    Configure(
        VOID
        );

    IPnpCallbackHardware *
    QueryIPnpCallbackHardware(
        VOID
        )
    {
        AddRef();
        return static_cast<IPnpCallbackHardware *>(this);
    }

//
// COM methods
//
public:

    //
    // IUnknown methods.
    //

    virtual
    ULONG
    STDMETHODCALLTYPE
    AddRef(
        VOID
        )
    {
        return __super::AddRef();
    }

    _At_(this, __drv_freesMem(object))
    virtual
    ULONG
    STDMETHODCALLTYPE
    Release(
        VOID
       )
    {
        return __super::Release();
    }

    virtual
    HRESULT
    STDMETHODCALLTYPE
    QueryInterface(
        _In_ REFIID InterfaceId,
        _Outptr_ PVOID *Object
        );

    //
    // IPnpCallbackHardware
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnPrepareHardware(
            _In_ IWDFDevice *FxDevice
            );

    virtual
    HRESULT
    STDMETHODCALLTYPE
    OnReleaseHardware(
        _In_ IWDFDevice *FxDevice
        );
};
