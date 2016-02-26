//By Fanxiushu 2011-20-24

#include "stream.h"
#include "llx_audfet_cdo.h"
#include "llx_audio_data_cache.h"
#include "ShowBuffer.h"
#include "dk_micdata_monitor.h"

WaveStream::WaveStream
      (PUNKNOWN pUnknownOuter):CUnknown( pUnknownOuter )
{
	////
	m_pOwner = NULL;
	m_fCapture = FALSE; 
	/////

	m_fDmaActive = FALSE;
    m_ulDmaPosition = 0;
    m_ullElapsedTimeCarryForward = 0;
    m_ulByteDisplacementCarryForward = 0;
    m_ulDmaBufferSize = DMA_BUFFER_SIZE ;
    m_ulDmaMovementRate = 0;
    m_ullDmaTimeStamp = 0;
	////
	m_ksState = KSSTATE_STOP ; 

	m_pDpc = NULL; 
	m_pTimer = NULL; 

	////
	
}

WaveStream::~WaveStream()
{
	DPT( "WaveStream::~WaveStream <%s>", m_fCapture?"Capture":"Render");
	/////

	if( m_pTimer){
		KeCancelTimer( m_pTimer ); 
		ExFreePoolWithTag( m_pTimer, VA_POOLTAG ); 
	}
	if( m_pDpc){
		ExFreePoolWithTag( m_pDpc ,VA_POOLTAG );
	}
	///////

	//////
	if( this->m_fCapture) {
		m_pOwner->m_fCaptureAllocated = FALSE; 
	}
	else{
		m_pOwner->m_fRenderAllocated = FALSE; 
	}
}

STDMETHODIMP
WaveStream::NonDelegatingQueryInterface( REFIID  Interface, PVOID * Object )
{
	if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN((IMiniportWaveCyclicStream*)(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveCyclicStream))
    {
        *Object = PVOID((IMiniportWaveCyclicStream*)(this));
    }
	else if (IsEqualGUIDAligned(Interface, IID_IDmaChannel))
    {
        *Object = PVOID((IDmaChannel*)(this));
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
////////////


//DMA
STDMETHODIMP_(NTSTATUS)
WaveStream::AllocateBuffer(IN ULONG  BufferSize,IN PPHYSICAL_ADDRESS  PhysicalAddressConstraint OPTIONAL )
{
	DPT("IDmaChannel::AllocBuffer size=%d",BufferSize );
	return STATUS_SUCCESS;
}
STDMETHODIMP_(ULONG)
WaveStream::AllocatedBufferSize(void)
{
//	DPT("IDmaChannel::AllocatedBufferSize. ");
	return DMA_BUFFER_SIZE;
}
STDMETHODIMP_(ULONG)
WaveStream::BufferSize( void)
{
//	DPT("IDmaChannel::BufferSize");
	return DMA_BUFFER_SIZE; 
}

extern PDEVICE_OBJECT g_pCDO;

STDMETHODIMP_(void)
WaveStream::CopyFrom
(
    IN  PVOID                   Destination,
    IN  PVOID                   Source,
    IN  ULONG                   ByteCount
)
{
	////
	//DPT("IDmaChannel::CopyFrom");

	LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;

	//
	dk_set_start_micdata_flag();

	//
    pCDODevExt->m_pBDC4MicData->lock();
	if ( Destination ) {
		if ( ByteCount <= pCDODevExt->m_pBDC4MicData->getDataLen() ) {
			unsigned int uSizePop = pCDODevExt->m_pBDC4MicData->popBuf( (unsigned char*)Destination, ByteCount );
			DPT( "CopyFrom(PushMicData): ByteCount:%d, uSizePop=%d\n", ByteCount, uSizePop );
			if ( uSizePop != ByteCount ) {
				DPT( "uSizePop != ByteCount\n" );
			}
		} else {
            DPT( "warnning: ByteCount:%d > pCDODevExt->m_pBDC4MicData->getDataLen()\n", ByteCount, pCDODevExt->m_pBDC4MicData->getDataLen() );
		}
	}
    pCDODevExt->m_pBDC4MicData->unlock();

	// data_transfer_record( this->m_pOwner->m_hDataTrans, Destination, ByteCount ); 
	///
}

STDMETHODIMP_(void)
WaveStream::CopyTo
(
    IN  PVOID                   Destination,
    IN  PVOID                   Source,
    IN  ULONG                   ByteCount
)
{
	//DPT("IDmaChannel::CopyTo");
	//
    if ( g_pCDO ) {
       LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;
	   if ( pCDODevExt ) {
           //ShowBuffer( Source, 24 );
		   audio_data_save( pCDODevExt->m_hADC, Source, ByteCount );
	   }
	}
	////
	//  ( this->m_pOwner->m_hDataTrans, Source, ByteCount ); 
}


STDMETHODIMP_(void)
WaveStream::FreeBuffer( void )
{
	DPT("IDmaChannel::FreeBuffer");
}
STDMETHODIMP_(PADAPTER_OBJECT)
WaveStream::GetAdapterObject(void)
{
//	DPT("IDmaChannel::GetAdapterObject.");
	return NULL;
}
STDMETHODIMP_(ULONG)
WaveStream::MaximumBufferSize( void)
{
//	DPT("IDmaChannel::MaximumBufferSize ");
	return DMA_BUFFER_SIZE; 
}
STDMETHODIMP_(PHYSICAL_ADDRESS)
WaveStream::PhysicalAddress( void)
{
//	DPT("IDmaChannel::PhysicalAddress");

	PHYSICAL_ADDRESS addr; 
	addr.QuadPart = (LONGLONG) this->m_pOwner->m_hDataTrans; 
	///
	return addr; 
}
STDMETHODIMP_(void)
WaveStream::SetBufferSize( IN ULONG   BufferSize )
{
	DPT("IDmaChannel::SetBufferSize Size=%d", BufferSize);
}
STDMETHODIMP_(PVOID)
WaveStream::SystemAddress( void)
{
//	DPT("IDmaChannel::SystemAddress");
	return this->m_pOwner->m_hDataTrans; 
}
STDMETHODIMP_(ULONG)
WaveStream::TransferCount( void )
{
//	DPT("IDmaChannel::TransferCount");
	return DMA_BUFFER_SIZE; 
}



void
TimerNotify
(
    IN  PKDPC                   Dpc,
    IN  PVOID                   DeferredContext,
    IN  PVOID                   SA1,
    IN  PVOID                   SA2
)
{
	////
	WaveCyclic* owner = (WaveCyclic*)DeferredContext;
	//////
	if( owner && owner->m_Port ){
		owner->m_Port->Notify( owner->m_servicegroup ); 
	}
	///////////////
}
////// IMiniportWaveCyclicStream 
NTSTATUS WaveStream::Init(WaveCyclic* owner, ULONG Pin, BOOLEAN Capture, PKSDATAFORMAT DataFormat)
{
	this->m_pOwner = owner; 
	this->m_fCapture = Capture; 
	this->m_Pin = Pin; 
	/////
//	return STATUS_INVALID_DEVICE_REQUEST;

	NTSTATUS status = SetFormat( DataFormat ); 
	if( !NT_SUCCESS( status ) ){
		return status; 
	}
	//////
	m_pDpc = (PRKDPC)ExAllocatePoolWithTag( NonPagedPool, sizeof(KDPC), VA_POOLTAG );
	m_pTimer = (PKTIMER)ExAllocatePoolWithTag( NonPagedPool, sizeof(KTIMER), VA_POOLTAG );
	if( !m_pDpc || !m_pTimer ){
		DPT("ExAllocatePoolWithTag error no Memory. ");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	////Init Timer 
	KeInitializeDpc( m_pDpc, TimerNotify, m_pOwner ); 
	KeInitializeTimerEx(m_pTimer, NotificationTimer);
	
	return STATUS_SUCCESS;

}

///

STDMETHODIMP
WaveStream::GetPosition(OUT PULONG  Position )
{
    if (m_fDmaActive)
    {
        // Get the current time
        //
        ULONGLONG CurrentTime = KeQueryInterruptTime();

        // Calculate the time elapsed since the last call to GetPosition() or since the
        // DMA engine started.  Note that the division by 10000 to convert to milliseconds
        // may cause us to lose some of the time, so we will carry the remainder forward 
        // to the next GetPosition() call.
        //
        ULONG TimeElapsedInMS =
            ( (ULONG) (CurrentTime - m_ullDmaTimeStamp + m_ullElapsedTimeCarryForward) ) / 10000;

        // Carry forward the remainder of this division so we don't fall behind with our position.
        //
        m_ullElapsedTimeCarryForward = 
            (CurrentTime - m_ullDmaTimeStamp + m_ullElapsedTimeCarryForward) % 10000;

        // Calculate how many bytes in the DMA buffer would have been processed in the elapsed
        // time.  Note that the division by 1000 to convert to milliseconds may cause us to 
        // lose some bytes, so we will carry the remainder forward to the next GetPosition() call.
        //
        ULONG ByteDisplacement =
            ((m_ulDmaMovementRate * TimeElapsedInMS) + m_ulByteDisplacementCarryForward) / 1000;

        // Carry forward the remainder of this division so we don't fall behind with our position.
        //
        m_ulByteDisplacementCarryForward = (
            (m_ulDmaMovementRate * TimeElapsedInMS) + m_ulByteDisplacementCarryForward) % 1000;

        // Increment the DMA position by the number of bytes displaced since the last
        // call to GetPosition() and ensure we properly wrap at buffer length.
        //
        m_ulDmaPosition =
            (m_ulDmaPosition + ByteDisplacement) % m_ulDmaBufferSize;

        // Return the new DMA position
        //
        *Position = m_ulDmaPosition;

        // Update the DMA time stamp for the next call to GetPosition()
        //
        m_ullDmaTimeStamp = CurrentTime;
    }
    else
    {
        // DMA is inactive so just return the current DMA position.
        //
        *Position = m_ulDmaPosition;
    }

    return STATUS_SUCCESS;
}

STDMETHODIMP
WaveStream::NormalizePhysicalPosition(IN OUT PLONGLONG  PhysicalPosition)
{
	*PhysicalPosition =
        ( _100NS_UNITS_PER_SECOND / m_nBlockAlign * *PhysicalPosition ) /
        m_pOwner->m_SamplingFrequency;

	return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
WaveStream::SetFormat( IN  PKSDATAFORMAT  Format )
{
    PWAVEFORMATEX               pWfx;
	////
	if( m_ksState == KSSTATE_RUN ) return STATUS_INVALID_DEVICE_REQUEST;

	pWfx = GetWaveFormatEx( Format ); 
	if(!pWfx ){
		DPT("Invalid WaveFormate.");
		return STATUS_INVALID_PARAMETER;
	}
    if( pWfx->nChannels!=2 || pWfx->wBitsPerSample!=16 || pWfx->nSamplesPerSec !=48000)
		return STATUS_INVALID_DEVICE_REQUEST;
	////////

	this->m_nChannel = pWfx->nChannels;
	this->m_nSamplesPerSec = pWfx->nSamplesPerSec;
	this->m_wBitsPerSample = pWfx->wBitsPerSample; 
	this->m_nBlockAlign = pWfx->nBlockAlign ;
	this->m_ulDmaMovementRate = pWfx->nAvgBytesPerSec;
	
	this->m_pOwner->m_SamplingFrequency = this->m_nSamplesPerSec; 

	/////
	DPT(" [Stream->SetFormat ] <%s> Channel=%d, Simple=%d, Bits=%d", 
		m_fCapture?"Capture":"Render",pWfx->nChannels, pWfx->nSamplesPerSec, pWfx->wBitsPerSample ); 


	return STATUS_SUCCESS;
}

STDMETHODIMP_(ULONG)
WaveStream::SetNotificationFreq(IN  ULONG  Interval, OUT PULONG  FramingSize )
{
	DPT("IMiniportWaveCyclicStream::SetNotificationFreq Interval=%d", Interval );

	m_pOwner->m_NotificationInterval = Interval;
	///
	*FramingSize =
        m_nBlockAlign *
        m_pOwner->m_SamplingFrequency *
        Interval / 1000;

	return Interval;
}

STDMETHODIMP
WaveStream::SetState( IN  KSSTATE  NewState)
{
    DPT("IMiniportWaveCyclicStream::SetState.");
	///
    NTSTATUS                    ntStatus = STATUS_SUCCESS;

    // The acquire state is not distinguishable from the stop state for our
    // purposes.
    //
    if (NewState == KSSTATE_ACQUIRE)
    {
        NewState = KSSTATE_STOP;
    }
	////
	if( m_ksState == NewState ) return STATUS_SUCCESS; 
	
	////
	switch(NewState)
	{
	case KSSTATE_PAUSE:
		{
			DPT ("KSSTATE_PAUSE" );
			
			m_fDmaActive = FALSE;
		}
		break;
		
	case KSSTATE_RUN:
		{
			DPT("KSSTATE_RUN Interval=%d", m_pOwner->m_NotificationInterval );
			
			LARGE_INTEGER   delay;
			
			// Set the timer for DPC.
			//
			m_ullDmaTimeStamp             = KeQueryInterruptTime();
			m_ullElapsedTimeCarryForward  = 0;
			m_fDmaActive                  = TRUE;
			delay.HighPart                = 0;
			delay.LowPart                 = m_pOwner->m_NotificationInterval;
			
			KeSetTimerEx (
			m_pTimer,
			delay,
			m_pOwner->m_NotificationInterval,
			m_pDpc
			);
		}
		break;
		
	case KSSTATE_STOP:
		
		DPT ("KSSTATE_STOP" );
		
		m_fDmaActive                      = FALSE;
		m_ulDmaPosition                   = 0;
		m_ullElapsedTimeCarryForward      = 0;
		m_ulByteDisplacementCarryForward  = 0;
		
		KeCancelTimer( m_pTimer );
		
		break;
	}
	
	m_ksState = NewState;
	
    return ntStatus;
}

STDMETHODIMP_(void)
WaveStream::Silence( __out_bcount(ByteCount) PVOID Buffer, IN ULONG  ByteCount )
{
	RtlFillMemory(Buffer, ByteCount, (m_wBitsPerSample==16) ? 0 : 0x80);
}

////////////

