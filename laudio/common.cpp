//By Fanxiushu 2011-10-24

#include "common.h"
#include "top_wav.h"

///common function
PWAVEFORMATEX  GetWaveFormatEx( IN  PKSDATAFORMAT pDataFormat  )
{

    PWAVEFORMATEX           pWfx = NULL;
    
    // If this is a known dataformat extract the waveformat info.
    //
    if
    ( 
        pDataFormat &&
        ( IsEqualGUIDAligned(pDataFormat->MajorFormat, 
                KSDATAFORMAT_TYPE_AUDIO)             &&
          ( IsEqualGUIDAligned(pDataFormat->Specifier, 
                KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
            IsEqualGUIDAligned(pDataFormat->Specifier, 
                KSDATAFORMAT_SPECIFIER_DSOUND) ) )
    )
    {
        pWfx = PWAVEFORMATEX(pDataFormat + 1);

        if (IsEqualGUIDAligned(pDataFormat->Specifier, 
                KSDATAFORMAT_SPECIFIER_DSOUND))
        {
            PKSDSOUND_BUFFERDESC    pwfxds;

            pwfxds = PKSDSOUND_BUFFERDESC(pDataFormat + 1);
            pWfx = &pwfxds->WaveFormatEx;
        }
    }

    return pWfx;
} 
NTSTATUS ValidateFormat(IN  PKSDATAFORMAT  pDataFormat)
{
	NTSTATUS status = STATUS_INVALID_PARAMETER;
	PWAVEFORMATEX               pwfx;

    pwfx = GetWaveFormatEx(pDataFormat);
    if (pwfx)
    {
        if (IS_VALID_WAVEFORMATEX_GUID(&pDataFormat->SubFormat))
        {
            USHORT wfxID = EXTRACT_WAVEFORMATEX_ID(&pDataFormat->SubFormat);

            switch (wfxID)
            {
                case WAVE_FORMAT_PCM:
                {
                    switch (pwfx->wFormatTag)
                    {
                        case WAVE_FORMAT_PCM:
                        {
                            if( pwfx->nChannels>=MIN_CHANNELS&&pwfx->nChannels<=MAX_CHANNELS_PCM &&
								pwfx->wBitsPerSample>=MIN_BITS_PER_SAMPLE_PCM && pwfx->wBitsPerSample<=MAX_BITS_PER_SAMPLE_PCM &&
								pwfx->nSamplesPerSec>=MIN_SAMPLE_RATE && pwfx->nSamplesPerSec<=MAX_SAMPLE_RATE )
							{
								status = STATUS_SUCCESS; 
							}
                            break;
                        }
                    }
                    break;
                }
                default:
                    DPT("Invalid format EXTRACT_WAVEFORMATEX_ID!");
                    break;
            }
        }
        else
        {
            DPT("Invalid pDataFormat->SubFormat!" );
        }
    }

    return status;
} 

///copy from MSVAD
NTSTATUS                        
PropertyHandler_BasicSupport
(
    IN PPCPROPERTY_REQUEST         PropertyRequest,
    IN ULONG                       Flags,
    IN DWORD                       PropTypeSetId
)

{

    ASSERT(Flags & KSPROPERTY_TYPE_BASICSUPPORT);

    NTSTATUS                    ntStatus = STATUS_INVALID_PARAMETER;

    if (PropertyRequest->ValueSize >= sizeof(KSPROPERTY_DESCRIPTION))
    {
        // if return buffer can hold a KSPROPERTY_DESCRIPTION, return it
        //
        PKSPROPERTY_DESCRIPTION PropDesc = 
            PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

        PropDesc->AccessFlags       = Flags;
        PropDesc->DescriptionSize   = sizeof(KSPROPERTY_DESCRIPTION);
        if  (VT_ILLEGAL != PropTypeSetId)
        {
            PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
            PropDesc->PropTypeSet.Id    = PropTypeSetId;
        }
        else
        {
            PropDesc->PropTypeSet.Set   = GUID_NULL;
            PropDesc->PropTypeSet.Id    = 0;
        }
        PropDesc->PropTypeSet.Flags = 0;
        PropDesc->MembersListCount  = 0;
        PropDesc->Reserved          = 0;

        PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
        ntStatus = STATUS_SUCCESS;
    } 
    else if (PropertyRequest->ValueSize >= sizeof(ULONG))
    {
        // if return buffer can hold a ULONG, return the access flags
        //
        *(PULONG(PropertyRequest->Value)) = Flags;

        PropertyRequest->ValueSize = sizeof(ULONG);
        ntStatus = STATUS_SUCCESS;                    
    }
    else
    {
        PropertyRequest->ValueSize = 0;
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    return ntStatus;
} // PropertyHandler_BasicSupport

//-----------------------------------------------------------------------------
NTSTATUS 
ValidatePropertyParams
(
    IN PPCPROPERTY_REQUEST      PropertyRequest, 
    IN ULONG                    cbSize,
    IN ULONG                    cbInstanceSize /* = 0  */
)

{

    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    if (PropertyRequest && cbSize)
    {
        // If the caller is asking for ValueSize.
        //
        if (0 == PropertyRequest->ValueSize) 
        {
            PropertyRequest->ValueSize = cbSize;
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        // If the caller passed an invalid ValueSize.
        //
        else if (PropertyRequest->ValueSize < cbSize)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        else if (PropertyRequest->InstanceSize < cbInstanceSize)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        // If all parameters are OK.
        // 
        else if (PropertyRequest->ValueSize == cbSize)
        {
            if (PropertyRequest->Value)
            {
                ntStatus = STATUS_SUCCESS;
                //
                // Caller should set ValueSize, if the property 
                // call is successful.
                //
            }
        }
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    
    // Clear the ValueSize if unsuccessful.
    //
    if (PropertyRequest &&
        STATUS_SUCCESS != ntStatus &&
        STATUS_BUFFER_OVERFLOW != ntStatus)
    {
        PropertyRequest->ValueSize = 0;
    }

    return ntStatus;
} // ValidatePropertyParams


////
class AdapterPowerMgr: 
     public IAdapterPowerManagement,
	 public CUnknown
{
protected:
	DEVICE_POWER_STATE   m_PowerState;
public:
	/////
	// Default CUnknown
	DECLARE_STD_UNKNOWN();
	AdapterPowerMgr( PUNKNOWN pUnknownOuter);
	~AdapterPowerMgr();
	
	//=====================================================================
	// Default IAdapterPowerManagement
	IMP_IAdapterPowerManagement;
	//////////////////////////////////////////
	NTSTATUS Init(PDEVICE_OBJECT fdo);
};


AdapterPowerMgr::AdapterPowerMgr
      (PUNKNOWN pUnknownOuter):CUnknown( pUnknownOuter )
{
	////
	m_PowerState  = PowerDeviceD0;
}

AdapterPowerMgr::~AdapterPowerMgr()
{
}

STDMETHODIMP
AdapterPowerMgr::NonDelegatingQueryInterface( REFIID  Interface, PVOID * Object )
{
	if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN((AdapterPowerMgr*)(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IAdapterPowerManagement))
    {
        *Object = PVOID(PADAPTERPOWERMANAGEMENT(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }
	return STATUS_INVALID_PARAMETER;
}

NTSTATUS AdapterPowerMgr::Init(PDEVICE_OBJECT fdo)
{
	return STATUS_SUCCESS;
}
/////
STDMETHODIMP_(void)
AdapterPowerMgr::PowerChangeState( IN  POWER_STATE  NewState )
{
	////
    if (NewState.DeviceState != m_PowerState)
    {
        // switch on new state
        switch (NewState.DeviceState)
        {
            case PowerDeviceD0:
            case PowerDeviceD1:
            case PowerDeviceD2:
            case PowerDeviceD3:
                m_PowerState = NewState.DeviceState;
                DPT("Entering D%d", (ULONG(m_PowerState) - ULONG(PowerDeviceD0)) );
                break;
            default:
                DPT("Unknown Device Power State");
                break;
        }
    }
} 

STDMETHODIMP_(NTSTATUS)
AdapterPowerMgr::QueryDeviceCapabilities( IN  PDEVICE_CAPABILITIES  PowerDeviceCaps )
{
    UNREFERENCED_PARAMETER(PowerDeviceCaps);

    DPT("[AdapterPowerMgr::QueryDeviceCapabilities]");

    return (STATUS_SUCCESS);
} 

STDMETHODIMP_(NTSTATUS)
AdapterPowerMgr::QueryPowerChangeState( IN  POWER_STATE  NewStateQuery )
{
    UNREFERENCED_PARAMETER(NewStateQuery);

    DPT("[AdapterPowerMgr::QueryPowerChangeState]");

    return STATUS_SUCCESS;
} 


////////////////


//=============================================================================
static NTSTATUS
InstallSubdevice
( 
    __in        PDEVICE_OBJECT          DeviceObject,
    __in        PIRP                    Irp,
    __in        PWSTR                   Name,
    __in        REFGUID                 PortClassId,
    __in        REFGUID                 MiniportClassId,
    __in_opt    PFNCREATEINSTANCE       MiniportCreate,
    __in_opt    PUNKNOWN                UnknownAdapter,
    __in_opt    PRESOURCELIST           ResourceList,
    __in        REFGUID                 PortInterfaceId,
    __out_opt   PUNKNOWN *              OutPortInterface,
    __out_opt   PUNKNOWN *              OutPortUnknown
)
{

    ASSERT(DeviceObject);
    ASSERT(Irp);
    ASSERT(Name);

    NTSTATUS                    ntStatus;
    PPORT                       port = NULL;
    PUNKNOWN                    miniport = NULL;
     
    DPT( "[InstallSubDevice %S]", Name );

    // Create the port driver object
    //
    ntStatus = PcNewPort(&port, PortClassId);

    // Create the miniport object
    //
    if (NT_SUCCESS(ntStatus))
    {
        if (MiniportCreate)
        {
            ntStatus = 
                MiniportCreate
                ( 
                    &miniport,
                    MiniportClassId,
                    NULL,
                    NonPagedPool 
                );
        }
        else
        {
            ntStatus = 
                PcNewMiniport
                (
                    (PMINIPORT *) &miniport, 
                    MiniportClassId
                );
        }
    }

    // Init the port driver and miniport in one go.
    //
    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = 
            port->Init
            ( 
                DeviceObject,
                Irp,
                miniport,
                UnknownAdapter,
                ResourceList 
            );

        if (NT_SUCCESS(ntStatus))
        {
            // Register the subdevice (port/miniport combination).
            //
            ntStatus = 
                PcRegisterSubdevice
                ( 
                    DeviceObject,
                    Name,
                    port 
                );
        }

        // We don't need the miniport any more.  Either the port has it,
        // or we've failed, and it should be deleted.
        //
        miniport->Release();
    }

    // Deposit the port interfaces if it's needed.
    //
    if (NT_SUCCESS(ntStatus))
    {
        if (OutPortUnknown)
        {
            ntStatus = 
                port->QueryInterface
                ( 
                    IID_IUnknown,
                    (PVOID *)OutPortUnknown 
                );
        }

        if (OutPortInterface)
        {
            ntStatus = 
                port->QueryInterface
                ( 
                    PortInterfaceId,
                    (PVOID *) OutPortInterface 
                );
        }
    }

    if (port)
    {
        port->Release();
    }

    return ntStatus;
} 

static NTSTATUS
createAdapterPowerMgr(OUT PUNKNOWN *Unknown, IN  REFCLSID, 
			IN  PUNKNOWN  UnknownOuter OPTIONAL,IN  POOL_TYPE PoolType )
{
	STD_CREATE_BODY( AdapterPowerMgr, Unknown, UnknownOuter, PoolType );
}
static NTSTATUS
createWaveCyclic(OUT PUNKNOWN *Unknown, IN  REFCLSID, 
			IN  PUNKNOWN  UnknownOuter OPTIONAL,IN  POOL_TYPE PoolType )
{
	STD_CREATE_BODY( WaveCyclic, Unknown, UnknownOuter, PoolType );
}
static NTSTATUS
createTopology(OUT PUNKNOWN *Unknown, IN  REFCLSID, 
			IN  PUNKNOWN  UnknownOuter OPTIONAL,IN  POOL_TYPE PoolType )
{
	STD_CREATE_BODY( Topology, Unknown, UnknownOuter, PoolType );
}

NTSTATUS
start_device
( 
    IN  PDEVICE_OBJECT          DeviceObject,     
    IN  PIRP                    Irp,              
    IN  PRESOURCELIST           ResourceList      
)  
{
	NTSTATUS status = STATUS_SUCCESS;
	PUNKNOWN                    unknownTopology = NULL;
    PUNKNOWN                    unknownWave     = NULL;
	PUNKNOWN					unknownPowerMgr = NULL; 

#define CHK_RET( SS )   if( !NT_SUCCESS( status ) ){ DPT( SS" <0x%X>", status ); break; }

	do{
		status = createAdapterPowerMgr( &unknownPowerMgr, IID_IAdapterPowerManagement,NULL, NonPagedPool ); 
		CHK_RET( "createAdapterPowerMgr Error." );  

		AdapterPowerMgr* mgr = (AdapterPowerMgr*)unknownPowerMgr; 
		status = mgr->Init( DeviceObject ); 
		CHK_RET( "AdapterPowerMgr->Init() Error" ) ; 

		status = PcRegisterAdapterPowerManagement( unknownPowerMgr, DeviceObject ); 
		CHK_RET( "PcRegisterAdapterPowerManagement Error" );  
		/////
		//
		status = InstallSubdevice( DeviceObject, Irp, L"Topology",
                CLSID_PortTopology, CLSID_PortTopology, 
                createTopology,
                NULL, NULL, IID_IPortTopology,
                NULL,
                &unknownTopology ); 
		CHK_RET("InstallSubdevice Topology Error");

		////
		status = InstallSubdevice( DeviceObject, Irp, L"Wave",
                CLSID_PortWaveCyclic, CLSID_PortWaveCyclic,   
                createWaveCyclic,
                NULL,
                NULL,
                IID_IPortWaveCyclic,
                NULL,
                &unknownWave );
		CHK_RET("InstallSubdevice Wave Error");

		///////
        if( !unknownTopology || !unknownWave ) break;

		status = PcRegisterPhysicalConnection( DeviceObject,
                    unknownTopology, KSPIN_TOPO_WAVEIN_DEST ,
                    unknownWave, KSPIN_WAVE_CAPTURE_SOURCE
                );
		CHK_RET("PcRegisterPhysicalConnection Topo-->Wave Error");

		status = PcRegisterPhysicalConnection( DeviceObject,
                        unknownWave, KSPIN_WAVE_RENDER_SOURCE,
                        unknownTopology, KSPIN_TOPO_WAVEOUT_SOURCE
                    );
		CHK_RET("PcRegisterPhysicalConnection Wave-->Topo Error");
		///////

	}while(0); 

	SAFE_RELEASE( unknownTopology );
	SAFE_RELEASE( unknownWave );
	SAFE_RELEASE( unknownPowerMgr );
	////
	DPT("Start_device < %s >.", NT_SUCCESS(status) ? "OK" : "FAIL" );

	return status;
}

