
////////
////By Fanxiushu 2011-10-21
/// DDK 
#include "common.h"

#include <Ntifs.h>
#include <portcls.h>
#include <stdunk.h>
#include <ksdebug.h>
//实现播放数据copy到录音


struct _data_trans_t
{
	KSPIN_LOCK spin_lock; //同步内存
    KIRQL spin_irql; 
	////
	int rec_pos;
	int data_len;
    unsigned char rec_buf[ DMA_BUFFER_SIZE ]; 
};

///////
#define lock(dt)    KeAcquireSpinLock( &dt->spin_lock , &dt->spin_irql ); 
#define unlock(dt)  KeReleaseSpinLock( &dt->spin_lock,  dt->spin_irql ); 
///////

PVOID data_transfer_create()
{
	_data_trans_t* dt = (_data_trans_t*)ExAllocatePoolWithTag( 
		   NonPagedPool, sizeof( struct _data_trans_t ), VA_POOLTAG ); 
	if( !dt ){
		DPT("data_transfer_create() Error<No Memory>.\n");
		return NULL;
	}

	KeInitializeSpinLock( &dt->spin_lock ); 

	dt->rec_pos = dt->data_len = 0; 
	////
	return dt; 
}

void data_transfer_destroy(IN PVOID hdt)
{
	if( hdt)
		ExFreePoolWithTag( hdt, VA_POOLTAG ); 
}

int data_transfer_play( IN PVOID hdt, IN void* buf, IN int len )
{
//	ASSERT( hdt ); 
//	ASSERT( buf );
//	ASSERT( len > 0) ;
	////
	_data_trans_t* dt = (_data_trans_t*)hdt;
	/////
	if( len > DMA_BUFFER_SIZE ) {
		DPT("Shit Too Large Play Buffer.\n");
		return -1;
	}
	//////
	lock( dt );

	if( dt->rec_pos + dt->data_len + len > DMA_BUFFER_SIZE ){ // 新到的数据长度超出缓冲
		if( dt->data_len + len > DMA_BUFFER_SIZE ){//新到的数据和原来的数据总长度超过缓冲容量,清空原来数据，从新填充
			dt->rec_pos = 0; 
			RtlCopyMemory( dt->rec_buf, buf, len ); 
			dt->data_len = len; 
		}
		else{
			RtlMoveMemory( dt->rec_buf, dt->rec_buf + dt->rec_pos, dt->data_len ); 
			dt->rec_pos = 0; 
			RtlCopyMemory( dt->rec_buf + dt->data_len, buf, len );
			dt->data_len += len ; 
		}
	}
	else{
		RtlCopyMemory( dt->rec_buf + dt->rec_pos + dt->data_len, buf, len );
		dt->data_len += len ; 
	}
//	DPT("Handle=%p, Play_Buf=%p, Play_len=%d\n", hdt, buf, len ); 

	unlock( dt );
	////
	return 0; 
}


int data_transfer_record( IN PVOID hdt, OUT void* buf, IN int len )
{
//	ASSERT( hdt ); 
//	ASSERT( buf );
//	ASSERT( len > 0) ;
	/////
	_data_trans_t* dt = (_data_trans_t*)hdt;
	//////
	lock( dt );

	////
	if( dt->data_len <= len ){
		if( dt->data_len < len ) 
			RtlFillMemory( (unsigned char*)buf + dt->data_len, len - dt->data_len , 0 ); 
		if( dt->data_len > 0){
			RtlCopyMemory( buf, dt->rec_buf + dt->rec_pos, dt->data_len );
		}
		dt->rec_pos = 0 ;
		dt->data_len = 0; 
	}
	else{
		RtlCopyMemory( buf, dt->rec_buf + dt->rec_pos, len );
		dt->rec_pos  += len ; 
		dt->data_len -= len ;
	}
	/////
//	DPT("Handle=%p, Rec_Buf=%p, Rec_len=%d\n", hdt,buf,len);

	unlock( dt );

	return 0;
}

