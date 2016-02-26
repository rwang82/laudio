#ifndef __DK_SOCKET_SERVER_H__
#define __DK_SOCKET_SERVER_H__
#include <winsock2.h>
#include <Windows.h>
#include "dk_ts_helper.h"
#include "hmcmn_event.h"
#include <string>

class dk_socket_server {
public:
	dk_socket_server();
	~dk_socket_server();

public:
	bool start(const char* pszAddressListen, short sPortListen);

private:
	void _init();
	void _unInit();

private:
	static DWORD WINAPI ReceiveThreadProc(void* pParam);
	static DWORD WINAPI AcceptThreadProc(void* pParam);

public:
	hmcmn::hm_event< SOCKET, unsigned char*, unsigned int> m_eventRecvData;
	hmcmn::hm_event< SOCKET > m_eventDisConnected;
	hmcmn::hm_event< SOCKET > m_eventAccepted;

private:
	unsigned int m_uFlag;
	HANDLE m_hEventNeedExit;
	HANDLE m_hThreadRecv;
	HANDLE m_hThreadAccept;
	SOCKET m_socketListen;
	dk_ts_helper m_tsAccessSocketConn;
	SOCKET m_sockConn;
	unsigned char* m_pBufRecv;

};





#endif //__DK_SOCKET_SERVER_H__