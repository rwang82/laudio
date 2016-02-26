#include "llx_block_data_cache.h"

const unsigned int LEN_BLOCK_SIZE = 4;

llx_block_data_cache::llx_block_data_cache()
: m_dwLenData( 0 )
, m_dwPosStart( 0 )
, m_uSizeBufCache( SIZE_LLX_CACHE_4_MICDATA ) {
}

llx_block_data_cache::~llx_block_data_cache() {
}

void llx_block_data_cache::init() {
    m_uSizeBufCache = SIZE_LLX_CACHE_4_MICDATA;
  	reset();
}

void llx_block_data_cache::lock() {
	KeAcquireSpinLock( &m_SpinLock, &m_SpinIrql );
}

void llx_block_data_cache::unlock() {
	KeReleaseSpinLock( &m_SpinLock, m_SpinIrql );
}

bool llx_block_data_cache::pushbackBlock( unsigned char* pBufBlock, unsigned int uLenBlock ) {
	unsigned int uLenBufNeedSave = uLenBlock + LEN_BLOCK_SIZE;

	while( m_dwLenData + uLenBufNeedSave > m_uSizeBufCache ) {
	    _flush1stBlock();
	}
	
	_pushNewBlock( pBufBlock, uLenBlock );
	
	m_dwLenData += uLenBufNeedSave;

	return true;
}

unsigned int llx_block_data_cache::get1stBlockSize() {
	if ( m_dwLenData == 0 )
		return 0;
    __int32 i32BlockSize = 0;
	
	_getValInt32( i32BlockSize );
	return i32BlockSize;
}

unsigned int llx_block_data_cache::popBlockMax( unsigned char* pBufBlockMax, unsigned int uLenBufBlockMax ) {
    unsigned int uBufCopy = 0;
    unsigned int u1stBlockSize = 0;
	unsigned int uBlockSize = 0;
	
    
	u1stBlockSize = get1stBlockSize();
	while ( u1stBlockSize > 0 && u1stBlockSize < (uLenBufBlockMax - uBufCopy) ) {
	    uBlockSize = pop1stBlock( pBufBlockMax + uBufCopy, (uLenBufBlockMax - uBufCopy) );
		//
		uBufCopy += uBlockSize;
		u1stBlockSize = get1stBlockSize();
	}

    return uBufCopy;
}

unsigned int llx_block_data_cache::pop1stBlock( unsigned char* pBufBlock, unsigned int uLenBlock ) {
	if ( !pBufBlock || m_dwLenData == 0 )
		return 0;
    __int32 i32BlockSize = 0;
	unsigned int uPos1stBlock = 0;
	
	_getValInt32( i32BlockSize );
	if ( i32BlockSize == 0 || (unsigned int)i32BlockSize > uLenBlock ) {
	    return 0;
	} 
	
	uPos1stBlock = ( m_dwPosStart + LEN_BLOCK_SIZE ) % m_uSizeBufCache;
	//
	if ( uPos1stBlock + i32BlockSize <= m_uSizeBufCache ) {
		RtlCopyMemory( pBufBlock, &m_bufCache[ uPos1stBlock ], i32BlockSize );
	} else {
		unsigned int uSeg1 = m_uSizeBufCache - uPos1stBlock;
		unsigned int uSeg2 = i32BlockSize - uSeg1;
		RtlCopyMemory( pBufBlock, &m_bufCache[ m_dwPosStart + LEN_BLOCK_SIZE ], uSeg1 );
		RtlCopyMemory( pBufBlock + uSeg1, &m_bufCache[ 0 ], uSeg2 );
	}
	//
	m_dwLenData -= ( LEN_BLOCK_SIZE + i32BlockSize );
	if ( m_dwLenData == 0 ) {
	    m_dwPosStart = 0;
	} else {
	    m_dwPosStart = ( m_dwPosStart + LEN_BLOCK_SIZE + i32BlockSize ) % m_uSizeBufCache;
	}
    
	return i32BlockSize;
}

void llx_block_data_cache::_flush1stBlock() {
    __int32 i32BlockSize = 0;
	
	_getValInt32( i32BlockSize );

	m_dwLenData -= ( LEN_BLOCK_SIZE + i32BlockSize );
	if ( m_dwLenData == 0 ) {
	    m_dwPosStart = 0;
	} else {
	    m_dwPosStart = ( m_dwPosStart + LEN_BLOCK_SIZE + i32BlockSize )%m_uSizeBufCache;
	}
}

void llx_block_data_cache::_pushNewBlock( unsigned char* pBufBlock, unsigned int uBlockSize ) {
    unsigned int uPosTail = ( m_dwPosStart + m_dwLenData ) % m_uSizeBufCache;
	unsigned int uPosStartNewBlock = (uPosTail + LEN_BLOCK_SIZE) % m_uSizeBufCache;

	//
	_pushValInt32( (__int32)uBlockSize );
    //
	if ( uPosStartNewBlock + uBlockSize <= m_uSizeBufCache ) {
	    RtlCopyMemory( &m_bufCache[ uPosStartNewBlock ], pBufBlock, uBlockSize );
	} else {
	    unsigned int uSeg1 = m_uSizeBufCache - uPosStartNewBlock;
		unsigned int uSeg2 = uBlockSize - uSeg1;
 		RtlCopyMemory( &m_bufCache[ uPosStartNewBlock ], pBufBlock, uSeg1 );
 		RtlCopyMemory( &m_bufCache[ 0 ], pBufBlock + uSeg1, uSeg2 );
	}
}

void llx_block_data_cache::_pushValInt32( __int32 val ) {
    unsigned int uPosTail = ( m_dwPosStart + m_dwLenData ) % m_uSizeBufCache;
    
	if ( uPosTail + LEN_BLOCK_SIZE <= m_uSizeBufCache ) {
		RtlCopyMemory( &m_bufCache[ uPosTail ], &val, LEN_BLOCK_SIZE );
	} else {
	    unsigned int uSeg1 = m_uSizeBufCache - uPosTail;
		unsigned int uSeg2 = LEN_BLOCK_SIZE - uSeg1;
        
        RtlCopyMemory( &m_bufCache[ uPosTail ], &val, uSeg1 );
		RtlCopyMemory( &m_bufCache[ 0 ], ((unsigned char*)&val)+uSeg1, uSeg2 );
	}
}

void llx_block_data_cache::_getValInt32( __int32& val ) {
	if ( m_dwPosStart + LEN_BLOCK_SIZE <= m_uSizeBufCache ) {
	    RtlCopyMemory( &val, &m_bufCache[ m_dwPosStart ], LEN_BLOCK_SIZE );
	} else {
	    unsigned int uSeg1 = m_uSizeBufCache - m_dwPosStart;
		unsigned int uSeg2 = LEN_BLOCK_SIZE - uSeg1;

		RtlCopyMemory( &val, &m_bufCache[ m_dwPosStart ], uSeg1 );
		RtlCopyMemory( ((unsigned char*)&val)+uSeg1, &m_bufCache[ 0 ], uSeg2 );
	}
}