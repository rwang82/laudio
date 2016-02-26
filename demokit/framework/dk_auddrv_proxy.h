#ifndef __DKAUDDRVPROXY_H__
#define __DKAUDDRVPROXY_H__
#include "dk_ts_helper.h"
//
enum ENUMDKMICDATA_STATE {
	EDKMICDATA_STATE_SLEEP = 0,
	EDKMICDATA_STATE_NEEDDATA = 1
};
//
class dk_auddrv_proxy {
public:
	dk_auddrv_proxy();
	~dk_auddrv_proxy();

public:
    unsigned int fetchAudioData( unsigned char* pBufRecv, unsigned int uLenBuf );
	bool pushMicData( unsigned char* pBufMD, unsigned int uLenBufMD );
	void clearAudioData();
	bool fetchMicDataStartEvent( HANDLE* phEventMicNeedData );
	bool fetchMicDataStopEvent( HANDLE* phEventMicSleep );
	bool fetchMicDataState( ENUMDKMICDATA_STATE* peMicDataState );

private:
	void _init();
	void _unInit();
	void _openDevice();
	void _closeDevice();
	bool _isAvailable();

private:
	dk_ts_helper m_tsAccessVAD;
	HANDLE m_hDeviceVAD; //virtual audio device.  
};

#endif //__DKAUDDRVPROXY_H__


