#include "llx_audfet_cdo.h"
#include "llx_audio_data_cache.h"
#include "llx_block_data_cache.h"
#include "llx_ring_cache.h"
#include "dk_micdata_monitor.h"
//
#define STRING_LLXAFCDO_DEVNAME (L"\\Device\\DKAFCDO")
#define STRING_LLXAFCDO_SYMBOLICLINKNAME (L"\\??\\DKAFCDO")
#define USTRING_DK_EVENTNAME_MICNEEDDATA (L"\\BaseNamedObjects\\DK_EVENTNAME_MICNEEDDATA")
#define USTRING_DK_EVENTNAME_MICSLEEP (L"\\BaseNamedObjects\\DK_EVENTNAME_MICSLEEP")

//
#ifndef DPT
  #ifdef DBG
    #define DPT   DbgPrint
  #else
    #define DPT   //
  #endif //DBG
#endif //DPT
//
PDRIVER_DISPATCH gfnC2B_Create = NULL;
PDRIVER_DISPATCH gfnC2B_Close = NULL;
PDRIVER_DISPATCH gfnC2B_DeviceControl = NULL;
PDRIVER_UNLOAD gfnC2B_Unload = NULL;
PDEVICE_OBJECT g_pCDO = NULL;
//
NTSTATUS LLXDisPatchCreate (PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS LLXDisPatchClose (PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS LLXDispatchDeviceControl( IN PDEVICE_OBJECT fdo, IN PIRP Irp );
void LLXWDMUnload( IN PDRIVER_OBJECT DriverObject );
//
static void prepareMJFunction4CDO(PDRIVER_OBJECT DriverObject);
//
void DKMicDataState::init() {
	KeInitializeSpinLock(&m_SpinLock);
	m_eDKMicDataState = EDKMICDATA_STATE_SLEEP;
	m_bStateChanged = false;
	KeQuerySystemTime(&m_timeTagLastStartFlag);
}

void DKMicDataState::lock() {
	KeAcquireSpinLock(&m_SpinLock, &m_SpinIrql);
}

void DKMicDataState::unlock() {
	KeReleaseSpinLock(&m_SpinLock, m_SpinIrql);
}

void DKMicDataState::setStartFlag() {
	KeQuerySystemTime(&m_timeTagLastStartFlag);
	if (m_eDKMicDataState == EDKMICDATA_STATE_NEEDDATA)
		return;
	m_eDKMicDataState = EDKMICDATA_STATE_NEEDDATA;
	m_bStateChanged = true;
}

void DKMicDataState::setStopFlag() {
	if (m_eDKMicDataState == EDKMICDATA_STATE_SLEEP)
		return;
	m_eDKMicDataState = EDKMICDATA_STATE_SLEEP;
	m_bStateChanged = true;
}

void DKMicDataState::getTimeTagLastStartFlag(PLARGE_INTEGER plIntTimeTag) {
	if (plIntTimeTag == NULL)
		return;
	RtlCopyMemory(plIntTimeTag, &m_timeTagLastStartFlag, sizeof(LARGE_INTEGER));
}

bool DKMicDataState::isStateChanged() {
	return m_bStateChanged;
}

void DKMicDataState::resetStateChangedFlag() {
	m_bStateChanged = false;
}
//
NTSTATUS CreateCDO( IN PDRIVER_OBJECT DriverObject ) {
    DPT( "Enter CreateCDO\n" );
	NTSTATUS status;
	PDEVICE_OBJECT fdo;
	UNICODE_STRING devName;
	LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = NULL;
	//
	prepareMJFunction4CDO( DriverObject );
	//
	RtlInitUnicodeString(&devName, STRING_LLXAFCDO_DEVNAME);

	//
	status = IoCreateDevice( 
	    DriverObject, 
        sizeof( LLXAFCDO_DEVICE_EXTENSION ),
        &devName,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &fdo);
		//
	if ( !NT_SUCCESS(status) ) {
	    DPT( "Exit CreateCDO  IoCreateDevice failed, status:0x%08x\n", status );
	    return status;
	}
    pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)fdo->DeviceExtension;
	if ( !pCDODevExt ) {
        DPT( "error : pCDODevExt == NULL" );
	} else {
		pCDODevExt->m_hADC = adcobj_create();
//		pCDODevExt->m_pBDC4MicData = (llx_block_data_cache*)ExAllocatePool( NonPagedPool, sizeof( struct llx_block_data_cache ) );
		pCDODevExt->m_pBDC4MicData = (llx_ring_cache*)ExAllocatePool( NonPagedPool, sizeof( struct llx_ring_cache ) );
		pCDODevExt->m_pBDC4MicData->init();
		pCDODevExt->m_stateMicData.init(); 
		pCDODevExt->m_hEventMicNeedData = NULL;
		pCDODevExt->m_hEventMicSleep = NULL;
	}
	g_pCDO = fdo;
	//
	dk_start_micdata_monitor();

	// 创建符号链接
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString( &symLinkName, STRING_LLXAFCDO_SYMBOLICLINKNAME );
	status = IoCreateSymbolicLink( &(UNICODE_STRING)symLinkName, &(UNICODE_STRING)devName );
	//
	if ( !NT_SUCCESS( status ) ) {
	    IoDeleteSymbolicLink( &(UNICODE_STRING)symLinkName );
		status = IoCreateSymbolicLink( &(UNICODE_STRING)symLinkName, &(UNICODE_STRING)devName );
		//
		if ( !NT_SUCCESS(status) ) {
		    DPT( "Leave CreateCDO  IoCreateSymbolicLink failed: 0x%08x\n", status );
			return status;
		}
	}
	pCDODevExt->m_ustrSymLinkName = symLinkName;
	//设置设备标志
	fdo->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
	fdo->Flags &= ~DO_DEVICE_INITIALIZING;
	
	DPT( "Leave CreateCDO\n" );
	return STATUS_SUCCESS;

}

NTSTATUS DestroyCDO() {
	DPT( "Enter DestroyCDO\n" );
	if (!g_pCDO)
		return -1;
	LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;
	
	// delete symbollink
	UNICODE_STRING ustrSymLinkName = pCDODevExt->m_ustrSymLinkName;
	::IoDeleteSymbolicLink( &ustrSymLinkName );

	//
	dk_stop_micdata_monitor();
	// 
	adcobj_destroy(pCDODevExt->m_hADC);

	//
	if (pCDODevExt->m_pBDC4MicData) {
		ExFreePool(pCDODevExt->m_pBDC4MicData);
		pCDODevExt->m_pBDC4MicData = NULL;
	}

	//
	::IoDeleteDevice(g_pCDO);
	g_pCDO = NULL;

	return STATUS_SUCCESS;
}

static void prepareMJFunction4CDO( PDRIVER_OBJECT DriverObject ) {
    //
    DriverObject->MajorFunction[IRP_MJ_CREATE] = LLXDisPatchCreate;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = LLXDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = LLXDisPatchClose;
	DriverObject->DriverUnload = LLXWDMUnload;
    
}

#pragma PAGEDCODE
void HookMJFunction( PDRIVER_OBJECT DriverObject ) {
	PAGED_CODE();

    if ( DriverObject->MajorFunction[ IRP_MJ_CREATE ] != LLXDisPatchCreate ) {
		gfnC2B_Create = DriverObject->MajorFunction[ IRP_MJ_CREATE ];
		DriverObject->MajorFunction[ IRP_MJ_CREATE ] = LLXDisPatchCreate;
	}

    if ( DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] != LLXDispatchDeviceControl ) {
		gfnC2B_DeviceControl = DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];
		DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = LLXDispatchDeviceControl;
    }
	if ( DriverObject->MajorFunction[ IRP_MJ_CLOSE ] != LLXDisPatchClose ) {
        gfnC2B_Close = DriverObject->MajorFunction[ IRP_MJ_CLOSE ];
        DriverObject->MajorFunction[ IRP_MJ_CLOSE ] = LLXDisPatchClose;
	}
	if ( DriverObject->DriverUnload != LLXWDMUnload ) {
		gfnC2B_Unload = DriverObject->DriverUnload;
		DriverObject->DriverUnload = LLXWDMUnload;
	}

}


#pragma PAGEDCODE
NTSTATUS
LLXDisPatchClose (PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	if (DeviceObject == g_pCDO) {
		DPT("Enter LLXDisPatchClose, DeviceObject == g_pCDO\n");
	}

	PAGED_CODE();
    DPT( "Enter LLXDisPatchClose. DeviceObject %s g_pCDO\n", ( DeviceObject == g_pCDO ) ? "==" : "!=" );
	if ( gfnC2B_Close && DeviceObject != g_pCDO ) {
		NTSTATUS status = gfnC2B_Close( DeviceObject, Irp );
		DPT( "Exit LLXDisPatchClose.\n" );
        return status;
	}
	UNREFERENCED_PARAMETER(DeviceObject);

	//
	LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;

	//
	if (pCDODevExt->m_hEventMicNeedData) {
		::ZwClose(pCDODevExt->m_hEventMicNeedData);
		pCDODevExt->m_hEventMicNeedData = NULL;
	}
	if (pCDODevExt->m_hEventMicSleep) {
		::ZwClose(pCDODevExt->m_hEventMicSleep);
		pCDODevExt->m_hEventMicSleep = NULL;
	}

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest( Irp, IO_NO_INCREMENT );

	return STATUS_SUCCESS;
}


#pragma PAGEDCODE
NTSTATUS
LLXDisPatchCreate (PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    DPT( "Enter LLXDisPatchCreate. DeviceObject %s g_pCDO\n", ( DeviceObject == g_pCDO ) ? "==" : "!=" );
	
    if ( gfnC2B_Create && DeviceObject != g_pCDO ) {
		NTSTATUS status = gfnC2B_Create( DeviceObject, Irp );
		DPT( "Exit LLXDisPatchCreate.\n" );
        return status;
	}

    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

	UNICODE_STRING ustrEventNameMicNeedData;
	UNICODE_STRING ustrEventNameMicSleep;
	LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;
	//
	RtlInitUnicodeString(&ustrEventNameMicNeedData, USTRING_DK_EVENTNAME_MICNEEDDATA);
	RtlInitUnicodeString(&ustrEventNameMicSleep, USTRING_DK_EVENTNAME_MICSLEEP);
	PKEVENT pkEventMicNeedData = ::IoCreateSynchronizationEvent(&ustrEventNameMicNeedData, &(pCDODevExt->m_hEventMicNeedData));
	PKEVENT pkEventMicSleep = ::IoCreateSynchronizationEvent(&ustrEventNameMicSleep, &(pCDODevExt->m_hEventMicSleep));
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

#pragma PAGEDCODE
void LLXWDMUnload( IN PDRIVER_OBJECT DriverObject ) {
    PAGED_CODE();

	DPT( "Enter LLXWDMUnload\n" );

	if ( gfnC2B_Unload ) {
        gfnC2B_Unload( DriverObject );
	}

    if ( g_pCDO ) {
		LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;
		//
        adcobj_destroy( pCDODevExt->m_hADC );
		pCDODevExt->m_hADC = NULL;
		//
		ExFreePool( pCDODevExt->m_pBDC4MicData );
		pCDODevExt->m_pBDC4MicData = NULL;

		// *** notice ***/
		// if need to delete pCDODevExt ?
	}
	
	DPT( "Exit LLXWDMUnload\n" );

}

#define IOCTL_AUDIO_FETCH                          CTL_CODE( FILE_DEVICE_UNKNOWN, 0x802, METHOD_OUT_DIRECT, 0x03 )
#define IOCTL_AUDIO_CLEAR                          CTL_CODE( FILE_DEVICE_UNKNOWN, 0x803, METHOD_OUT_DIRECT, 0x03 )
#define IOCTL_MIC_PUSH                             CTL_CODE( FILE_DEVICE_UNKNOWN, 0x804, METHOD_IN_DIRECT, 0x03 )
#define IOCTL_FETCH_MICDATA_START_EVENT            CTL_CODE( FILE_DEVICE_UNKNOWN, 0x805, METHOD_OUT_DIRECT, 0x03 )
#define IOCTL_FETCH_MICDATA_STOP_EVENT             CTL_CODE( FILE_DEVICE_UNKNOWN, 0x806, METHOD_OUT_DIRECT, 0x03 )
#define IOCTL_FETCH_MICDATA_STATE                  CTL_CODE( FILE_DEVICE_UNKNOWN, 0x807, METHOD_OUT_DIRECT, 0x03 )

NTSTATUS On_IOCTL_AUDIO_FETCH(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS On_IOCTL_FETCH_MICDATA_START_EVENT(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS On_IOCTL_FETCH_MICDATA_STOP_EVENT(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS On_IOCTL_MIC_PUSH(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS On_IOCTL_AUDIO_CLEAR(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS On_IOCTL_FETCH_MICDATA_STATE(IN PDEVICE_OBJECT fdo, IN PIRP Irp);

#pragma PAGEDCODE
NTSTATUS LLXDispatchDeviceControl( IN PDEVICE_OBJECT fdo, IN PIRP Irp ) {
    PAGED_CODE();

	//DPT( "enter HelloWDMDispatchRoutine!... fdo:%p, Irp->Type:0x%08x\n", fdo, Irp->Type );
    if ( gfnC2B_DeviceControl && fdo != g_pCDO ) {
        return gfnC2B_DeviceControl( fdo, Irp );
    }

	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation( Irp );
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	if ( code == IOCTL_AUDIO_FETCH ) {
		return On_IOCTL_AUDIO_FETCH( fdo, Irp );
	} else if ( code == IOCTL_AUDIO_CLEAR ) {
		return On_IOCTL_AUDIO_CLEAR( fdo, Irp );
	} else if ( code == IOCTL_MIC_PUSH ) {
		return On_IOCTL_MIC_PUSH( fdo, Irp );
	} else if (code == IOCTL_FETCH_MICDATA_START_EVENT) {
		return On_IOCTL_FETCH_MICDATA_START_EVENT(fdo, Irp);
	} else if (code == IOCTL_FETCH_MICDATA_STOP_EVENT) {
		return On_IOCTL_FETCH_MICDATA_STOP_EVENT(fdo, Irp);
	} else if (code == IOCTL_FETCH_MICDATA_STATE ) {
		return On_IOCTL_FETCH_MICDATA_STATE( fdo, Irp );
	}
	else {
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0; // no bytes xfered.
	}

	IoCompleteRequest( Irp, IO_NO_INCREMENT );
	//DPT( "leave HelloWDMDispatchRoutine!...\n" );
    return STATUS_SUCCESS;
}

NTSTATUS On_IOCTL_AUDIO_FETCH(IN PDEVICE_OBJECT fdo, IN PIRP Irp) {
	PVOID pBufSys = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	if (pCDODevExt->m_hADC) {
		unsigned int uLenBufSys = stack->Parameters.DeviceIoControl.OutputBufferLength;
		if (0 == audio_data_fetch(pCDODevExt->m_hADC, pBufSys, &uLenBufSys)) {
			if (uLenBufSys != 0) {
				// DPT( "call audio_data_fetch success, uLenBufSys:%d\n", uLenBufSys );
			}
		}
		else {
			DPT("call audio_data_fetch failed!\n");
		}

		if (uLenBufSys > 0) {
			DPT("got IOCTL_AUDIO_FETCH code. fetchBufferCount:%d\n", uLenBufSys);
		}
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = uLenBufSys;
	}
	else {
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
	}

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS On_IOCTL_AUDIO_CLEAR(IN PDEVICE_OBJECT fdo, IN PIRP Irp) {
	LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;
	audio_data_clear(pCDODevExt->m_hADC);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0; // no bytes xfered.
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS On_IOCTL_MIC_PUSH(IN PDEVICE_OBJECT fdo, IN PIRP Irp) {
	LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	unsigned char* pBufInput = (unsigned char*)Irp->AssociatedIrp.SystemBuffer;
	unsigned int uLenBufInput = stack->Parameters.DeviceIoControl.InputBufferLength;

	pCDODevExt->m_pBDC4MicData->lock();
	pCDODevExt->m_pBDC4MicData->pushBuf(pBufInput, uLenBufInput);
	DPT("IOCTL_MIC_PUSH: uLenBufInput:%d\n", uLenBufInput);
	pCDODevExt->m_pBDC4MicData->unlock();

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS On_IOCTL_FETCH_MICDATA_START_EVENT(IN PDEVICE_OBJECT fdo, IN PIRP Irp) {

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;



	//LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;
	//PVOID pBufSys = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	//PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	//unsigned int uLenBufSys = stack->Parameters.DeviceIoControl.OutputBufferLength;
	//
	//if (uLenBufSys < sizeof(HANDLE)) {
	//	Irp->IoStatus.Status = STATUS_SUCCESS;
	//	Irp->IoStatus.Information = 0;
	//}
	//else {
	//	HANDLE hEventUserMode = 0;
	//	NTSTATUS statusTmp = ObOpenObjectByPointer(&pCDODevExt->m_hKEventMicNeedData, 0, NULL, SYNCHRONIZE, *ExEventObjectType, UserMode, (HANDLE*)&hEventUserMode);
	//	if (NT_SUCCESS(statusTmp)) {
	//		RtlCopyMemory(pBufSys, &hEventUserMode, sizeof(HANDLE));
	//		Irp->IoStatus.Status = STATUS_SUCCESS;
	//		Irp->IoStatus.Information = sizeof(HANDLE);
	//	}
	//	else {
	//		Irp->IoStatus.Status = STATUS_SUCCESS;
	//		Irp->IoStatus.Information = 0;
	//	}
	//}

	//IoCompleteRequest(Irp, IO_NO_INCREMENT);
	//return STATUS_SUCCESS;
}

NTSTATUS On_IOCTL_FETCH_MICDATA_STOP_EVENT(IN PDEVICE_OBJECT fdo, IN PIRP Irp) {
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;



	//LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;
	//PVOID pBufSys = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	//PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	//unsigned int uLenBufSys = stack->Parameters.DeviceIoControl.OutputBufferLength;

	//if (uLenBufSys < sizeof(HANDLE)) {
	//	Irp->IoStatus.Status = STATUS_SUCCESS;
	//	Irp->IoStatus.Information = 0;
	//}
	//else {
	//	HANDLE hEventUserMode = 0;
	//	NTSTATUS statusTmp = ObOpenObjectByPointer(&pCDODevExt->m_hKEventMicSleep, 0, NULL, SYNCHRONIZE, *ExEventObjectType, UserMode, (HANDLE*)&hEventUserMode);
	//	if (NT_SUCCESS(statusTmp)) {
	//		RtlCopyMemory(pBufSys, &hEventUserMode, sizeof(HANDLE));
	//		Irp->IoStatus.Status = STATUS_SUCCESS;
	//		Irp->IoStatus.Information = sizeof(HANDLE);
	//	}
	//	else {
	//		Irp->IoStatus.Status = STATUS_SUCCESS;
	//		Irp->IoStatus.Information = 0;
	//	}
	//}

	//IoCompleteRequest(Irp, IO_NO_INCREMENT);
	//return STATUS_SUCCESS;
}

NTSTATUS On_IOCTL_FETCH_MICDATA_STATE(IN PDEVICE_OBJECT fdo, IN PIRP Irp) {
	LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;
	PVOID pBufSys = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	unsigned int uLenBufSys = stack->Parameters.DeviceIoControl.OutputBufferLength;

	if (uLenBufSys < sizeof(ENUMDKMICDATA_STATE)) {
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = 0;
	}
	else {
		pCDODevExt->m_stateMicData.lock();
		ENUMDKMICDATA_STATE eStateMicData = pCDODevExt->m_stateMicData.getState();
		RtlCopyMemory(pBufSys, &eStateMicData, sizeof(ENUMDKMICDATA_STATE));
		pCDODevExt->m_stateMicData.unlock();
		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = sizeof(ENUMDKMICDATA_STATE);
	}

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}



