#include "stdafx.h"
#include "dk_task_fetch.h"
#include "dk_root.h"

dk_task_fetch::dk_task_fetch( dk_root* pDKRoot )
	: m_pDKRoot(pDKRoot) {
	//m_hFilePCM = ::CreateFile(_T("FetchPCMDump.dat"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	m_hFilePCM = NULL;
	if (m_hFilePCM == INVALID_HANDLE_VALUE) {
		DWORD dwErr = ::GetLastError();
		int a = 0;
	}
}

dk_task_fetch::~dk_task_fetch() {
	if (m_hFilePCM) {
		::CloseHandle(m_hFilePCM);
		m_hFilePCM = NULL;
	}
}

void dk_task_fetch::Run() {
	unsigned char* pBufFetch = new unsigned char[ DKAUDIO_CACHE_SIZE ];
	unsigned int uLenRet = 0;
	DWORD dwWritten = 0;

	while ( !_isNeedExitTask( 1 ) ) {
		uLenRet = m_pDKRoot->m_auddrvProxy.fetchAudioData( pBufFetch, DKAUDIO_CACHE_SIZE );
        if ( uLenRet > 0 ) {
			dk_ring_pushback(m_pDKRoot->m_pCachePCM, pBufFetch, uLenRet);
			//::WriteFile(m_hFilePCM, pBufFetch, uLenRet, &dwWritten, NULL);
		} else {
			::Sleep( 1 );
		}
	}

	if ( pBufFetch ) {
	    delete []pBufFetch;
		pBufFetch = NULL;
	}
}