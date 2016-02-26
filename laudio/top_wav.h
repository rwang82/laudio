/// By Fanxiushu 2011-10-24   

#pragma once 

#include "common.h"

///////
class WaveCyclic:
     public IMiniportWaveCyclic,
     public CUnknown
{
public:
	BOOLEAN m_fCaptureAllocated; //同时只能打开一个输入输出流
	BOOLEAN m_fRenderAllocated;
public:
	PPORTWAVECYCLIC             m_Port; // callback interface 
	PPCFILTER_DESCRIPTOR        m_wavedesc; // Filter descriptor.
	PSERVICEGROUP               m_servicegroup; 
	PVOID                       m_hDataTrans; //用来从播放转发数据到录音

	ULONG                       m_NotificationInterval; // milliseconds.
    ULONG                       m_SamplingFrequency;    // Frames per second. (nSamplesPerSec)

public:
	////
	DECLARE_STD_UNKNOWN();
	WaveCyclic( PUNKNOWN pUnknownOuter);
	~WaveCyclic();
	//////
	IMP_IMiniportWaveCyclic;
	/////////////
public:
	/////
	NTSTATUS PropertyHandlerComponentId( IN PPCPROPERTY_REQUEST PropertyRequest );
	NTSTATUS PropertyHandlerProposedFormat( IN PPCPROPERTY_REQUEST  PropertyRequest);
};

///

#define MAX_TOPOLOGY_NODES    20

class Topology:
    public IMiniportTopology,
	public CUnknown
{
public:
	PPCFILTER_DESCRIPTOR        m_topodesc; // Filter descriptor.
public:
	/////
	DECLARE_STD_UNKNOWN();
	Topology( PUNKNOWN pUnknownOuter);
	~Topology();
	//////
	IMP_IMiniportTopology;

protected:
	ULONG m_ulMux; 
	BOOL  m_MuteControls[MAX_TOPOLOGY_NODES];
    LONG  m_VolumeControls[MAX_TOPOLOGY_NODES];
public:
	NTSTATUS  PropertyHandlerGeneric (IN  PPCPROPERTY_REQUEST     PropertyRequest );
	NTSTATUS  PropertyHandlerBasicSupportVolume( IN  PPCPROPERTY_REQUEST     PropertyRequest);
	NTSTATUS  PropertyHandlerVolume(IN  PPCPROPERTY_REQUEST  PropertyRequest );
	NTSTATUS  PropertyHandlerMuxSource( IN  PPCPROPERTY_REQUEST PropertyRequest );
};

