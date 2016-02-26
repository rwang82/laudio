////By Fanxiushu 2011-10-22

#include "common.h"
#include "llx_audfet_cdo.h"

#define MAX_MINIPORTS 2     // Number of maximum miniports.

static NTSTATUS StartDevice( 
    IN  PDEVICE_OBJECT          DeviceObject,     
    IN  PIRP                    Irp,              
    IN  PRESOURCELIST           ResourceList      
);
static NTSTATUS AddDevice( 
    IN  PDRIVER_OBJECT          DriverObject,
    IN  PDEVICE_OBJECT          PhysicalDeviceObject );


extern "C" 
NTSTATUS DriverEntry ( IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING    RegistryPathName )
{
    //
    NTSTATUS statusTmp;

	statusTmp = CreateCDO( DriverObject );
	if ( !NT_SUCCESS( statusTmp ) ) {
        DPT( "error: CreateCDO failed! status: 0x%08x\n", statusTmp );
	}
	
	//
	NTSTATUS status = PcInitializeAdapterDriver( DriverObject, RegistryPathName,
            (PDRIVER_ADD_DEVICE)AddDevice );

    //
    HookMJFunction( DriverObject );
	
	return status;
	/////////
}

static NTSTATUS AddDevice( 
    IN  PDRIVER_OBJECT          DriverObject,
    IN  PDEVICE_OBJECT          PhysicalDeviceObject )
{
	/////
	#pragma warning(disable:28152)
	////
	return PcAddAdapterDevice( DriverObject, PhysicalDeviceObject,
		PCPFNSTARTDEVICE(StartDevice), MAX_MINIPORTS, 0 );
	////////
}

static NTSTATUS StartDevice( 
    IN  PDEVICE_OBJECT          DeviceObject,     
    IN  PIRP                    Irp,              
    IN  PRESOURCELIST           ResourceList      
)
{
	return start_device( DeviceObject, Irp, ResourceList ); 
}

