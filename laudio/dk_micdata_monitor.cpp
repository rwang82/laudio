#include "dk_micdata_monitor.h"
#include "common.h"
//
HANDLE g_hThreadMicDataMonitor = NULL;
HANDLE g_hEventExitMDMT = NULL;
KSTART_ROUTINE ThreadProcMicDataMonitor;
extern PDEVICE_OBJECT g_pCDO;
//
#define DELAY_ONE_MICROSECOND 	(-10)
#define DELAY_ONE_MILLISECOND	(DELAY_ONE_MICROSECOND*1000)
VOID DKSleep(LONG msec)
{
	LARGE_INTEGER my_interval;
	my_interval.QuadPart = DELAY_ONE_MILLISECOND;
	my_interval.QuadPart *= msec;
	KeDelayExecutionThread(KernelMode, 0, &my_interval);
}
//
bool dk_start_micdata_monitor() {
	if ( g_hThreadMicDataMonitor) {
		DPT( "g_hThreadMicDataMonitor != NULL." );
		return false;
	}
	
	if (STATUS_SUCCESS != ::ZwCreateEvent(&g_hEventExitMDMT, EVENT_ALL_ACCESS, NULL, NotificationEvent, FALSE ) )
		return false;
	if (NULL == g_hEventExitMDMT)
		return false;
	if (STATUS_SUCCESS != PsCreateSystemThread(&g_hThreadMicDataMonitor, THREAD_ALL_ACCESS, NULL, NULL, NULL, ThreadProcMicDataMonitor, &g_hEventExitMDMT))
		return false;
	return g_hThreadMicDataMonitor != NULL;
}

void dk_stop_micdata_monitor() {
	if (!g_hThreadMicDataMonitor) {
		DPT( "g_hThreadMicDataMonitor == NULL" );
		return;
	}
	LONG preState = 0;
	if (g_hEventExitMDMT) {
		::ZwSetEvent(g_hEventExitMDMT, &preState);
	}

	// if need wait thread exit ?
	g_hThreadMicDataMonitor = NULL;
}

void dk_set_start_micdata_flag() {
	if (!g_pCDO) {
		return;
	}
	LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;
	if (!pCDODevExt) {
		return;
	}
	DKMicDataState* pMicDataState = &pCDODevExt->m_stateMicData;
	if (!pMicDataState) {
		return;
	}
	//
	pMicDataState->lock();
	pMicDataState->setStartFlag();
	pMicDataState->unlock();
}

void dk_set_stop_micdata_flag() {
	if (!g_pCDO) {
		return;
	}
	LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;
	if (!pCDODevExt) {
		return;
	}
	DKMicDataState* pMicDataState = &pCDODevExt->m_stateMicData;
	if (!pMicDataState) {
		return;
	}
	//
	pMicDataState->lock();
	pMicDataState->setStopFlag();
	pMicDataState->unlock();
}

void checkMicStateChange(DKMicDataState* pMicDataState, bool* pbChange, ENUMDKMICDATA_STATE* peCurState);


void TheadExitRomDebug() {
	int a = 0;
	int b = a + 10;

}

VOID ThreadProcMicDataMonitor(_In_ PVOID StartContext)
{
	LARGE_INTEGER lInt;
	NTSTATUS ntstateWait;
	HANDLE hEventExitMDMT = *((HANDLE*)StartContext);
	if (!g_pCDO) {
		DPT( "!g_pCDO in ThreadProcMicDataMonitor, so exit thread." );
		return;
	}
	LLXAFCDO_DEVICE_EXTENSION* pCDODevExt = (LLXAFCDO_DEVICE_EXTENSION*)g_pCDO->DeviceExtension;
	if (!pCDODevExt) {
		DPT("!pCDODevExt in ThreadProcMicDataMonitor, so exit thread.");
		return;
	}
	DKMicDataState* pMicDataState = &pCDODevExt->m_stateMicData;
	if (!pMicDataState) {
		DPT("!pMicDataState in ThreadProcMicDataMonitor, so exit thread.");
		return;
	}
	ENUMDKMICDATA_STATE eDKMicDataState;
	bool bStateChanged = false;

	lInt.QuadPart = 10 * DELAY_ONE_MILLISECOND;
	while (STATUS_SUCCESS != ZwWaitForSingleObject(hEventExitMDMT, FALSE, &lInt))
	{
		bStateChanged = false;
		checkMicStateChange(pMicDataState, &bStateChanged, &eDKMicDataState);

		//
		if (bStateChanged) {
			if (eDKMicDataState == EDKMICDATA_STATE_NEEDDATA) {
				LONG preState = 0;
				//::KeSetEvent( &(pCDODevExt->m_hKEventMicNeedData), IO_NO_INCREMENT, FALSE );
				if (pCDODevExt->m_hEventMicNeedData) {
					::ZwSetEvent(pCDODevExt->m_hEventMicNeedData, &preState);
				}
				DPT( "ZwSetEvent m_hEventStartMicData." );
			}
			else if (eDKMicDataState == EDKMICDATA_STATE_SLEEP) {
				LONG preState = 0;
				//::KeSetEvent(&(pCDODevExt->m_hKEventMicSleep), IO_NO_INCREMENT, FALSE);
				if (pCDODevExt->m_hEventMicSleep) {
					::ZwSetEvent(pCDODevExt->m_hEventMicSleep, &preState);
				}
				DPT("ZwSetEvent m_hEventStopMicData.");
			}
		}
	}

	TheadExitRomDebug();
}

void checkMicStateChange(DKMicDataState* pMicDataState, bool* pbChange, ENUMDKMICDATA_STATE* peCurState) {
	if (!pMicDataState || !pbChange || !peCurState)
		return;
	LARGE_INTEGER timeTagLastStartFlag;
	LARGE_INTEGER timeTagCur;
	LONGLONG lTimeOut = 300 * 1000 * 10; // 500 mm.

	pMicDataState->lock();
	//
	pMicDataState->getTimeTagLastStartFlag(&timeTagLastStartFlag);
	KeQuerySystemTime(&timeTagCur);
	if (timeTagCur.QuadPart - timeTagLastStartFlag.QuadPart > lTimeOut) {
		pMicDataState->setStopFlag();
	}

	//
	*pbChange = pMicDataState->isStateChanged();
	if (*pbChange) {
		pMicDataState->resetStateChangedFlag();
		*peCurState = pMicDataState->getState();
	}
	pMicDataState->unlock();
}