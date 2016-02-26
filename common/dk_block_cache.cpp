#include "stdafx.h"
#include "dk_block_cache.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

//
const unsigned int LEN_BLOCK_SIZE = 4;
//
//
#ifndef min
#define min(x,y) ( (x) < (y) ? (x) : (y) )
#endif //min
//
#ifndef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#endif //max
//
#define DKBC_FLAG_NONE (0x00000000)
#define DKBC_FLAG_NEED_EXIT (0x00000001)
//
int _dk_block_safe_enter( struct dk_block_cache* pDKBC );
void _dk_block_safe_exit( struct dk_block_cache* pDKBC );
void _dk_block_push_newblock( struct dk_block_cache* pDKBC, unsigned char* pBufBlock, unsigned int uBlockSize ); 
unsigned int _dk_block_get_1stblock_size( struct dk_block_cache* pDKBC );
unsigned int _dk_block_pop_1stblock( struct dk_block_cache* pDKBC, unsigned char* pBufBlock, unsigned int uLenBlock );
void _dk_block_flush_1stblock( struct dk_block_cache* pDKBC );
void _dk_block_push_int32( struct dk_block_cache* pDKBC, long val );
void _dk_block_get_int32( struct dk_block_cache* pDKBC, long* pVal );
//
struct dk_block_cache* dk_block_create( unsigned int uSizeCache ) {
    if ( uSizeCache == 0 ) return 0;
    struct dk_block_cache* pDKBCNew = ( struct dk_block_cache* )malloc( sizeof( struct dk_block_cache ) );
    
    memset( pDKBCNew, 0, sizeof( struct dk_block_cache ) );
    pDKBCNew->m_uFlag = DKBC_FLAG_NONE;
    pDKBCNew->m_pBufCache = (unsigned char*)malloc( uSizeCache );
    pDKBCNew->m_uSizeBufCache = uSizeCache;
    pDKBCNew->m_uLenData = 0;
    pDKBCNew->m_uPosStart = 0;
#ifdef _WIN32
    pDKBCNew->m_hEventAccessCache = ::CreateEvent( NULL, FALSE, TRUE, NULL );
    pDKBCNew->m_hEventNeedExit = ::CreateEvent( NULL, TRUE, FALSE, NULL );
#else
    pthread_mutex_init( &pDKBCNew->m_lockCache, 0 );
#endif //_WIN32
    return pDKBCNew;
}

void dk_block_destroy( struct dk_block_cache* pDKBC ) {
    if ( !pDKBC ) return;
    pDKBC->m_uFlag |= DKBC_FLAG_NEED_EXIT;
#ifdef _WIN32
    ::SetEvent(pDKBC->m_hEventNeedExit);
    ::Sleep( 1 );
    if (pDKBC->m_hEventNeedExit) {
        ::CloseHandle( pDKBC->m_hEventNeedExit );
        pDKBC->m_hEventNeedExit = NULL;
    }

    if (pDKBC->m_hEventAccessCache) {
        ::CloseHandle( pDKBC->m_hEventAccessCache );
        pDKBC->m_hEventAccessCache = NULL;
    }
#endif //_WIN32
    if ( pDKBC->m_pBufCache ) {
        free( pDKBC->m_pBufCache );
        pDKBC->m_pBufCache = NULL;
    }
    free( pDKBC );
    pDKBC = 0;
}

int _dk_block_safe_enter( struct dk_block_cache* pDKBC ) {
    if ( !pDKBC ) return -1;
#ifdef _WIN32
    HANDLE aryEvent[2];
    DWORD dwWaitRet;

    aryEvent[0] = pDKBC->m_hEventAccessCache;
    aryEvent[1] = pDKBC->m_hEventNeedExit;
    dwWaitRet = ::WaitForMultipleObjects(2, aryEvent, FALSE, INFINITE);
    if (dwWaitRet == WAIT_OBJECT_0) { // pDKBC->m_hEventAccessCache
        return 0;
    } else if (dwWaitRet == (WAIT_OBJECT_0 + 1)) { // pDKBC->m_hEventTaskEngineExit
        return -1;
    }
#else
    pthread_mutex_lock( &pDKBC->m_lockCache );
#endif //_WIN32
    return ( DKBC_FLAG_NEED_EXIT != ( pDKBC->m_uFlag & DKBC_FLAG_NEED_EXIT ) ) ? 0 : -1;
}

void _dk_block_safe_exit( struct dk_block_cache* pDKBC ) {
#ifdef _WIN32
    ::SetEvent(pDKBC->m_hEventAccessCache);
#else
    pthread_mutex_unlock( &pDKBC->m_lockCache );
#endif //_WIN32
}

int dk_block_pushback( struct dk_block_cache* pDKBC, unsigned char* pBufBlock, unsigned int uLenBlock ) {
    if ( !pDKBC ) return -1;
    unsigned int uLenBufNeedSave = uLenBlock + LEN_BLOCK_SIZE;

    if (0 != _dk_block_safe_enter( pDKBC ))
        return -1;
    if ( uLenBufNeedSave > pDKBC->m_uSizeBufCache ) {
        assert( uLenBufNeedSave <= pDKBC->m_uSizeBufCache );
        _dk_block_safe_exit( pDKBC );
        return -1;
    }
    while( pDKBC->m_uLenData + uLenBufNeedSave > pDKBC->m_uSizeBufCache ) {
        _dk_block_flush_1stblock( pDKBC );
    }
    _dk_block_push_newblock( pDKBC, pBufBlock, uLenBlock );
    pDKBC->m_uLenData += uLenBufNeedSave;
    _dk_block_safe_exit( pDKBC );

    return 0;
}

unsigned int dk_block_getalldata_length( struct dk_block_cache* pDKBC ) {
    if ( !pDKBC ) return 0;
    unsigned int uLenAllData = 0;

    if ( 0 != _dk_block_safe_enter( pDKBC ) )
        return 0;
    uLenAllData = pDKBC->m_uLenData;
    _dk_block_safe_exit( pDKBC );

    return uLenAllData; 
}

unsigned int dk_block_get1stblock_size( struct dk_block_cache* pDKBC ) {
    if ( !pDKBC ) return 0;
    unsigned int uBlockSize = 0;
   
    if (0 != _dk_block_safe_enter( pDKBC ))
        return 0;
    uBlockSize = _dk_block_get_1stblock_size(pDKBC);
    _dk_block_safe_exit( pDKBC );

    return uBlockSize;
}

void dk_block_reset( struct dk_block_cache* pDKBC ) { 
    if (0 != _dk_block_safe_enter( pDKBC ))
        return;
    pDKBC->m_uLenData = 0; 
    pDKBC->m_uPosStart = 0; 
    _dk_block_safe_exit( pDKBC ); 
}

unsigned int _dk_block_get_1stblock_size( struct dk_block_cache* pDKBC ) {
    if ( !pDKBC ) return 0;
    if ( pDKBC->m_uLenData == 0 ) {
        return 0;
    }
    long i32BlockSize = 0;
    
    _dk_block_get_int32( pDKBC, (long*)&i32BlockSize );
    return i32BlockSize;
}

unsigned int dk_block_pop1stblock( struct dk_block_cache* pDKBC, unsigned char* pBufBlock, unsigned int uLenBlock ) {
    if ( !pDKBC || !pBufBlock || uLenBlock == 0 ) return 0;
    unsigned int uSizeCopied = 0;

    if (0 != _dk_block_safe_enter( pDKBC ))
        return 0;
    uSizeCopied = _dk_block_pop_1stblock( pDKBC, pBufBlock, uLenBlock );
    _dk_block_safe_exit( pDKBC );

    return uSizeCopied;
}

unsigned int _dk_block_pop_1stblock( struct dk_block_cache* pDKBC, unsigned char* pBufBlock, unsigned int uLenBlock ) {
    if ( !pDKBC || !pBufBlock || pDKBC->m_uLenData == 0 )
        return 0;
    long i32BlockSize = 0;
    unsigned int uPos1stBlock = 0;
    
    _dk_block_get_int32( pDKBC, (long*)&i32BlockSize );
    if ( i32BlockSize == 0 || (unsigned int)i32BlockSize > uLenBlock ) {
        return 0;
    } 
    
    uPos1stBlock = ( pDKBC->m_uPosStart + LEN_BLOCK_SIZE ) % pDKBC->m_uSizeBufCache;
    //
    if ( uPos1stBlock + i32BlockSize <= pDKBC->m_uSizeBufCache ) {
        memcpy( pBufBlock, &pDKBC->m_pBufCache[ uPos1stBlock ], i32BlockSize );

    } else {
        unsigned int uSeg1 = pDKBC->m_uSizeBufCache - uPos1stBlock;
        unsigned int uSeg2 = i32BlockSize - uSeg1;
        memcpy( pBufBlock, &pDKBC->m_pBufCache[ pDKBC->m_uPosStart + LEN_BLOCK_SIZE ], uSeg1 );
        memcpy( pBufBlock + uSeg1, &pDKBC->m_pBufCache[ 0 ], uSeg2 );
    }
    //
    pDKBC->m_uLenData -= ( LEN_BLOCK_SIZE + i32BlockSize );
    if ( pDKBC->m_uLenData == 0 ) {
        pDKBC->m_uPosStart = 0;
    } else {
        pDKBC->m_uPosStart = ( pDKBC->m_uPosStart + LEN_BLOCK_SIZE + i32BlockSize ) % pDKBC->m_uSizeBufCache;
    }
    
    return i32BlockSize;
}

void _dk_block_flush_1stblock( struct dk_block_cache* pDKBC ) {
    if ( !pDKBC ) return;
    long i32BlockSize = 0;
    
    _dk_block_get_int32( pDKBC, &i32BlockSize );

    pDKBC->m_uLenData -= ( LEN_BLOCK_SIZE + i32BlockSize );
    if ( pDKBC->m_uLenData == 0 ) {
        pDKBC->m_uPosStart = 0;
    } else {
        pDKBC->m_uPosStart = ( pDKBC->m_uPosStart + LEN_BLOCK_SIZE + i32BlockSize )%pDKBC->m_uSizeBufCache;
    }
}

void _dk_block_push_newblock( struct dk_block_cache* pDKBC, unsigned char* pBufBlock, unsigned int uBlockSize ) {
    if ( !pDKBC || !pBufBlock || uBlockSize == 0 )
        return;
    unsigned int uPosTail = ( pDKBC->m_uPosStart + pDKBC->m_uLenData ) % pDKBC->m_uSizeBufCache;
    unsigned int uPosStartNewBlock = (uPosTail + LEN_BLOCK_SIZE) % pDKBC->m_uSizeBufCache;

    //
    _dk_block_push_int32( pDKBC, (long)uBlockSize );
    //
    if ( uPosStartNewBlock + uBlockSize <= pDKBC->m_uSizeBufCache ) {
        memcpy( &pDKBC->m_pBufCache[ uPosStartNewBlock ], pBufBlock, uBlockSize );
    } else {
        unsigned int uSeg1 = pDKBC->m_uSizeBufCache - uPosStartNewBlock;
        unsigned int uSeg2 = uBlockSize - uSeg1;
        memcpy( &pDKBC->m_pBufCache[ uPosStartNewBlock ], pBufBlock, uSeg1 );
        memcpy( &pDKBC->m_pBufCache[ 0 ], pBufBlock + uSeg1, uSeg2 );
    }
}

void _dk_block_push_int32( struct dk_block_cache* pDKBC, long val ) {
    assert( pDKBC );
    unsigned int uPosTail = ( pDKBC->m_uPosStart + pDKBC->m_uLenData ) % pDKBC->m_uSizeBufCache;
    
    if ( uPosTail + LEN_BLOCK_SIZE <= pDKBC->m_uSizeBufCache ) {
        memcpy( &pDKBC->m_pBufCache[ uPosTail ], &val, LEN_BLOCK_SIZE );
    } else {
        unsigned int uSeg1 = pDKBC->m_uSizeBufCache - uPosTail;
        unsigned int uSeg2 = LEN_BLOCK_SIZE - uSeg1;
        
        memcpy( &pDKBC->m_pBufCache[ uPosTail ], &val, uSeg1 );
        memcpy( &pDKBC->m_pBufCache[ 0 ], ((unsigned char*)&val)+uSeg1, uSeg2 );
    }
}

void _dk_block_get_int32( struct dk_block_cache* pDKBC, long* pVal ) {
    assert( pDKBC && pVal );
    if ( pDKBC->m_uPosStart + LEN_BLOCK_SIZE <= pDKBC->m_uSizeBufCache ) {
        memcpy( pVal, &pDKBC->m_pBufCache[ pDKBC->m_uPosStart ], LEN_BLOCK_SIZE );
    } else {
        unsigned int uSeg1 = pDKBC->m_uSizeBufCache - pDKBC->m_uPosStart;
        unsigned int uSeg2 = LEN_BLOCK_SIZE - uSeg1;

        memcpy( pVal, &pDKBC->m_pBufCache[ pDKBC->m_uPosStart ], uSeg1 );
        memcpy( ((unsigned char*)pVal)+uSeg1, &pDKBC->m_pBufCache[ 0 ], uSeg2 );
    }
}
