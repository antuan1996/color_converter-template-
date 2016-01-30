#ifndef STRUCT_DEFINE
#define STRUCT_DEFINE

#ifdef LINUX_BUILD
#define TARGET_INLINE inline __attribute__((always_inline))
#define TARGET_MEMALIGN(align, size) memalign( (align), (size))
#define TARGET_FREEALIGN free
#elif defined WINDOWS_BUILD
#define TARGET_INLINE __forceinline
#define TARGET_MEMALIGN(align, size) memalign( (size), (align) )
#define TARGET_FREEALIGN _aligned_free
#endif

#define IS_RGB(cs) (cs == RGB24 || cs == BGR24 || cs == A2R10G10B10 || cs ==  A2B10G10R10 || cs == RGB32 || cs == BGR32)
#define IS_YUV(cs) (cs == YUV444 || cs == YUYV || cs == YVYU || cs == UYVY || cs == NV21 || cs == NV12 || cs == YV12 || cs == I420 || cs == YV16 || cs == YUV422 || cs == P210 || cs == P010)
#define IS_8BIT(cs) (cs == RGB24 || cs == BGR24 || cs == RGB32 || cs == BGR32 || cs == YUV444 || cs == YUYV || cs == YVYU || cs == UYVY || cs == NV21 || cs == NV12 || cs == YV12 || cs == I420 || cs == YV16 || cs == YUV422)
#define IS_10BIT(cs) (cs == A2B10G10R10 || cs == A2R10G10B10 || cs == P210 || cs == P010 || cs == Y210 || cs == V210)
#define IS_INTERLEAVED(cs) (cs == RGB24 || cs == BGR24 || cs == A2R10G10B10 || cs ==  A2B10G10R10 || cs == RGB32 || cs == BGR32 || cs == YUV444 || cs == YUYV || cs == YVYU || cs == UYVY || cs == V210)
#define IS_PLANAR(cs) (cs == YV12 || cs == I420 || cs == YV16)
#define IS_SEMIPLANAR( cs ) (cs == NV21 || cs == NV12)
#define IS_YUV422( cs ) (cs == YUYV || cs == YVYU || cs == UYVY || cs == YV16 || cs == YUV422 || cs == P210 || cs == YV16 || cs == Y210 || cs == V210)
#define IS_YUV420( cs ) (cs == NV21 || cs == NV12 || cs == YV12 || cs == I420 || cs == P010)
#define IS_YUV444( cs ) (cs == YUV444)


#define SHIFT_LEFT 1
#define SHIFT_RIGHT 0



#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <assert.h>

enum Colorspace
{
 //  name       pack     order       size

    YUYV,   // Interleaved Y->U->Y->V (4:2:2)
    YUY2 = YUYV,   // Interleaved Y->U->Y->V (4:2:2) (duplicate)
    YVYU,   // Interleaved Y->V->Y->U (4:2:2)
    UYVY,   // Interleaved U->Y->V->Y (4:2:2)
    Y210,   // Interleaved Y->U->Y->V (4:2:2) (10 bit p/s)

    RGB24,  // Standart interleaved RGB(8x8x8)
    BGR24,  // Interleaved BGR(8x8x8)
    RGB32,  // Interleaved RGBX(8x8x8x8)
    BGR32,  // Interleaved BGRX(8x8x8x8)

    A2R10G10B10, // Interleaved A(2 bit ) R(10bit) G(10bit) B(10 bit)
    A2B10G10R10, // Interleaved A(2 bit ) B(10bit) G(10bit) R(10 bit)

    NV21,   // Planar Y, merged V->U (4:2:0)
    NV12,   // Planar Y, merged U->V (4:2:0)
    P210,   // Planar Y, merged V->U (4:2:2)  10 bit
    P010,   // Planar Y, merged V->U (4:2:0)  10-bit

    YUV444, // Planar Y->U->V (4:4:4)
    YV12,   // Planar Y, V, U (4:2:0)
    I420,   // Planar Y, U, V (4:2:0)
    IYUV = I420,   // Planar Y, U, V (4:2:0) (duplicate)
    YV16,   // Planar Y, V, U (4:2:2)
    YUV422, // Planar Y, U, V (4:2:2)

    V210,   // Interleaved YUV422, 12  10bit values packed in 128bit
};
enum Standard
{
	BT_601,
	BT_709,
	BT_2020
};
#endif
