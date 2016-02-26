#ifndef __DK_ROOT_H__
#define __DK_ROOT_H__
#include "DKTaskEngine.h"
#include "dk_socket_client.h"
#include "dk_ring_cache.h"
#include "dk_auddrv_proxy.h"
#include <string>
//
#define DK_DEST_PORT (6688)
#define DKAUDIO_CACHE_SIZE (20*1024)
//
class dk_root {
public:
	dk_root( const char* pszIPAddrDest );
	~dk_root();

public:

private:
	void _init();
	void _unInit();

public:
	dk_ring_cache* m_pCachePCM;
	dk_auddrv_proxy m_auddrvProxy;
	dk_socket_client m_client;

private:
	std::string m_strIPAddrDest;
	DKTaskEngine m_engine4Fetch;
	DKTaskEngine m_engine4Send;

};



#endif //__DK_ROOT_H__