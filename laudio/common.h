////By Fanxiushu 2011-10-24

#ifndef _COMMON_H
#define _COMMON_H
 
#pragma warning (disable : 4127)

#include <initguid.h>
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

#define DMA_BUFFER_SIZE        88*1024 // 88K
////
#define SAFE_RELEASE(A) if(A) { (A)->Release(); (A) = NULL ;} 

#define VA_POOLTAG   'FXSD'

/////
#define ISVA_VERSION       1
#define ISVA_REVISION      0

// Product Id
// {BFD3A2D8-478D-4820-BDB4-3125F3B0310C}
#define STATIC_PID_ISVA\
    0xBFD3A2D8, 0x478D, 0x4820, 0xbd, 0xb4, 0x31, 0x25, 0xf3, 0xb0, 0x31, 0x0c
DEFINE_GUIDSTRUCT("BFD3A2D8-478D-4820-BDB4-3125F3B0310C", PID_ISVA);
#define PID_ISVA DEFINE_GUIDNAMED(PID_ISVA)

// Name Id
// {853157B5-909D-425c-9153-37325753FFC8}
#define STATIC_NAME_ISVA\
    0x853157B5, 0x909D, 0x425c, 0x91, 0x53, 0x37, 0x32, 0x57, 0x53, 0xff, 0xc8
DEFINE_GUIDSTRUCT("853157B5-909D-425c-9153-37325753FFC8", NAME_ISVA);
#define NAME_ISVA DEFINE_GUIDNAMED(NAME_ISVA)


///////////////////////////******************************/////////////////////
#ifdef DBG
#define DPT( fmt, ... ) DbgPrintEx ( DPFLTR_DEFAULT_ID, DPFLTR_INFO_LEVEL, fmt, __VA_ARGS__ )
#else
#define DPT   //
#endif

///Property
#define KSPROPERTY_TYPE_ALL         KSPROPERTY_TYPE_BASICSUPPORT | \
                                    KSPROPERTY_TYPE_GET | \
                                    KSPROPERTY_TYPE_SET

// Pin properties.
#define MAX_OUTPUT_STREAMS          1       // Number of capture streams.
#define MAX_INPUT_STREAMS           1       // Number of render streams.
#define MAX_TOTAL_STREAMS           MAX_OUTPUT_STREAMS + MAX_INPUT_STREAMS                      

// PCM Info
#define MIN_CHANNELS                1       // Min Channels.
#define MAX_CHANNELS_PCM            2       // Max Channels.
#define MIN_BITS_PER_SAMPLE_PCM     8       // Min Bits Per Sample
#define MAX_BITS_PER_SAMPLE_PCM     16      // Max Bits Per Sample
#define MIN_SAMPLE_RATE             4000    // Min Sample Rate
#define MAX_SAMPLE_RATE             64000   // Max Sample Rate

// Wave pins
enum 
{
    KSPIN_WAVE_CAPTURE_SINK = 0,
    KSPIN_WAVE_CAPTURE_SOURCE,
    KSPIN_WAVE_RENDER_SINK, 
    KSPIN_WAVE_RENDER_SOURCE
};

// Wave Topology nodes.
enum 
{
    KSNODE_WAVE_ADC = 0,
    KSNODE_WAVE_DAC
};

// topology pins.
enum
{
    KSPIN_TOPO_WAVEOUT_SOURCE = 0,
//    KSPIN_TOPO_SYNTHOUT_SOURCE,
//    KSPIN_TOPO_SYNTHIN_SOURCE,
    KSPIN_TOPO_MIC_SOURCE,
    KSPIN_TOPO_LINEOUT_DEST,
    KSPIN_TOPO_WAVEIN_DEST
};


// topology nodes.
enum
{
    KSNODE_TOPO_WAVEOUT_VOLUME = 0,
    KSNODE_TOPO_WAVEOUT_MUTE,
//    KSNODE_TOPO_SYNTHOUT_VOLUME,
//    KSNODE_TOPO_SYNTHOUT_MUTE,
    KSNODE_TOPO_MIC_VOLUME,
//    KSNODE_TOPO_SYNTHIN_VOLUME,
    KSNODE_TOPO_LINEOUT_MIX,
    KSNODE_TOPO_LINEOUT_VOLUME,
    KSNODE_TOPO_WAVEIN_MUX,


};
///////////////////////////**********************/////////////////////////

//function
PWAVEFORMATEX  GetWaveFormatEx( IN  PKSDATAFORMAT pDataFormat  );
NTSTATUS ValidateFormat(IN  PKSDATAFORMAT  pDataFormat);
NTSTATUS                        
PropertyHandler_BasicSupport (IN PPCPROPERTY_REQUEST  PropertyRequest,
    IN ULONG  Flags, IN DWORD  PropTypeSetId);
NTSTATUS 
ValidatePropertyParams
(
    IN PPCPROPERTY_REQUEST      PropertyRequest, 
    IN ULONG                    cbSize,
    IN ULONG                    cbInstanceSize /* = 0  */
);
////////////////

NTSTATUS
start_device
( 
    IN  PDEVICE_OBJECT          DeviceObject,     
    IN  PIRP                    Irp,              
    IN  PRESOURCELIST           ResourceList      
) ;

//// 
PVOID data_transfer_create();
void data_transfer_destroy(IN PVOID hdt);
int data_transfer_play( IN PVOID hdt, IN void* buf, IN int len );
int data_transfer_record( IN PVOID hdt, OUT void* buf, IN int len );

#endif //_COMMON_H

