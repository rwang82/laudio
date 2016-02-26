#ifndef __LLX_AUDFET_CDO_H__
#define __LLX_AUDFET_CDO_H__
#include <Ntifs.h>
#include "llx_block_data_cache.h"
#include "llx_ring_cache.h"
//
enum ENUMDKMICDATA_STATE {
	EDKMICDATA_STATE_SLEEP = 0,
	EDKMICDATA_STATE_NEEDDATA = 1
};
//
struct DKMicDataState {
public:
	void init();
	void lock();
	void unlock();
	void setStartFlag();
	void setStopFlag();
	ENUMDKMICDATA_STATE getState() { return m_eDKMicDataState; }
	bool isStateChanged();
	void resetStateChangedFlag();
	void getTimeTagLastStartFlag(PLARGE_INTEGER plIntTimeTag);

private:
	KSPIN_LOCK m_SpinLock;
	KIRQL m_SpinIrql;
	ENUMDKMICDATA_STATE m_eDKMicDataState;
	bool m_bStateChanged;
	LARGE_INTEGER m_timeTagLastStartFlag;
};
//
struct LLXAFCDO_DEVICE_EXTENSION {
    void* m_hADC;
//	llx_block_data_cache* m_pBDC4MicData;
    llx_ring_cache* m_pBDC4MicData;
	UNICODE_STRING m_ustrSymLinkName;
	HANDLE m_hEventMicNeedData;
	HANDLE m_hEventMicSleep;
	DKMicDataState m_stateMicData;
};

#pragma PAGEDCODE
NTSTATUS CreateCDO( IN PDRIVER_OBJECT DriverObject );

#pragma PAGECODE
NTSTATUS DestroyCDO();

#pragma PAGEDCODE
void HookMJFunction( PDRIVER_OBJECT DriverObject );

#endif //__LLX_AUDFET_CDO_H__




