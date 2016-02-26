#include "stdafx.h"
#include <winsock2.h>
#include <Windows.h>
#include "dk_socket_client.h"
#include "MemFuncPack.h"
#pragma comment(lib, "ws2_32.lib")
//
#ifndef HITTEST_BIT
#define HITTEST_BIT( val, bit ) ( ( val & bit ) == bit )
#endif //HITTEST_BIT
//
#define DKSC_FLAG_NONE (0x00000000)
#define DKSC_FLAG_MANUAL_RECONNECT (0x00000001)
//
dk_socket_client::dk_socket_client(bool bManualReConnect)
: m_uFlag(bManualReConnect ? DKSC_FLAG_MANUAL_RECONNECT : DKSC_FLAG_NONE)
, m_hEventNeedExit( NULL )
, m_hRecvThread( NULL )
, m_strIPAddrDest("")
, m_sPortDest( 0 )
, m_strIPAddrBind("")
, m_sPortBind( 0 )
, m_socketConn( NULL )
, m_pBufRecv( NULL ) {
	_init();
}

dk_socket_client::~dk_socket_client() {
	_unInit();
}

void dk_socket_client::connect(const char* pszIPDest, short sPortDest, const char* pszIPBind, short sPortBind) {
	if (!pszIPDest || sPortDest == 0)
		return;
	if (!m_tsAccessSocket.safeEnterFunc())
		return;
	CMemFuncPack mfpk( &m_tsAccessSocket, &dk_ts_helper::safeExitFunc );

	m_strIPAddrDest = pszIPDest;
	m_sPortDest = sPortDest;
	m_strIPAddrBind = pszIPBind ? pszIPBind : "";
	m_sPortBind = sPortBind;

	// close first, then connect thread will connect to dest address.
	_do_close();
	//
	Sleep( 50 );

	if (HITTEST_BIT(m_uFlag, DKSC_FLAG_MANUAL_RECONNECT)) {
		_do_connect(pszIPDest, sPortDest, m_strIPAddrBind.c_str(), m_sPortBind);
	}
}

void dk_socket_client::closeConnect() {
	if (!m_tsAccessSocket.safeEnterFunc())
		return;
	CMemFuncPack mfpk(&m_tsAccessSocket, &dk_ts_helper::safeExitFunc);
	_do_close();
}

void dk_socket_client::_do_close() {
	if (m_socketConn) {
		::closesocket(m_socketConn);
	}
	m_socketConn = NULL;
}

bool dk_socket_client::isConnected() {
	if (!m_tsAccessSocket.safeEnterFunc())
		return false;
	CMemFuncPack mfpk(&m_tsAccessSocket, &dk_ts_helper::safeExitFunc);
    
	return _is_connected();
}

void dk_socket_client::send(unsigned char* pBuf, unsigned int uLenBuf) {
	if (!pBuf || (uLenBuf == 0))
		return;
	if (!m_tsAccessSocket.safeEnterFunc())
		return;
	CMemFuncPack mfpk(&m_tsAccessSocket, &dk_ts_helper::safeExitFunc);

	if ( !_is_connected() )
		return;
	::send(m_socketConn, (const char*)pBuf, (int)uLenBuf, 0 );
}

bool dk_socket_client::_is_connected() {
	return m_socketConn != NULL;
}

bool dk_socket_client::_do_connect(const char* pszIPDest, short sPort, const char* pszIPBind, short sPortBind) {
	struct sockaddr_in destAddr;
	struct sockaddr_in localAddr;
	int nRet = 0;

	m_socketConn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socketConn == INVALID_SOCKET) {
		m_socketConn = NULL;
		return false;
	}

	// bind local ip.
	if (m_strIPAddrBind.length() > 0) {
		localAddr.sin_family = AF_INET;
		localAddr.sin_addr.s_addr = inet_addr((char*)m_strIPAddrBind.c_str());
		localAddr.sin_port = m_sPortBind;
		int nRet = ::bind(m_socketConn, (sockaddr*)&localAddr, sizeof(sockaddr));
		if (nRet != 0) {
			goto error;
		}
	}

	// connect remote ip.
	destAddr.sin_family = AF_INET;
	destAddr.sin_addr.s_addr = inet_addr( pszIPDest );
	destAddr.sin_port = htons( sPort );
	memset(destAddr.sin_zero, 0x00, 8);
	nRet = ::connect(m_socketConn, (struct sockaddr*)&destAddr, sizeof(destAddr));
	if (nRet == SOCKET_ERROR) {
		goto error;
	}

	m_eventConnected.invoke();
	return true;

error:
	if (m_socketConn) {
		::closesocket(m_socketConn);
		m_socketConn = NULL;
	}
	return false;
}

DWORD WINAPI dk_socket_client::ReceiveThreadProc(void* pParam) {
	DWORD dwWaitRet = 0;
	dk_socket_client* pDKSClient = (dk_socket_client*)pParam;
	int nLenRecv = 0;
	SOCKET sConnect = NULL;

	while (1)
	{
		dwWaitRet = ::WaitForSingleObject( pDKSClient->m_hEventNeedExit, 1 );
		if (dwWaitRet != WAIT_TIMEOUT)
			break;

		if (!pDKSClient->m_tsAccessSocket.safeEnterFunc())
			break;
		CMemFuncPack mfpk(&pDKSClient->m_tsAccessSocket, &dk_ts_helper::safeExitFunc);
		//
		if (!pDKSClient->_is_connected()) {
			::Sleep(100);
			continue;
		}
		sConnect = pDKSClient->m_socketConn;
		mfpk.Do();
		nLenRecv = ::recv(sConnect, (char*)pDKSClient->m_pBufRecv, DKSC_LEN_RECVBUF, 0);
		if (nLenRecv <= 0) {
			pDKSClient->_do_close();
			//
			pDKSClient->m_eventDisConnect.invoke();
			continue;
		}

		pDKSClient->m_eventRecvData.invoke(pDKSClient->m_pBufRecv, nLenRecv);
	}

	return 0;
}

DWORD WINAPI dk_socket_client::ConnectThreadProc(void* pParam) {
	dk_socket_client* pDKSClient = (dk_socket_client*)pParam;

	while ( WAIT_TIMEOUT == ::WaitForSingleObject(pDKSClient->m_hEventNeedExit, 100) )
	{
		if (!pDKSClient->m_tsAccessSocket.safeEnterFunc())
			break;
		CMemFuncPack mfpk(&pDKSClient->m_tsAccessSocket, &dk_ts_helper::safeExitFunc);
		//
		if (pDKSClient->_is_connected()) {
			continue;
		}
		//
		if (!pDKSClient->_is_dest_address_valied()) {
			continue;
		}
		//
		pDKSClient->_do_close();
		//
		pDKSClient->_do_connect(pDKSClient->m_strIPAddrDest.c_str(), pDKSClient->m_sPortDest, pDKSClient->m_strIPAddrBind.c_str(), pDKSClient->m_sPortBind );
	}

	return 0;
}

void dk_socket_client::_init() {
	WSADATA Ws;
	//Init Windows Socket
	if (WSAStartup(MAKEWORD(2, 2), &Ws) != 0) {
	    // need more code here. to notify error happened.
	}

	if (m_pBufRecv) {
		delete []m_pBufRecv;
		m_pBufRecv = NULL;
	}
	m_pBufRecv = new unsigned char[DKSC_LEN_RECVBUF];
	m_hEventNeedExit = ::CreateEvent(NULL, TRUE, FALSE, NULL );
	m_hRecvThread = ::CreateThread(NULL, 0, &dk_socket_client::ReceiveThreadProc, this, 0, NULL);
	if (!HITTEST_BIT(m_uFlag, DKSC_FLAG_MANUAL_RECONNECT)) {
		::CreateThread(NULL, 0, &dk_socket_client::ConnectThreadProc, this, 0, NULL);
	}
}

void dk_socket_client::_unInit() {
	if (m_hEventNeedExit) {
		::SetEvent(m_hEventNeedExit);
	}
	m_tsAccessSocket.cancelAllAccess();
	::Sleep(1);
	::CloseHandle(m_hEventNeedExit);

	//
	if (m_hRecvThread) {
		::WaitForSingleObject(m_hRecvThread, INFINITE);
	}
	if (m_pBufRecv) {
		delete[]m_pBufRecv;
		m_pBufRecv = NULL;
	}
	
	::WSACleanup();
}

bool dk_socket_client::_is_dest_address_valied() {
	return m_strIPAddrDest.length() > 7 && (m_sPortDest != 0);
}