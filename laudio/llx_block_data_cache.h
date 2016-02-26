#ifndef __LLX_BLOCK_DATA_CACHE_H__
#define __LLX_BLOCK_DATA_CACHE_H__
#include <portcls.h>
//
#define SIZE_LLX_CACHE_4_MICDATA (128*1024)
//
struct llx_block_data_cache {
public:
	llx_block_data_cache();
	~llx_block_data_cache();

public:
	////////////////////////////////////////////////////////////////////////////
	//
	void init();
	////////////////////////////////////////////////////////////////////////////
	//
	void lock();
	////////////////////////////////////////////////////////////////////////////
	//
	void unlock();
	////////////////////////////////////////////////////////////////////////////
	//
	bool pushbackBlock( unsigned char* pBufBlock, unsigned int uLenBlock );
	////////////////////////////////////////////////////////////////////////////
	//
	unsigned int get1stBlockSize();
	////////////////////////////////////////////////////////////////////////////
	// return value : specify the number of byte copy to pBufBlock.
	unsigned int pop1stBlock( unsigned char* pBufBlock, unsigned int uLenBlock );
	////////////////////////////////////////////////////////////////////////////
	// return value : specify the number of byte copy to pBufBlockMax.
	unsigned int popBlockMax( unsigned char* pBufBlockMax, unsigned int uLenBufBlockMax );
	////////////////////////////////////////////////////////////////////////////
	//
	void reset() { m_dwLenData = 0; m_dwPosStart = 0; }

private:
	void _flush1stBlock();
	void _pushNewBlock( unsigned char* pBufBlock, unsigned int uBlockSize );
	void _pushValInt32( __int32 val );
	void _getValInt32( __int32& val );

private:
    KSPIN_LOCK m_SpinLock;
	KIRQL m_SpinIrql;
	unsigned char m_bufCache[ SIZE_LLX_CACHE_4_MICDATA ];
	unsigned int m_uSizeBufCache;
	volatile DWORD m_dwLenData;
	DWORD m_dwPosStart;
};



#endif //__LLX_BLOCK_DATA_CACHE_H__