#ifndef __LLX_RING_CACHE_H__
#define __LLX_RING_CACHE_H__
#include <portcls.h>
//
#define SIZE_LLX_RING_CACHE (8*1024)
//
struct llx_ring_cache {
public:
	llx_ring_cache();
	~llx_ring_cache();

public:
	void init();
	int pushBuf( unsigned char* pBufIn, unsigned int uLenBufIn );
	unsigned int popBuf( unsigned char* pBufOut, unsigned int uLenBufOut );
	unsigned int getDataLen() { return m_lenData; }
    void lock();
	void unlock();

	
private:
	void _flush( unsigned int uSizeLeave );

private:
    KSPIN_LOCK m_SpinLock;
	KIRQL m_SpinIrql;
	unsigned char m_bufRing[ SIZE_LLX_RING_CACHE ];
	unsigned int m_sizeRingBuf;
	unsigned int m_posStart;
	unsigned int m_lenData;
};

#endif //__LLX_RING_CACHE_H__