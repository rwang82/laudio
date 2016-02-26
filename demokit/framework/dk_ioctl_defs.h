#ifndef __LLXAFIOCTRLDEFS_H__
#define __LLXAFIOCTRLDEFS_H__

#ifndef METHOD_BUFFERED
	#define METHOD_BUFFERED                 0
	#define METHOD_IN_DIRECT                1
	#define METHOD_OUT_DIRECT               2
	#define METHOD_NEITHER                  3
#endif //METHOD_BUFFERED

#define FILE_DEVICE_UNKNOWN             0x00000022

#define CTL_CODE( DeviceType, Function, Method, Access ) \
	( ((unsigned int)DeviceType)<<16 \
    | ((unsigned int)Access&0x3)<<14 \
	| ((unsigned int)Function&0xFFF)<<2 \
    | ((unsigned int)Method&0x3))

#define IOCTL_AUDIO_FETCH                          CTL_CODE( FILE_DEVICE_UNKNOWN, 0x802, METHOD_OUT_DIRECT, 0x03 )
#define IOCTL_AUDIO_CLEAR                          CTL_CODE( FILE_DEVICE_UNKNOWN, 0x803, METHOD_OUT_DIRECT, 0x03 )
#define IOCTL_MIC_PUSH                             CTL_CODE( FILE_DEVICE_UNKNOWN, 0x804, METHOD_IN_DIRECT, 0x03 )
#define IOCTL_FETCH_MICDATA_START_EVENT            CTL_CODE( FILE_DEVICE_UNKNOWN, 0x805, METHOD_OUT_DIRECT, 0x03 )
#define IOCTL_FETCH_MICSLEEP_EVENT                 CTL_CODE( FILE_DEVICE_UNKNOWN, 0x806, METHOD_OUT_DIRECT, 0x03 )
#define IOCTL_FETCH_MICDATA_STATE                  CTL_CODE( FILE_DEVICE_UNKNOWN, 0x807, METHOD_OUT_DIRECT, 0x03 )



#endif //__LLXAFIOCTRLDEFS_H__