#include "stdafx.h"
#include "dk_task_send.h"
#include "dk_root.h"

dk_task_send::dk_task_send(dk_root* pDKRoot)
: m_pDKRoot( pDKRoot ){
	//m_hFilePCM = ::CreateFile(_T("PCMDump.dat"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	m_hFilePCM = NULL;
	if (m_hFilePCM == INVALID_HANDLE_VALUE) {
		DWORD dwErr = ::GetLastError();
		int a = 0;
	}
}

dk_task_send::~dk_task_send() {
	if (m_hFilePCM) {
		::CloseHandle(m_hFilePCM);
		m_hFilePCM = NULL;
	}
}

void dk_task_send::Run() {
	unsigned char* pBufSend = new unsigned char[ DKAUDIO_CACHE_SIZE ];
	unsigned int uLenRet = 0;
	DWORD dwWritten;

	while ( !_isNeedExitTask( 1 ) )
	{
		uLenRet = dk_ring_popfront( m_pDKRoot->m_pCachePCM, pBufSend, DKAUDIO_CACHE_SIZE );
		if ( uLenRet > 0 ) {
			if ( m_pDKRoot->m_client.isConnected() ) {
				m_pDKRoot->m_client.send(pBufSend, uLenRet);
			}
			//::WriteFile(m_hFilePCM, pBufSend, uLenRet, &dwWritten, NULL);
		} else {
			::Sleep( 1 );
		}
	}

	if ( pBufSend ) {
	    delete[] pBufSend;
		pBufSend = NULL;
	}
}