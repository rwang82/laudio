#include "stdafx.h"
#include "dk_root.h"
#include "dk_task_send.h"
#include "dk_task_fetch.h"

dk_root::dk_root( const char* pszIPAddrDest )
: m_strIPAddrDest( pszIPAddrDest )
, m_pCachePCM( NULL )
{
	_init();
}

dk_root::~dk_root() {
	_unInit();
}

void dk_root::_init() {
	DKTaskEngine::task_id_type taskId;

	//
	m_pCachePCM = dk_ring_create( DKAUDIO_CACHE_SIZE );
	//
	m_client.connect( m_strIPAddrDest.c_str(), DK_DEST_PORT );
	//
	m_engine4Fetch.pushbackTask( new dk_task_fetch( this ), taskId );
	m_engine4Send.pushbackTask( new dk_task_send( this ), taskId );
}

void dk_root::_unInit() {
	m_client.closeConnect();
	//
	m_engine4Fetch.exit();
	m_engine4Send.exit();
	//
	if ( m_pCachePCM ) {
	    dk_ring_destroy( m_pCachePCM );
		m_pCachePCM = NULL;
	}
}




