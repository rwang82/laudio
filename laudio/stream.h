// By Fanxiushu 2011-10-25

#pragma once 


#include "top_wav.h"

class WaveStream:
     public IMiniportWaveCyclicStream,
     public IDmaChannel,
	 public CUnknown
{
public:
	///
	DECLARE_STD_UNKNOWN();
	WaveStream( PUNKNOWN pUnknownOuter);
	~WaveStream();
	/////
	IMP_IMiniportWaveCyclicStream;
    IMP_IDmaChannel;
	//////

public:
	WaveCyclic* m_pOwner; 
	BOOLEAN m_fCapture; 
	ULONG m_Pin; 
	NTSTATUS Init(WaveCyclic* owner, ULONG Pin, BOOLEAN Capture, PKSDATAFORMAT DataFormat); 

protected:
	/////
	USHORT                     m_nChannel;                           // 1 or 2 
	SHORT                      m_wBitsPerSample;                     // 16- or 8-bit samples.
	ULONG                      m_nSamplesPerSec;                      // 44100 or 48000
    ULONG                      m_nBlockAlign;                        // Block alignment of current format.

	BOOLEAN                     m_fDmaActive;                       // Dma currently active? 
    ULONG                       m_ulDmaPosition;                    // Position in Dma
    ULONG                       m_ulDmaBufferSize;                  // Size of dma buffer
    ULONG                       m_ulDmaMovementRate;                // Rate of transfer specific to system
    ULONGLONG                   m_ullDmaTimeStamp;                  // Dma time elasped 
    ULONGLONG                   m_ullElapsedTimeCarryForward;       // Time to carry forward in position calc.
    ULONG                       m_ulByteDisplacementCarryForward;   // Bytes to carry forward to next calc.
	//////
	KSSTATE                     m_ksState;

	PRKDPC                      m_pDpc;                             // Deferred procedure call object
    PKTIMER                     m_pTimer;                           // Timer object
	/////
};


