///By Fanxiushu 2011-1024

#include "top_wav.h"
#include "llx_audfet_cdo.h"

static NTSTATUS PropertyHandler_WaveFilter(IN PPCPROPERTY_REQUEST PropertyRequest);

#include "wavtable.h"
#include "stream.h"

WaveCyclic::WaveCyclic
      (PUNKNOWN pUnknownOuter):CUnknown( pUnknownOuter )
{
	////
	m_wavedesc = NULL; 
	m_fCaptureAllocated = m_fRenderAllocated = FALSE; 
	m_servicegroup = NULL; 
	m_hDataTrans = NULL; 
	m_Port = NULL; 
	m_NotificationInterval = 0; 
	m_SamplingFrequency = 0; 
}

WaveCyclic::~WaveCyclic()
{
	DPT("WaveCyclic::~WaveCyclic()");
	/////
	SAFE_RELEASE( m_servicegroup );
	SAFE_RELEASE( m_Port ); 
	
	data_transfer_destroy( m_hDataTrans ); m_hDataTrans = NULL; 

	//
	DestroyCDO();

}

STDMETHODIMP
WaveCyclic::NonDelegatingQueryInterface( REFIID  Interface, PVOID * Object )
{
	if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN((WaveCyclic*)(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveCyclic))
    {
        *Object = PVOID((IMiniportWaveCyclic*)(this));
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


/// IMiniportWaveCyclic
STDMETHODIMP_(NTSTATUS)
WaveCyclic::DataRangeIntersection
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
	DPT("WaveCyclic::DataRangeIntersection");
	return status;
}

STDMETHODIMP_(NTSTATUS)
WaveCyclic::GetDescription
( 
    OUT PPCFILTER_DESCRIPTOR * OutFilterDescriptor 
)
{
	NTSTATUS status = STATUS_SUCCESS;
	DPT("WaveCyclic::GetDescription");
	*OutFilterDescriptor = m_wavedesc;

	return status;
}

STDMETHODIMP_(NTSTATUS)
WaveCyclic::Init( IN  PUNKNOWN  UnknownAdapter_, 
		IN  PRESOURCELIST ResourceList_, IN  PPORTWAVECYCLIC  Port_ )
{
	NTSTATUS status = STATUS_SUCCESS;
	DPT("WaveCyclic::Init");

	m_wavedesc = &MiniportFilterDescriptor ; 

	/////创建数据转发
	m_hDataTrans = data_transfer_create(); 
	if(!m_hDataTrans) {
		DPT("XX [data_transfer_create] error.");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	////
	status = PcNewServiceGroup(&m_servicegroup, NULL);
	if( !NT_SUCCESS( status)){
		DPT("PcNewServiceGroup Error.");
		return status;
	}
	//////
	m_Port = Port_;
	m_Port->AddRef(); 


	return status;
}

STDMETHODIMP_(NTSTATUS)
WaveCyclic::NewStream
( 
    OUT PMINIPORTWAVECYCLICSTREAM * OutStream,
    IN  PUNKNOWN                OuterUnknown,
    IN  POOL_TYPE               PoolType,
    IN  ULONG                   Pin,
    IN  BOOLEAN                 Capture,
    IN  PKSDATAFORMAT           DataFormat,
    OUT PDMACHANNEL *           OutDmaChannel,
    OUT PSERVICEGROUP *         OutServiceGroup 
)
{
	NTSTATUS status = STATUS_SUCCESS;
	DPT("WaveCyclic::NewStream");
	/////
	if( Capture ){
		if( m_fCaptureAllocated ) {
			DPT("Only Alloc One Capture Stream "); 
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
	}else{
		if( m_fRenderAllocated ){
			DPT("Only Alloc One Render Stream");
			status = STATUS_INSUFFICIENT_RESOURCES; 
		}
	}
	if( !NT_SUCCESS( status )) return status; 
	//////

	//check Format
	status = ValidateFormat( DataFormat) ; 
	if( !NT_SUCCESS(status)) { 
		DPT("[WaveCyclic::NewStream] Invalid Format ");
		return status; 
	}
	//////

	WaveStream* stream = NULL;
	stream = new (NonPagedPool, VA_POOLTAG) WaveStream( OuterUnknown ); 
	if(stream ){
		stream->AddRef(); 
		status = stream->Init( this, Pin, Capture, DataFormat ); 
	}
	else 
		status = STATUS_INSUFFICIENT_RESOURCES;
	if( !NT_SUCCESS( status )) {
		DPT("Init Stream for [%s] error", Capture?"Capture":"Render" ); 
		SAFE_RELEASE( stream ); 
		return status;
	}
	////

	if( Capture ){
		m_fCaptureAllocated = TRUE; 
	}
	else {
		m_fRenderAllocated = TRUE;
	}
	////////////

	(*OutStream) = (PMINIPORTWAVECYCLICSTREAM)stream;
	(*OutStream)->AddRef();

	*OutDmaChannel = (PDMACHANNEL)stream; 
	(*OutDmaChannel)->AddRef(); 

	*OutServiceGroup = m_servicegroup;
	m_servicegroup->AddRef(); 

	///
	SAFE_RELEASE( stream ); 
	//////
	return status;
}


//// Property

//copy from MSVAD
//=============================================================================
NTSTATUS
WaveCyclic::PropertyHandlerComponentId
(
    IN PPCPROPERTY_REQUEST      PropertyRequest
)
/*++

Routine Description:

  Handles KSPROPERTY_GENERAL_COMPONENTID

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{

 //   DPT("[PropertyHandlerComponentId]");

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

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
        ntStatus = 
            ValidatePropertyParams
            (
                PropertyRequest, 
                sizeof(KSCOMPONENTID), 
                0
            );
        if (NT_SUCCESS(ntStatus))
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
            {
                PKSCOMPONENTID pComponentId = (PKSCOMPONENTID)
                    PropertyRequest->Value;

                INIT_MMREG_MID(&pComponentId->Manufacturer, MM_MICROSOFT);
                pComponentId->Product   = PID_ISVA;
                pComponentId->Name      = NAME_ISVA;
                pComponentId->Component = GUID_NULL; // Not used for extended caps.
                pComponentId->Version   = ISVA_VERSION;
                pComponentId->Revision  = ISVA_REVISION;

                PropertyRequest->ValueSize = sizeof(KSCOMPONENTID);
                ntStatus = STATUS_SUCCESS;
            }
        }
        else
        {
            DPT("[PropertyHandlerComponentId - Invalid parameter]");
            ntStatus = STATUS_INVALID_PARAMETER;
        }
    }

    return ntStatus;
} // PropertyHandlerComponentId

#define CB_EXTENSIBLE (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))

//=============================================================================
NTSTATUS
WaveCyclic::PropertyHandlerProposedFormat
(
    IN PPCPROPERTY_REQUEST      PropertyRequest
)
/*++

Routine Description:

  Handles KSPROPERTY_GENERAL_COMPONENTID

Arguments:

  PropertyRequest - 

Return Value:

  NT status code.

--*/
{

//    DPT("[PropertyHandlerProposedFormat]");

    NTSTATUS ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        ntStatus = 
            PropertyHandler_BasicSupport
            (
                PropertyRequest,
                KSPROPERTY_TYPE_BASICSUPPORT | KSPROPERTY_TYPE_SET,
                VT_ILLEGAL
            );
    }
    else
    {
        ULONG cbMinSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);

        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = cbMinSize;
            ntStatus = STATUS_BUFFER_OVERFLOW;
        }
        else if (PropertyRequest->ValueSize < cbMinSize)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
            {
                KSDATAFORMAT_WAVEFORMATEX* pKsFormat = (KSDATAFORMAT_WAVEFORMATEX*)PropertyRequest->Value;

                ntStatus = STATUS_NO_MATCH;

                if ((pKsFormat->DataFormat.MajorFormat == KSDATAFORMAT_TYPE_AUDIO) &&
                    (pKsFormat->DataFormat.SubFormat == KSDATAFORMAT_SUBTYPE_PCM) &&
                    (pKsFormat->DataFormat.Specifier == KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
                {
                    WAVEFORMATEX* pWfx = (WAVEFORMATEX*)&pKsFormat->WaveFormatEx;

                    // make sure the WAVEFORMATEX part of the format makes sense
                    if ((pWfx->wBitsPerSample == 16) &&
                        (/*(pWfx->nSamplesPerSec == 44100) ||*/ (pWfx->nSamplesPerSec == 48000)) &&
                        (pWfx->nBlockAlign == (pWfx->nChannels * 2)) &&
                        (pWfx->nAvgBytesPerSec == (pWfx->nSamplesPerSec * pWfx->nBlockAlign)))
                    {
                        if ((pWfx->wFormatTag == WAVE_FORMAT_PCM) && (pWfx->cbSize == 0))
                        {
                            if (pWfx->nChannels == 2)
                            {
                                ntStatus = STATUS_SUCCESS;
                            }
                        }
                        else 
                        if ((pWfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) && (pWfx->cbSize == CB_EXTENSIBLE))
                        {
                            WAVEFORMATEXTENSIBLE* pWfxT = (WAVEFORMATEXTENSIBLE*)pWfx;

                            if (((pWfx->nChannels == 2) && (pWfxT->dwChannelMask == KSAUDIO_SPEAKER_STEREO))/* ||
                                ((pWfx->nChannels == 6) && (pWfxT->dwChannelMask == KSAUDIO_SPEAKER_5POINT1_SURROUND)) ||
                                ((pWfx->nChannels == 8) && (pWfxT->dwChannelMask == KSAUDIO_SPEAKER_7POINT1_SURROUND))*/)
                            {
                                ntStatus = STATUS_SUCCESS;
                            }
                        }
                    }
                }
            }
            else
            {
                ntStatus = STATUS_INVALID_PARAMETER;
            }
        }
    }

    return ntStatus;
} // PropertyHandlerProposedFormat

static NTSTATUS 
PropertyHandler_WaveFilter(IN PPCPROPERTY_REQUEST PropertyRequest)
{

    NTSTATUS   ntStatus = STATUS_INVALID_DEVICE_REQUEST;

    WaveCyclic*   pWave = (WaveCyclic*) PropertyRequest->MajorTarget;

    switch (PropertyRequest->PropertyItem->Id)
    {
        case KSPROPERTY_GENERAL_COMPONENTID:
            ntStatus = 
                pWave->PropertyHandlerComponentId
                (
                    PropertyRequest
                );
            break;

        case KSPROPERTY_PIN_PROPOSEDATAFORMAT:
            ntStatus = 
                pWave->PropertyHandlerProposedFormat
                (
                    PropertyRequest
                );
            break;
        
        default:
            DPT("[PropertyHandler_WaveFilter: Invalid Device Request]" );
    }

    return ntStatus;
}

