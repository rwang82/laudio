#ifndef __DK_SOCKET_CLIENT_H__
#define __DK_SOCKET_CLIENT_H__
#include "dk_ts_helper.h"
#include "hmcmn_event.h"
#include <string>
#include <winsock2.h>
//
#define DKSC_LEN_RECVBUF (1024*1024)
//
class dk_socket_client {
public:
	dk_socket_client(bool bManualReConnect = false);
	~dk_socket_client();

public:
	void connect(const char* pszIPDest, short sPortDest, const char* pszIPBind = NULL, short sPortBind = 0);
	void send(unsigned char* pBuf, unsigned int uLenBuf);
	bool isConnected();
	void closeConnect();

private:
	bool _do_connect(const char* pszIPDest, short sPort, const char* pszIPBind, short sPortBind);
	void _do_close();
	bool _is_dest_address_valied();
	bool _is_connected();
	void _init();
	void _unInit();

private:
	static DWORD WINAPI ReceiveThreadProc(void* pParam);
	static DWORD WINAPI ConnectThreadProc(void* pParam);

public:
	hmcmn::hm_event<unsigned char*, unsigned int> m_eventRecvData;
	hmcmn::hm_event<> m_eventDisConnect;
	hmcmn::hm_event<> m_eventConnected;

private:
	unsigned int m_uFlag;
	HANDLE m_hEventNeedExit;
	HANDLE m_hRecvThread;
	std::string m_strIPAddrDest;
	short m_sPortDest;
	std::string m_strIPAddrBind;
	short m_sPortBind;
	dk_ts_helper m_tsAccessSocket;
	SOCKET m_socketConn;
	unsigned char* m_pBufRecv;
};


#endif //__DK_SOCKET_CLIENT_H__