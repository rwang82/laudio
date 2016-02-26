/// By Fanxiushu 2011-10-24

#include "top_wav.h"

static NTSTATUS
PropertyHandler_TopoFilter( IN PPCPROPERTY_REQUEST PropertyRequest);
static NTSTATUS
PropertyHandler_Topology( IN PPCPROPERTY_REQUEST PropertyRequest );

#include "toptable.h"


Topology::Topology
      (PUNKNOWN pUnknownOuter):CUnknown( pUnknownOuter )
{
	////
	m_topodesc = NULL; 
	////
	m_ulMux = 3 ; 
}

Topology::~Topology()
{
	DPT("Topology::~Topology()");
}

STDMETHODIMP
Topology::NonDelegatingQueryInterface( REFIID  Interface, PVOID * Object )
{
	if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN((Topology*)(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportTopology))
    {
        *Object = PVOID((IMiniportTopology*)(this));
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

////IMiniportTopology
STDMETHODIMP_(NTSTATUS)
Topology::DataRangeIntersection
( 
    IN  ULONG                       PinId,
    IN  PKSDATARANGE                ClientDataRange,
    IN  PKSDATARANGE                MyDataRange,
    IN  ULONG                       OutputBufferLength,
    OUT PVOID                       ResultantFormat,
    OUT PULONG                      ResultantFormatLength 
)
{
	NTSTATUS status = STATUS_NOT_IMPLEMENTED;
	DPT("Topology::DataRangeIntersection");

	return status;
}

STDMETHODIMP_(NTSTATUS)
Topology::GetDescription
( 
    OUT PPCFILTER_DESCRIPTOR * OutFilterDescriptor 
)
{
	NTSTATUS status = STATUS_SUCCESS;
	DPT("Topology::GetDescription");

	*OutFilterDescriptor = m_topodesc;

	return status;
}

STDMETHODIMP_(NTSTATUS)
Topology::Init( IN  PUNKNOWN  UnknownAdapter_, 
		IN  PRESOURCELIST ResourceList_, IN  PPORTTOPOLOGY  Port_ )
{
	NTSTATUS status = STATUS_SUCCESS;
	DPT("Topology::Init");

	m_topodesc = &MiniportFilterDescriptor; 

	m_ulMux = KSPIN_TOPO_MIC_SOURCE;
	///

	RtlFillMemory(m_VolumeControls, sizeof(LONG) * MAX_TOPOLOGY_NODES, 0xFF);
    RtlFillMemory(m_MuteControls, sizeof(BOOL) * MAX_TOPOLOGY_NODES, FALSE );
	////

	return status;
}

/// Properter

NTSTATUS
Topology::PropertyHandlerBasicSupportVolume( IN  PPCPROPERTY_REQUEST     PropertyRequest)
{

    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    ULONG                       cbFullProperty = 
        sizeof(KSPROPERTY_DESCRIPTION) +
        sizeof(KSPROPERTY_MEMBERSHEADER) +
        sizeof(KSPROPERTY_STEPPING_LONG);

    if (PropertyRequest->ValueSize >= (sizeof(KSPROPERTY_DESCRIPTION)))
    {
        PKSPROPERTY_DESCRIPTION PropDesc = 
            PKSPROPERTY_DESCRIPTION(PropertyRequest->Value);

        PropDesc->AccessFlags       = KSPROPERTY_TYPE_ALL;
        PropDesc->DescriptionSize   = cbFullProperty;
        PropDesc->PropTypeSet.Set   = KSPROPTYPESETID_General;
        PropDesc->PropTypeSet.Id    = VT_I4;
        PropDesc->PropTypeSet.Flags = 0;
        PropDesc->MembersListCount  = 1;
        PropDesc->Reserved          = 0;

        // if return buffer can also hold a range description, return it too
        if(PropertyRequest->ValueSize >= cbFullProperty)
        {
            // fill in the members header
            PKSPROPERTY_MEMBERSHEADER Members = 
                PKSPROPERTY_MEMBERSHEADER(PropDesc + 1);

            Members->MembersFlags   = KSPROPERTY_MEMBER_STEPPEDRANGES;
            Members->MembersSize    = sizeof(KSPROPERTY_STEPPING_LONG);
            Members->MembersCount   = 1;
            Members->Flags          = KSPROPERTY_MEMBER_FLAG_BASICSUPPORT_MULTICHANNEL;

            // fill in the stepped range
            PKSPROPERTY_STEPPING_LONG Range = 
                PKSPROPERTY_STEPPING_LONG(Members + 1);

            Range->Bounds.SignedMaximum = 0x00000000;      //   0 dB
            Range->Bounds.SignedMinimum = -96 * 0x10000;   // -96 dB
            Range->SteppingDelta        = 0x08000;         //  .5 dB
            Range->Reserved             = 0;

            // set the return value size
            PropertyRequest->ValueSize = cbFullProperty;
        } 
        else
        {
            PropertyRequest->ValueSize = sizeof(KSPROPERTY_DESCRIPTION);
        }
    } 
    else if(PropertyRequest->ValueSize >= sizeof(ULONG))
    {
        // if return buffer can hold a ULONG, return the access flags
        PULONG AccessFlags = PULONG(PropertyRequest->Value);

        PropertyRequest->ValueSize = sizeof(ULONG);
        *AccessFlags = KSPROPERTY_TYPE_ALL;
    }
    else
    {
        PropertyRequest->ValueSize = 0;
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    return ntStatus;
} 
NTSTATUS
Topology::PropertyHandlerVolume(IN  PPCPROPERTY_REQUEST  PropertyRequest )
{

    DPT("[%s]",__FUNCTION__ );

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;
    LONG     lChannel;
    PULONG   pulVolume;

	//
	DPT("PropertyHandlerVolume Volume [ NODE=%d ].",PropertyRequest->Node);
	///只处理录音的音量
	if( KSNODE_TOPO_MIC_VOLUME != PropertyRequest->Node ){
	//	DPT("Only Process Record Volume [ NODE=%d ].",PropertyRequest->Node);
		return STATUS_NOT_IMPLEMENTED;
	}
	////
    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = PropertyHandlerBasicSupportVolume(PropertyRequest);
    }
    else
    {//return STATUS_NOT_IMPLEMENTED;
        ntStatus = 
            ValidatePropertyParams
            (
                PropertyRequest, 
                sizeof(ULONG),  // volume value is a ULONG
                sizeof(LONG)    // instance is the channel number
            );
        if (NT_SUCCESS(ntStatus))
        {
            lChannel = * (PLONG (PropertyRequest->Instance));
            pulVolume = PULONG (PropertyRequest->Value);

            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                *pulVolume = lChannel<MAX_TOPOLOGY_NODES?m_VolumeControls[lChannel]:0;
                    
                PropertyRequest->ValueSize = sizeof(ULONG);                
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                if( lChannel<MAX_TOPOLOGY_NODES)m_VolumeControls[lChannel]=*pulVolume;
				DPT("Topo -> SetVolume =%d, Channel=%d", *pulVolume, lChannel );
            }
        }
        else
        {
            DPT("[%s - ntStatus=0x%08x]",__FUNCTION__,ntStatus );
        }
    }

    return ntStatus;
} 

NTSTATUS                            
Topology::PropertyHandlerMuxSource( IN  PPCPROPERTY_REQUEST PropertyRequest )
{

    DPT("[%s]",__FUNCTION__ );

    NTSTATUS                    ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    //
    // Validate node
    // This property is only valid for WAVEIN_MUX node.
    //只处理录制的音量
//    if ( WAVEIN_MUX == PropertyRequest->Node)
    {
        if (PropertyRequest->ValueSize >= sizeof(ULONG))
        {
            PULONG pulMuxValue = PULONG(PropertyRequest->Value);
            
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                *pulMuxValue = m_ulMux ;
				////
                PropertyRequest->ValueSize = sizeof(ULONG);
                ntStatus = STATUS_SUCCESS;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                m_ulMux = *pulMuxValue ;

				DPT(" --- PropertyHandlerMuxSource Set Mux=%d", m_ulMux ); 

                ntStatus = STATUS_SUCCESS;
            }
            else if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
            {
                ntStatus = 
                    PropertyHandler_BasicSupport
                    ( 
                        PropertyRequest, 
                        KSPROPERTY_TYPE_ALL,
                        VT_I4
                    );
            }
        }
        else
        {
            DPT ("[PropertyHandlerMuxSource - Invalid parameter]" );
            ntStatus = STATUS_INVALID_PARAMETER;
        }
    }

    return ntStatus;
} 

NTSTATUS                            
Topology::PropertyHandlerGeneric (IN  PPCPROPERTY_REQUEST     PropertyRequest )
{

    NTSTATUS  ntStatus = STATUS_NOT_IMPLEMENTED; //STATUS_INVALID_DEVICE_REQUEST;

    switch (PropertyRequest->PropertyItem->Id)
    {
        case KSPROPERTY_AUDIO_VOLUMELEVEL:
            ntStatus = PropertyHandlerVolume(PropertyRequest);
			////
            break;
        
        case KSPROPERTY_AUDIO_MUX_SOURCE:
//            ntStatus = PropertyHandlerMuxSource(PropertyRequest);
            break;
/*
        case KSPROPERTY_AUDIO_CPU_RESOURCES:
            ntStatus = PropertyHandlerCpuResources(PropertyRequest);
            break;

        case KSPROPERTY_AUDIO_MUTE:
            ntStatus = PropertyHandlerMute(PropertyRequest);
            break;


        case KSPROPERTY_AUDIO_DEV_SPECIFIC:
            ntStatus = PropertyHandlerDevSpecific(PropertyRequest);
            break;
*/
        default:
            DPT("[Topology::PropertyHandlerGeneric: Invalid Device Request]" );
    }

    return ntStatus;
}

static NTSTATUS
PropertyHandler_TopoFilter( IN PPCPROPERTY_REQUEST PropertyRequest)
{
	NTSTATUS status = STATUS_NOT_IMPLEMENTED;
	DPT("CCC PropertyHandler_TopoFilter.");
	return status; 
}
static NTSTATUS
PropertyHandler_Topology( IN PPCPROPERTY_REQUEST PropertyRequest )
{
//	NTSTATUS status = STATUS_NOT_IMPLEMENTED;
//	DPT("CCC PropertyHandler_Topology.");

	return ((Topology*)
        (PropertyRequest->MajorTarget))->PropertyHandlerGeneric( PropertyRequest );
	////
}

