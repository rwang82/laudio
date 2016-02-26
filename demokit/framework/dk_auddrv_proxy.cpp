#include "stdafx.h"
#include "dk_auddrv_proxy.h"
#include "dk_ioctl_defs.h"
#include "MemFuncPack.h"
//#include "dk_log.h"
#include <atlconv.h>

dk_auddrv_proxy::dk_auddrv_proxy() {
	_init();
}

dk_auddrv_proxy::~dk_auddrv_proxy() {
    _unInit();
}

unsigned int dk_auddrv_proxy::fetchAudioData( unsigned char* pBufRecv, unsigned int uLenBuf ) {
	if ( !m_tsAccessVAD.safeEnterFunc() )
		return 0;
	CMemFuncPack mfpkSafeExit( &m_tsAccessVAD, &dk_ts_helper::safeExitFunc );
	if ( !_isAvailable() ) {
	    _openDevice();
		if ( !_isAvailable() )
			return 0;
	}
	//
	unsigned int uLenBufCopied = 0;
	if (!DeviceIoControl(m_hDeviceVAD, IOCTL_AUDIO_FETCH, NULL, 0, pBufRecv, uLenBuf, (LPDWORD)&uLenBufCopied, NULL)) {
		//DKLOG_ERROR( "failed to call DeviceIoControl." );
		return 0;
	}

	return uLenBufCopied;
}

void dk_auddrv_proxy::clearAudioData() {
	if (!m_tsAccessVAD.safeEnterFunc())
		return;
	CMemFuncPack mfpkSafeExit(&m_tsAccessVAD, &dk_ts_helper::safeExitFunc);
	if (!_isAvailable()) {
		_openDevice();
		if (!_isAvailable())
			return;
	}

	//
	unsigned int uLenBufCopied = 0;
	if (!DeviceIoControl(m_hDeviceVAD, IOCTL_AUDIO_CLEAR, NULL, 0, NULL, 0, (LPDWORD)&uLenBufCopied, NULL))
		return;

	return;
}

bool dk_auddrv_proxy::pushMicData( unsigned char* pBufMD, unsigned int uLenBufMD ) {
	if ( !m_tsAccessVAD.safeEnterFunc() )
		return 0;
	CMemFuncPack mfpkSafeExit( &m_tsAccessVAD, &dk_ts_helper::safeExitFunc );
	if ( !_isAvailable() ) {
	    _openDevice();
		if ( !_isAvailable() )
			return false;
	}
    DWORD dwRet = 0;
	
	return TRUE == ::DeviceIoControl( m_hDeviceVAD, IOCTL_MIC_PUSH, pBufMD, uLenBufMD, NULL, 0, &dwRet, NULL );
}

bool dk_auddrv_proxy::_isAvailable() {
    return m_hDeviceVAD && ( m_hDeviceVAD != INVALID_HANDLE_VALUE );
}

void dk_auddrv_proxy::_openDevice() {
	USES_CONVERSION;
	TCHAR szSymbolLinkName[] = _T("\\\\.\\DKAFCDO");
	m_hDeviceVAD = ::CreateFile( szSymbolLinkName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if (m_hDeviceVAD == NULL || m_hDeviceVAD == INVALID_HANDLE_VALUE) {
		//DKLOG_ERROR("Open Device Failed! SymbolLinkName:%s\n", T2A(szSymbolLinkName) );
		return;
	}
	//DKLOG_INFO("_openDevice %s success.", T2A(szSymbolLinkName));
}

void dk_auddrv_proxy::_closeDevice() {
	if ( m_hDeviceVAD ) {
		::CloseHandle( m_hDeviceVAD );
		m_hDeviceVAD = NULL;
	}
}

void dk_auddrv_proxy::_init() {
    _openDevice();
}

void dk_auddrv_proxy::_unInit() {
    _closeDevice();
}

bool dk_auddrv_proxy::fetchMicDataStartEvent(HANDLE* phEventMicNeedData) {
	if (!phEventMicNeedData)
		return false;
	if (!m_tsAccessVAD.safeEnterFunc())
		return 0;
	CMemFuncPack mfpkSafeExit(&m_tsAccessVAD, &dk_ts_helper::safeExitFunc);
	if (!_isAvailable()) {
		_openDevice();
		if (!_isAvailable())
			return false;
	}

	//
	unsigned int uLenBufCopied = 0;
	if (!DeviceIoControl(m_hDeviceVAD, IOCTL_FETCH_MICDATA_START_EVENT, NULL, 0, phEventMicNeedData, sizeof(HANDLE), (LPDWORD)&uLenBufCopied, NULL)) {
		//DKLOG_ERROR("failed to call DeviceIoControl.");
		return 0;
	}

	return uLenBufCopied == sizeof( HANDLE );
}

bool dk_auddrv_proxy::fetchMicDataStopEvent(HANDLE* phEventMicSleep) {
	if (!phEventMicSleep)
		return false;
	if (!m_tsAccessVAD.safeEnterFunc())
		return 0;
	CMemFuncPack mfpkSafeExit(&m_tsAccessVAD, &dk_ts_helper::safeExitFunc);
	if (!_isAvailable()) {
		_openDevice();
		if (!_isAvailable())
			return false;
	}

	//
	unsigned int uLenBufCopied = 0;
	if (!DeviceIoControl(m_hDeviceVAD, IOCTL_FETCH_MICSLEEP_EVENT, NULL, 0, phEventMicSleep, sizeof(HANDLE), (LPDWORD)&uLenBufCopied, NULL)) {
		//DKLOG_ERROR("failed to call DeviceIoControl.");
		return 0;
	}

	return uLenBufCopied == sizeof(HANDLE);
}

bool dk_auddrv_proxy::fetchMicDataState(ENUMDKMICDATA_STATE* peMicDataState) {
	if (!peMicDataState)
		return false;
	if (!m_tsAccessVAD.safeEnterFunc())
		return 0;
	CMemFuncPack mfpkSafeExit(&m_tsAccessVAD, &dk_ts_helper::safeExitFunc);
	if (!_isAvailable()) {
		_openDevice();
		if (!_isAvailable())
			return false;
	}

	//
	unsigned int uLenBufCopied = 0;
	if (!DeviceIoControl(m_hDeviceVAD, IOCTL_FETCH_MICDATA_STATE, NULL, 0, peMicDataState, sizeof(ENUMDKMICDATA_STATE), (LPDWORD)&uLenBufCopied, NULL)) {
		//DKLOG_ERROR("failed to call DeviceIoControl.");
		return 0;
	}

	return uLenBufCopied == sizeof(ENUMDKMICDATA_STATE);
}