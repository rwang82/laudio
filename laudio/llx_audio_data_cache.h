#ifndef __LLX_AUDIO_DATA_CACHE_H__
#define __LLX_AUDIO_DATA_CACHE_H__

void* adcobj_create();
void adcobj_destroy( void* hadc );
int audio_data_save( void* hadc, void* pBufNeedSave, unsigned int uLenBufNeedSave );
int audio_data_fetch( void* hadc, void* pBuf, unsigned int* puLenBuf );
void audio_data_clear( void* hadc );




#endif //__LLX_AUDIO_DATA_CACHE_H__



