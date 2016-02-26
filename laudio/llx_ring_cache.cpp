#include "llx_ring_cache.h"

//
#ifdef DBG
#define DPT   DbgPrint
#else
#define DPT   //
#endif
//
llx_ring_cache::llx_ring_cache()
: m_sizeRingBuf( SIZE_LLX_RING_CACHE )
, m_posStart( 0 )
, m_lenData( 0 ) {

}

llx_ring_cache::~llx_ring_cache() {

}

void llx_ring_cache::init() {
    m_sizeRingBuf = SIZE_LLX_RING_CACHE;
	m_posStart = 0;
	m_lenData = 0;
	KeInitializeSpinLock( &m_SpinLock );
}

int llx_ring_cache::pushBuf( unsigned char* pBufIn, unsigned int uLenBufIn ) {
    if ( uLenBufIn >= m_sizeRingBuf ) {
		RtlCopyMemory( &m_bufRing[ 0 ], ((unsigned char*)pBufIn)+( uLenBufIn - m_sizeRingBuf), m_sizeRingBuf );
		m_posStart = 0;
		m_lenData += m_sizeRingBuf;

	} else {
        if ( m_lenData + uLenBufIn >= m_sizeRingBuf ) {
			unsigned int uLenOldDataLeave = m_sizeRingBuf - uLenBufIn;
			_flush( uLenOldDataLeave );
    	} 
        if ( m_lenData + uLenBufIn > m_sizeRingBuf ) {
            DPT( "m_sizeRingBuf:%d, m_posStart:%d, m_lenData:%d, uLenBufIn:%d\n", m_sizeRingBuf, m_posStart, m_lenData, uLenBufIn );
			return -1;
        }
		//
		unsigned int uPosSaveStart = (m_posStart + m_lenData) % m_sizeRingBuf;
		if ( uPosSaveStart + uLenBufIn <= m_sizeRingBuf ) {
            RtlCopyMemory( &m_bufRing[ uPosSaveStart ], pBufIn, uLenBufIn );
		} else {
            unsigned int uSeg1 = m_sizeRingBuf - uPosSaveStart;
			unsigned int uSeg2 = uLenBufIn - uSeg1;
			RtlCopyMemory( &m_bufRing[ uPosSaveStart ], pBufIn, uSeg1 );
			RtlCopyMemory( &m_bufRing[ 0 ], ((unsigned char*)pBufIn)+uSeg1, uSeg2 );
		}	
		m_lenData += uLenBufIn;
	}

	DPT( "m_sizeRingBuf:%d, m_posStart:%d, m_lenData:%d, uLenBufIn:%d\n", m_sizeRingBuf, m_posStart, m_lenData, uLenBufIn );
    return 0;
}

unsigned int llx_ring_cache::popBuf( unsigned char* pBufOut, unsigned int uLenBufOut ) {
    unsigned int uLenBufCopy = 0;
	unsigned int uPosStartNew = 0;
	
	//DPT( "m_sizeRingBuf:%d, m_posStart:%d, m_lenData:%d, uLenBufOut:%d\n", m_sizeRingBuf, m_posStart, m_lenData, uLenBufOut );
	uLenBufCopy = ( uLenBufOut > m_lenData ) ? m_lenData : uLenBufOut;
	if ( uLenBufCopy != 0 ) {
		if ( m_posStart + uLenBufCopy <= m_sizeRingBuf ) {
			RtlCopyMemory( pBufOut, &( m_bufRing[ m_posStart ] ), uLenBufCopy );
			uPosStartNew = m_posStart + uLenBufCopy;
		} else {
			unsigned int uLenCopySeg1 = m_sizeRingBuf - m_posStart;
			unsigned int uLenCopySeg2 = uLenBufCopy - uLenCopySeg1;
			RtlCopyMemory( pBufOut, &( m_bufRing[ m_posStart ] ), uLenCopySeg1);
			RtlCopyMemory( (unsigned char*)pBufOut + uLenCopySeg1, &( m_bufRing[0] ), uLenCopySeg2 );
			uPosStartNew = uLenCopySeg2;
		}
		//
		m_posStart = uPosStartNew % m_sizeRingBuf;
		m_lenData -= uLenBufCopy;
	}

    return uLenBufCopy;
}

void llx_ring_cache::_flush( unsigned int uSizeLeave ) {
    if ( uSizeLeave >= m_sizeRingBuf || uSizeLeave >= m_lenData )
		return;

	if ( uSizeLeave == 0 ) {
        m_posStart = 0;
		m_lenData = 0;
		return;
	} 
	unsigned int uLenCut = m_lenData - uSizeLeave;
	//
    m_posStart = ( m_posStart + uLenCut )%m_sizeRingBuf;
    m_lenData = uSizeLeave;
	return;
}

void llx_ring_cache::lock() {
    KeAcquireSpinLock( &m_SpinLock, &m_SpinIrql );
}

void llx_ring_cache::unlock() {
    KeReleaseSpinLock( &m_SpinLock, m_SpinIrql );
}





	