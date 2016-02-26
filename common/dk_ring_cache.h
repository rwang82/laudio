#ifndef __DK_RING_CACHE_H__
#define __DK_RING_CACHE_H__
#ifdef _WIN32
//#include "Synchapi.h"
#include "WinNT.h"
#else
#include <pthread.h>
#endif //_WIN32

struct dk_ring_cache {
#ifdef _WIN32
    HANDLE m_hEventAccessCache;
    HANDLE m_hEventNeedExit;
#else
    pthread_mutex_t m_lockCache;
#endif //_WIN32
    unsigned int m_uFlag;
    unsigned char* m_pBuf;
    unsigned int m_uSizeBuf;
    unsigned int m_uPosStart;
    unsigned int m_uLenData;
};

struct dk_ring_cache* dk_ring_create( unsigned int uSizeCache );

void dk_ring_destroy( struct dk_ring_cache* pDKRC );

void dk_ring_reset( struct dk_ring_cache* pDKRC );

// return 0 means success, -1 means failed.
int dk_ring_pushback( struct dk_ring_cache* pDKRC, unsigned char* pBufIn, unsigned int uLenBufIn );

// return 0 means no data. >0 means specify the number of byte copy to pBufOut
unsigned int dk_ring_popfront( struct dk_ring_cache* pDKRC, unsigned char* pBufOut, unsigned int uLenBufOut );

// return the length of valid data in ring buffer.
unsigned int dk_ring_getdatalength( struct dk_ring_cache* pDKRC );

//
void dk_ring_flush( struct dk_ring_cache* pDKRC, unsigned int uSizeSave );

//
unsigned int dk_ring_copyback( struct dk_ring_cache* pDKRC, unsigned char* pBufCopy, unsigned int uLenBufCopy );














#endif //__DK_RING_CACHE_H__



