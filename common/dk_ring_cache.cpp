#include "stdafx.h"
#include "dk_ring_cache.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
/*
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
*/
#ifndef min
#define min( a, b ) ( (a) < (b) ? (a) : (b) )
#endif //min
//
#define DKRC_FLAG_NONE ( 0x00000000 )
#define DKRC_FLAG_EXIT_ALL ( 0x00000001 )
//
static int _dk_ring_safe_enter( struct dk_ring_cache* pDKRC );
static void _dk_ring_safe_exit( struct dk_ring_cache* pDKRC );
static void _dk_ring_flush( struct dk_ring_cache* pDKRC, unsigned int uSizeSave );
//
struct dk_ring_cache* dk_ring_create( unsigned int uSizeCache ) {
    if ( uSizeCache == 0 ) return 0;
    struct dk_ring_cache* pDKRCNew = (struct dk_ring_cache*)malloc( sizeof( struct dk_ring_cache ) ); 
    memset( pDKRCNew, 0, sizeof( struct dk_ring_cache ) );
    pDKRCNew->m_uFlag = DKRC_FLAG_NONE;
    pDKRCNew->m_pBuf = ( unsigned char* )malloc( uSizeCache );
    pDKRCNew->m_uSizeBuf = uSizeCache;
    pDKRCNew->m_uLenData = 0;
    pDKRCNew->m_uPosStart = 0;
#ifdef _WIN32
    pDKRCNew->m_hEventAccessCache = CreateEvent( NULL, FALSE, TRUE, NULL );
    pDKRCNew->m_hEventNeedExit = CreateEvent( NULL, TRUE, FALSE, NULL );
#else
    pthread_mutex_init( &pDKRCNew->m_lockCache, 0 );
#endif //_WIN32
    return pDKRCNew;
}

void dk_ring_destroy( struct dk_ring_cache* pDKRC ) {
    if ( !pDKRC ) return;
    pDKRC->m_uFlag |= DKRC_FLAG_EXIT_ALL;
#ifdef _WIN32
    SetEvent( pDKRC->m_hEventNeedExit );
    Sleep( 1 );
    if ( pDKRC->m_hEventNeedExit ) {
        CloseHandle( pDKRC->m_hEventNeedExit );
        pDKRC->m_hEventNeedExit = NULL; 
    } 
    if ( pDKRC->m_hEventAccessCache ) {
        CloseHandle( pDKRC->m_hEventAccessCache );
        pDKRC->m_hEventAccessCache = NULL;
    }
#else

#endif //_WIN32
    if ( pDKRC->m_pBuf ) {
        free( pDKRC->m_pBuf );
        pDKRC->m_pBuf = NULL;
    }
    free( pDKRC );
    pDKRC = 0;

}

void dk_ring_reset( struct dk_ring_cache* pDKRC ) {
    if ( 0 != _dk_ring_safe_enter( pDKRC ) )
        return;
    
    pDKRC->m_uPosStart = 0;
    pDKRC->m_uLenData = 0;
    _dk_ring_safe_exit( pDKRC );
}

void dk_ring_flush( struct dk_ring_cache* pDKRC, unsigned int uSizeSave ) {
    if ( !pDKRC ) return;
    if ( 0 != _dk_ring_safe_enter( pDKRC ) )
        return;
    _dk_ring_flush( pDKRC, uSizeSave );
    _dk_ring_safe_exit( pDKRC );
}

void _dk_ring_flush( struct dk_ring_cache* pDKRC, unsigned int uSizeSave ) {
    if ( !pDKRC || uSizeSave >= pDKRC->m_uSizeBuf || uSizeSave >= pDKRC->m_uLenData )
        return;
    if ( uSizeSave == 0 ) {
        pDKRC->m_uPosStart = 0;
        pDKRC->m_uLenData = 0;
        return;
    }
    unsigned int uLenCut = pDKRC->m_uLenData - uSizeSave;
    //
    pDKRC->m_uPosStart = ( pDKRC->m_uPosStart + uLenCut ) % pDKRC->m_uSizeBuf;
    pDKRC->m_uLenData = uSizeSave;
}

// return 0 means success, -1 means failed.
int dk_ring_pushback( struct dk_ring_cache* pDKRC, unsigned char* pBufIn, unsigned int uLenBufIn ) {
    if ( !pDKRC || !pBufIn || uLenBufIn == 0 )
        return -1;
    if ( 0 != _dk_ring_safe_enter( pDKRC ) )
        return -1;
    if ( uLenBufIn > pDKRC->m_uSizeBuf ) {
        memcpy( pDKRC->m_pBuf, pBufIn + ( uLenBufIn - pDKRC->m_uSizeBuf ), pDKRC->m_uSizeBuf );
        pDKRC->m_uPosStart = 0;
        pDKRC->m_uLenData = pDKRC->m_uSizeBuf;
    } else {
        if ( pDKRC->m_uLenData + uLenBufIn > pDKRC->m_uSizeBuf ) {
            unsigned int uLenDataSave = pDKRC->m_uSizeBuf - uLenBufIn;
            _dk_ring_flush( pDKRC, uLenDataSave );
        }
        assert( pDKRC->m_uLenData + uLenBufIn <= pDKRC->m_uSizeBuf );
        //
        unsigned int uPosSaveStart = ( pDKRC->m_uPosStart + pDKRC->m_uLenData ) % pDKRC->m_uSizeBuf;
        if ( uPosSaveStart + uLenBufIn <= pDKRC->m_uSizeBuf ) {
            memcpy( pDKRC->m_pBuf + uPosSaveStart, pBufIn, uLenBufIn );
        } else {
            unsigned int uSeg1 = pDKRC->m_uSizeBuf - uPosSaveStart;
            unsigned int uSeg2 = uLenBufIn - uSeg1;
            memcpy( pDKRC->m_pBuf + uPosSaveStart, pBufIn, uSeg1 );
            memcpy( pDKRC->m_pBuf, pBufIn + uSeg1, uSeg2 );
        }
        pDKRC->m_uLenData += uLenBufIn;
    }

    _dk_ring_safe_exit( pDKRC );
    return 0;
}

// return 0 means no data. >0 means specify the number of byte copy to pBufOut
unsigned int dk_ring_popfront( struct dk_ring_cache* pDKRC, unsigned char* pBufOut, unsigned int uLenBufOut ) {
    if ( !pDKRC || !pBufOut || ( uLenBufOut == 0 ) )
        return 0;
    unsigned int uLenBufCopy = 0;
    unsigned int uPosStartNew = 0; 

    if ( 0 != _dk_ring_safe_enter( pDKRC ) )
        return 0; 
    if ( pDKRC->m_uLenData == 0 ) {
        _dk_ring_safe_exit( pDKRC );
        return 0; 
    }
    uLenBufCopy = min( uLenBufOut, pDKRC->m_uLenData );
    assert( uLenBufCopy > 0 ); 
    //
    if ( pDKRC->m_uPosStart + uLenBufCopy <= pDKRC->m_uSizeBuf ) {
        memcpy( pBufOut, pDKRC->m_pBuf + pDKRC->m_uPosStart, uLenBufCopy );
        uPosStartNew = pDKRC->m_uPosStart + uLenBufCopy;
    } else {
        unsigned int uLenCopySeg1 = pDKRC->m_uSizeBuf - pDKRC->m_uPosStart;
        unsigned int uLenCopySeg2 = uLenBufCopy - uLenCopySeg1;
        memcpy( pBufOut, pDKRC->m_pBuf + pDKRC->m_uPosStart, uLenCopySeg1 );
        memcpy( pBufOut + uLenCopySeg1, pDKRC->m_pBuf, uLenCopySeg2 );
        uPosStartNew = uLenCopySeg2;
    }
    pDKRC->m_uPosStart = uPosStartNew % pDKRC->m_uSizeBuf;
    pDKRC->m_uLenData -= uLenBufCopy;

    _dk_ring_safe_exit( pDKRC );
    return uLenBufCopy;
}

// return the length of valid data in ring buffer.
unsigned int dk_ring_getdatalength( struct dk_ring_cache* pDKRC ) {
    if ( !pDKRC ) return 0;
    unsigned int uLenData = 0;

    if ( 0 != _dk_ring_safe_enter( pDKRC ) )
        return 0; 
    uLenData = pDKRC->m_uLenData;
    _dk_ring_safe_exit( pDKRC );
  
    return uLenData;
}

int _dk_ring_safe_enter( struct dk_ring_cache* pDKRC ) {
    if ( !pDKRC ) return -1;
#ifdef _WIN32
    HANDLE aryEvent[ 2 ];
    DWORD dwWaitRet;

    aryEvent[ 0 ] = pDKRC->m_hEventAccessCache;
    aryEvent[ 1 ] = pDKRC->m_hEventNeedExit;
    dwWaitRet = ::WaitForMultipleObjects( 2, aryEvent, FALSE, INFINITE );
    if ( dwWaitRet == WAIT_OBJECT_0 ) {
        return 0;
    } else if ( dwWaitRet == ( WAIT_OBJECT_0 + 1 ) ) {
        return -1;
    }
#else
    pthread_mutex_lock( &pDKRC->m_lockCache );
#endif //_WIN32
    return ( DKRC_FLAG_EXIT_ALL != ( pDKRC->m_uFlag & DKRC_FLAG_EXIT_ALL ) ) ? 0 : -1;
}

void _dk_ring_safe_exit( struct dk_ring_cache* pDKRC ) {
#ifdef _WIN32
    ::SetEvent( pDKRC->m_hEventAccessCache );
#else
    pthread_mutex_unlock( &pDKRC->m_lockCache );
#endif //_WIN32

}

unsigned int dk_ring_copyback( struct dk_ring_cache* pDKRC, unsigned char* pBufCopy, unsigned int uLenBufCopy ) {
    if ( !pDKRC || !pBufCopy || uLenBufCopy == 0 )
        return 0;
    if ( 0 != _dk_ring_safe_enter( pDKRC ))
        return 0;
    unsigned int uLenNeedCopy = min( uLenBufCopy, pDKRC->m_uLenData );

    if ( uLenNeedCopy > 0 ) {
        if ( pDKRC->m_uPosStart + uLenNeedCopy <= pDKRC->m_uSizeBuf ) {
            memcpy( pBufCopy, pDKRC->m_pBuf + pDKRC->m_uPosStart, uLenNeedCopy );
        } else {
            unsigned int uSeg1 = pDKRC->m_uSizeBuf - pDKRC->m_uPosStart;
            unsigned int uSeg2 = uLenNeedCopy - uSeg1;
            memcpy( pBufCopy, pDKRC->m_pBuf + pDKRC->m_uPosStart, uSeg1 );
            memcpy( pBufCopy + uSeg1, pDKRC->m_pBuf, uSeg2 );
        }
    }
    _dk_ring_safe_exit( pDKRC ); 

    return uLenNeedCopy;
}



