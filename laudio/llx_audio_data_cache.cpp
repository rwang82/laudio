#include "llx_audio_data_cache.h"
#ifdef __cplusplus
extern "C"{
#include <Ntifs.h>
}
#else
#include <Ntifs.h>
#endif
#include <portcls.h>
#include <stdunk.h>
#include <ksdebug.h>

//
#ifndef VA_POOLTAG
  #define VA_POOLTAG 'FXSD'
#endif //VA_POOLTAG
#ifndef DPT
  #ifdef DBG
    #define DPT   DbgPrint
  #else
    #define DPT   //
  #endif //DBG
#endif //DPT

//
//const unsigned int SIZE_LLX_ADC_BUFFER = (128*1024);
const unsigned int SIZE_LLX_ADC_BUFFER = (20 * 1024);
const unsigned int LEN_BLOCK_SIZE = 4; 
//
struct LLXAudioDataCache {
    KSPIN_LOCK m_SpinLock;
	KIRQL m_SpinIrql;

	unsigned int m_posStart;
	volatile unsigned int m_lenData;
	unsigned char m_bufADC[ SIZE_LLX_ADC_BUFFER ];
};
//
#define ADC_LOCK( adc_obj ) KeAcquireSpinLock( &adc_obj->m_SpinLock, &adc_obj->m_SpinIrql );
#define ADC_UNLOCK( adc_obj ) KeReleaseSpinLock( &adc_obj->m_SpinLock, adc_obj->m_SpinIrql );
//
static int get1stBlockSizeNoLock( LLXAudioDataCache* pADC );
static int flush1stBlockNoLock( LLXAudioDataCache* pADC );
static void flushAllBlockNoLock( LLXAudioDataCache* pADC );
static int pushNewBlockSizeNoLock( LLXAudioDataCache* pADC, unsigned int uSizeNewBlock );
static int pushNewBlockBufNoLock( LLXAudioDataCache* pADC, unsigned char* pBufBlockNew, unsigned int uSizeBufBlockNew );
//
void* adcobj_create(){
    LLXAudioDataCache* pADCObj = (LLXAudioDataCache*)ExAllocatePoolWithTag( NonPagedPool, sizeof( struct LLXAudioDataCache ), VA_POOLTAG );
    if( !pADCObj ) {
        DPT("adcobj_create() Error<No Memory>.\n");
		return NULL;
	}
    //
	KeInitializeSpinLock( &pADCObj->m_SpinLock );
    pADCObj->m_posStart = pADCObj->m_lenData = 0;
    //
    return pADCObj;
}

//
void adcobj_destroy(void* hadc){
	if (hadc) {
		ExFreePoolWithTag(hadc, VA_POOLTAG);
	}
}

unsigned int g_uDataSaveCount = 0;
unsigned int g_uDataSaveCount_4per = 0;
unsigned int g_uDataFetchCount = 0;

void audio_data_clear( void* hadc ) {
    LLXAudioDataCache* pADCObj = (LLXAudioDataCache*)hadc;
	ADC_LOCK(pADCObj);

	pADCObj->m_posStart = 0;
	pADCObj->m_lenData = 0;
	//
	g_uDataSaveCount = 0;
	g_uDataSaveCount_4per = 0;
    g_uDataFetchCount = 0;
	//
	DPT("audio_data_clear\n");

	ADC_UNLOCK(pADCObj);
}

//
int audio_data_save( void* hadc, void* pBuf, unsigned int uLenBuf ) {
    if ( !hadc || !pBuf ) {
        DPT( "Enter audio_data_save()\n" );    
		DPT("exit audio_data_save\n");
		return -1;
    }
    LLXAudioDataCache* pADCObj = (LLXAudioDataCache*)hadc;
	ADC_LOCK( pADCObj );
	unsigned int posTmp = 0;
	unsigned int uLenBufNeedSave = uLenBuf + LEN_BLOCK_SIZE;
    //
    g_uDataSaveCount += uLenBuf;
	//
    if ( SIZE_LLX_ADC_BUFFER <= uLenBufNeedSave ) {
        DPT("*********  SIZE_LLX_ADC_BUFFER < uLenBufNeedSave  *********\n");
		RtlCopyMemory( &(pADCObj->m_bufADC[0]), (&uLenBuf), LEN_BLOCK_SIZE );
        RtlCopyMemory( &(pADCObj->m_bufADC[LEN_BLOCK_SIZE]), (unsigned char*)pBuf + ( uLenBufNeedSave - SIZE_LLX_ADC_BUFFER ), ( SIZE_LLX_ADC_BUFFER - LEN_BLOCK_SIZE ) );
		pADCObj->m_posStart = 0;
		pADCObj->m_lenData = SIZE_LLX_ADC_BUFFER;
	} else {
        // while ( SIZE_LLX_ADC_BUFFER < pADCObj->m_lenData + uLenBufNeedSave ) {
		//	if ( 0 != flush1stBlockNoLock( pADCObj ) ) {
		//		DPT( "error: flush1stBlock failed!\n" );
		//		ADC_UNLOCK( pADCObj );
		//		return -1; // 
		//	}
		//}

		if (SIZE_LLX_ADC_BUFFER < pADCObj->m_lenData + uLenBufNeedSave) {
			flushAllBlockNoLock( pADCObj );
		}

		// now SIZE_LLX_ADC_BUFFER >= pADCObj->m_lenData + uLenBufNeedSave.
        //
        if ( 0 != pushNewBlockSizeNoLock( pADCObj, uLenBuf ) ) {
            DPT( "error: pushNewBlockSizeNoLock failed!\n" );
			ADC_UNLOCK( pADCObj );
			return -1;
		}
		if ( 0 != pushNewBlockBufNoLock( pADCObj, (unsigned char*)pBuf, uLenBuf ) ) {
            DPT( "error: pushNewBlockBufNoLock failed!\n" );
            ADC_UNLOCK( pADCObj );
			return -1;
		}
		pADCObj->m_lenData += uLenBufNeedSave;
		g_uDataSaveCount_4per += uLenBufNeedSave;
	}

	ADC_UNLOCK( pADCObj );
	return 0;
}
//
int audio_data_fetch( void* hadc, void* pBuf, unsigned int* puLenBuf ) {
    if ( !hadc || !pBuf || !puLenBuf )
		return -1;
    LLXAudioDataCache* pADCObj = (LLXAudioDataCache*)hadc;
	unsigned int uLenBufPrepare = *puLenBuf;
	unsigned int uLenBufCopy = 0;
	unsigned int uPosStartNew = 0;

    if ( uLenBufPrepare < SIZE_LLX_ADC_BUFFER ) {
		DPT( "error: uLenBufPrepare:%d is too small\n", uLenBufPrepare );
		return -1;
    }
	
	ADC_LOCK( pADCObj );
    if ( pADCObj->m_lenData == 0 ) {
        *puLenBuf = 0;
        pADCObj->m_lenData = 0;
        pADCObj->m_posStart = 0;
	} else {
        DPT( "Enter audio_data_fetch| posStart: %d, lenData: %d, uLenBufPrepare:%d\n", pADCObj->m_posStart, pADCObj->m_lenData, *puLenBuf );
		if ( pADCObj->m_posStart + pADCObj->m_lenData <= SIZE_LLX_ADC_BUFFER ) {
			RtlCopyMemory( pBuf, &(pADCObj->m_bufADC[ pADCObj->m_posStart ]), pADCObj->m_lenData );
		} else {
			unsigned int uSeg1 = SIZE_LLX_ADC_BUFFER - pADCObj->m_posStart;
			unsigned int uSeg2 = pADCObj->m_lenData - uSeg1;
			RtlCopyMemory( pBuf, &(pADCObj->m_bufADC[ pADCObj->m_posStart ]), uSeg1 );
			RtlCopyMemory( (unsigned char*)pBuf + uSeg1, &( pADCObj->m_bufADC[0]), uSeg2 );
		}
		*puLenBuf = pADCObj->m_lenData;	
        pADCObj->m_lenData = 0;
        pADCObj->m_posStart = 0;
        //
		g_uDataFetchCount += *puLenBuf;
		DPT( "Exit audio_data_fetch | g_uDataFetchCount:%d, posStart: %d, lenData: %d, uLenBufCopied:%d\n", g_uDataFetchCount, pADCObj->m_posStart, pADCObj->m_lenData, *puLenBuf );
	}

	ADC_UNLOCK( pADCObj );
	return 0;
}

int get1stBlockSizeNoLock( LLXAudioDataCache* pADC ) {
    if ( !pADC || ( pADC->m_lenData == 0 ) )
		return 0;
    unsigned char bufBlockSize[ LEN_BLOCK_SIZE ];
	
    if ( pADC->m_posStart <= SIZE_LLX_ADC_BUFFER - LEN_BLOCK_SIZE ) {
		RtlCopyMemory( bufBlockSize, &(pADC->m_bufADC[ pADC->m_posStart ]), LEN_BLOCK_SIZE );
	} else {
	    unsigned int uSeg1 = SIZE_LLX_ADC_BUFFER - pADC->m_posStart;
		unsigned int uSeg2 = LEN_BLOCK_SIZE - uSeg1;
        RtlCopyMemory( bufBlockSize, &(pADC->m_bufADC[ pADC->m_posStart ]), uSeg1 );
		RtlCopyMemory( &bufBlockSize[ uSeg1 ], &pADC->m_bufADC[ 0 ], uSeg2 );
	}
	
	return *((unsigned int*)bufBlockSize);
}

int flush1stBlockNoLock( LLXAudioDataCache* pADC ) {
	// if free buffer can not hold pBufNeedSave, flush some old data. 
	// so need modify pADCObj->m_posStart and pADCObj->m_lenData.
	// DPT( "######flush1stBlockNoLock\n" );
	unsigned int uSizeStartBlock = get1stBlockSizeNoLock( pADC );
	if ( uSizeStartBlock == 0 )
		return -1;
	unsigned int posTmp = pADC->m_posStart + ( LEN_BLOCK_SIZE + uSizeStartBlock );
	pADC->m_posStart = ( posTmp >= SIZE_LLX_ADC_BUFFER ) ? ( posTmp - SIZE_LLX_ADC_BUFFER ) : posTmp;
	pADC->m_lenData -= ( LEN_BLOCK_SIZE + uSizeStartBlock );
    
	return 0;
}

void flushAllBlockNoLock(LLXAudioDataCache* pADC) {
	//DPT( "flushAllBlockNoLock, m_posStart:%d, m_lenData:%d.", pADC->m_posStart, pADC->m_lenData );
	pADC->m_posStart = 0;
	pADC->m_lenData = 0;
}

int pushNewBlockSizeNoLock( LLXAudioDataCache* pADC, unsigned int uSizeNewBlock ) {
    unsigned int posTmp = 0;
	unsigned int posNewBlockSize = ( pADC->m_posStart + pADC->m_lenData ) % SIZE_LLX_ADC_BUFFER;
	
	if ( posNewBlockSize + LEN_BLOCK_SIZE <= SIZE_LLX_ADC_BUFFER ) {
		RtlCopyMemory( &(pADC->m_bufADC[posNewBlockSize]), (unsigned char*)&uSizeNewBlock, LEN_BLOCK_SIZE );
	} else {
		unsigned int uLenSeg1 = SIZE_LLX_ADC_BUFFER - posNewBlockSize;
		unsigned int uLenSeg2 = LEN_BLOCK_SIZE - uLenSeg1;
		RtlCopyMemory( &(pADC->m_bufADC[posNewBlockSize]), (unsigned char*)&uSizeNewBlock, uLenSeg1 );
		RtlCopyMemory( &(pADC->m_bufADC[0]), ((unsigned char*)&uSizeNewBlock) + uLenSeg1, uLenSeg2 );
	}
	
    return 0;
}

int pushNewBlockBufNoLock( LLXAudioDataCache* pADC, unsigned char* pBufBlockNew, unsigned int uSizeBufBlockNew ) {
	unsigned int posNewBlockBuf = ( pADC->m_posStart + pADC->m_lenData + LEN_BLOCK_SIZE ) % SIZE_LLX_ADC_BUFFER;
	
	if ( posNewBlockBuf + uSizeBufBlockNew <= SIZE_LLX_ADC_BUFFER ) {
		RtlCopyMemory( &(pADC->m_bufADC[posNewBlockBuf]), pBufBlockNew, uSizeBufBlockNew );
	} else {
		unsigned int uLenSeg1 = SIZE_LLX_ADC_BUFFER - posNewBlockBuf;
		unsigned int uLenSeg2 = uSizeBufBlockNew - uLenSeg1;
		RtlCopyMemory( &(pADC->m_bufADC[posNewBlockBuf]), pBufBlockNew, uLenSeg1 );
		RtlCopyMemory( &(pADC->m_bufADC[0]), pBufBlockNew + uLenSeg1, uLenSeg2 );
	}

    return 0;
}







