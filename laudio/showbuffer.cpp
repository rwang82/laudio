#include "ShowBuffer.h"
#include "common.h"
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

#define SB_ROW_COUNT (8)
#define LEN_BUFTMP (SB_ROW_COUNT*5 + 1)
#define Hex2CHAR( val ) ( g_aryChar[val] )
char g_aryChar[ 16 ] = { '0', '1', '2', '3', '4', '5', '6', '7',
                         '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};


void ShowBuffer( void* pBuffer, unsigned long uLen ) {
    ULONG uMax = 2048;
    char bufTmp[ LEN_BUFTMP ];
	RtlFillMemory( bufTmp, LEN_BUFTMP, 0 );
	char cTmp = 0;
	DPT( "Buffer Len:%d, CutTo:%d:\n", uLen, uMax );
	for ( ULONG uIndex = 0; uIndex<uLen; ++uIndex ) {
		if ( uIndex >= uMax )
			break;
		ULONG uRow = uIndex%SB_ROW_COUNT;
        bufTmp[ uRow*5 ] = '0';
		bufTmp[ uRow*5 + 1 ] = 'x';
		cTmp = (*(((unsigned char*)pBuffer)+uIndex))>>4;
		if ( cTmp < 0 || cTmp >= 16 ) {
            DPT("cTmp out of range. :%d", cTmp);
		}
        bufTmp[ uRow*5 + 2 ] = Hex2CHAR( cTmp );
        cTmp = (*(((unsigned char*)pBuffer)+uIndex))&0x0F;
		if ( cTmp < 0 || cTmp >= 16 ) {
            DPT("cTmp out of range. :%d", cTmp);
		}
		bufTmp[ uRow*5 + 3 ] = Hex2CHAR(cTmp);
		bufTmp[ uRow*5 + 4 ] = ' ';
		
		if ( uRow == (SB_ROW_COUNT-1) ) {
			DPT(bufTmp);
	        RtlFillMemory( bufTmp, LEN_BUFTMP, 0 );
		}	
	}
	
	if ( bufTmp[0] != 0 ) {
		DPT( bufTmp );
	}

}






