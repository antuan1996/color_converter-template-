#ifndef STRUCT_DEFINE
#define STRUCT_DEFINE



#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <malloc.h>

#ifdef LINUX_BUILD
//#define TARGET_INLINE inline __attribute__((always_inline))
#define TARGET_INLINE inline
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
#define IS_INTERLEAVED(cs) (cs == RGB24 || cs == BGR24 || cs == A2R10G10B10 || cs ==  A2B10G10R10 || cs == RGB32 || cs == BGR32  || cs == YUYV || cs == YVYU || cs == UYVY || cs == V210)
#define IS_PLANAR(cs) (cs == YV12 || cs == I420 || cs == YV16 || cs == YUV444)
#define IS_SEMIPLANAR( cs ) (cs == NV21 || cs == NV12)
#define IS_YUV422( cs ) (cs == YUYV || cs == YVYU || cs == UYVY || cs == YV16 || cs == YUV422 || cs == P210 || cs == YV16 || cs == Y210 || cs == V210)
#define IS_YUV420( cs ) (cs == NV21 || cs == NV12 || cs == YV12 || cs == I420 || cs == P010)
#define IS_YUV444( cs ) (cs == YUV444)


#define SHIFT_LEFT 1
#define SHIFT_RIGHT 0


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
static const int32_t k_bt601_RGB_to_YUV[3 * 3] =
{
    77,  150,  29,
    -44,  -87, 131,
    131, -110, -21,
};

static const int32_t e_matrix[3 * 3] =
{
    256, 0, 0,
    0, 256, 0,
    0, 0, 256
};

static const int32_t k_bt709_RGB_to_YUV[3 * 3] =
{
    54, 183, 18,
    -30, -101, 131,
    131, -119, -12
};

static const int32_t k_bt2020_RGB_to_YUV[3 * 3] =
{
    67, 174, 15,
    -37, -94, 131,
    131, -120, -10
};
static const int32_t k_bt601_YUV_to_RGB[3 * 3] =
{
    256,   0,  351,
    256, -86, -179,
    256, 444,    0
};

static const int32_t k_bt709_YUV_to_RGB[3 * 3] =
{
    256, 0 , 394,
    256, -47, -117,
    256, 465,    0
};
static const int32_t k_bt2020_YUV_to_RGB[3 * 3] =
{
    256, 0, 369,
    256, -41, -143,
    257, 471, 0
};

struct ConvertMeta
{
    ConvertMeta()
    {
        dst_data[ 0 ] = nullptr;
        dst_data[ 1 ] = nullptr;
        dst_data[ 2 ] = nullptr;
    }
    uint8_t *src_data[3];
    uint8_t *dst_data[3];
    size_t width;
    size_t height;
    size_t src_stride[3];
    size_t dst_stride_horiz[3];
    size_t dst_stride_vert[3];
};

template <Colorspace from_cs, Colorspace to_cs> void set_meta( ConvertMeta& meta , int width, int height, uint8_t* src_a, uint8_t* src_b = nullptr,uint8_t* src_c = nullptr)
{
    meta.width = width;
    meta.height = height;
    meta.src_data[ 0 ] = src_a;
    meta.src_data[ 1 ] = src_b;
    meta.src_data[ 2 ] = src_c;

    meta.src_stride[ 0 ] = meta.src_stride[ 1 ] = meta.src_stride[ 2 ] = 0;
    meta.dst_stride_horiz[ 0 ] = meta.dst_stride_horiz[ 1 ] = meta.dst_stride_horiz[ 2 ] = 0;
    meta.dst_stride_vert[ 0 ] = meta.dst_stride_vert[ 1 ] = meta.dst_stride_vert[ 2 ] = 0;
    if( meta.dst_data[ 0 ] != nullptr)
        TARGET_FREEALIGN( meta.dst_data[ 0 ]);
    meta.dst_data[ 0 ] = nullptr;
    meta.dst_data[ 1 ] = nullptr;
    meta.dst_data[ 2 ] = nullptr;
    if(from_cs == RGB24)
    {
        meta.src_stride[ 0 ] = width * 3;
    }
    if( from_cs == A2R10G10B10 || from_cs == A2B10G10R10 || from_cs == RGB32 || from_cs == BGR32)
    {
        meta.src_stride[ 0 ] = width * 4;
    }
    if(from_cs == YUV444)
    {
        meta.src_stride[ 0 ] = width;
        meta.src_stride[ 1 ] = width;
        meta.src_stride[ 2 ] = width;
    }
    if(from_cs == YUYV || from_cs == YVYU || from_cs == UYVY)
    {
        meta.src_stride[ 0 ] = width * 2;
    }
    if(from_cs == Y210)
    {
        meta.src_stride[ 0 ] = width * 2 * 2;
    }
    if(from_cs == NV21 || from_cs == NV12)
    {
        meta.src_stride[ 0 ] = width;
        meta.src_stride[ 1 ] = width;
    }
    if(from_cs == YV12 || from_cs == I420)
    {

        meta.src_stride[ 0 ] = width;
        meta.src_stride[ 1 ] = width / 2;
        meta.src_stride[ 2 ] = width / 2;

    }
    if(from_cs == YV16 || from_cs == YUV422)
    {

        meta.src_stride[ 0 ] = width;
        meta.src_stride[ 1 ] = width / 2;
        meta.src_stride[ 2 ] = width / 2;

    }
    if(from_cs == P210 || from_cs == P010)
    {
        meta.src_stride[ 0 ] = width * 2;
        meta.src_stride[ 1 ] = width * 2;
    }
    if(from_cs == V210)
    {
        // TODO align on 48 pixels
        meta.src_stride[ 0 ] = width * 16 / 6;
    }

    //destination settings

    if(to_cs == RGB24 ||  to_cs == BGR24)
    {
        meta.dst_stride_horiz[ 0 ] = width * 3;
        meta.dst_stride_vert[0] = height;
    }
    if(to_cs == RGB32 ||  to_cs == BGR32)
    {
        meta.dst_stride_horiz[ 0 ] = width * 4;
        meta.dst_stride_vert[0] = height;
    }

    if(to_cs == A2R10G10B10)
    {
         meta.dst_stride_horiz[ 0 ] = width * 4;
         meta.dst_stride_vert[ 0 ] = height;
    }
    if(to_cs == YUV444)
    {
        meta.dst_stride_horiz[ 0 ] = width;
        meta.dst_stride_horiz[ 1 ] = width;
        meta.dst_stride_horiz[ 2 ] = width;

        meta.dst_stride_vert[ 0 ] = height;
        meta.dst_stride_vert[ 1 ] = height;
        meta.dst_stride_vert[ 2 ] = height;

    }
    if(to_cs == YUYV || to_cs == YVYU || to_cs == UYVY)
    {
         meta.dst_stride_horiz[ 0 ] = width * 2;
         meta.dst_stride_vert[ 0 ] = height;
    }
    if(to_cs == Y210)
    {
         meta.dst_stride_horiz[ 0 ] = width * 4;
         meta.dst_stride_vert[0] = height;
    }
    if(to_cs == NV21 || to_cs == NV12)
    {
         meta.dst_stride_horiz[ 0 ] = width;
         meta.dst_stride_horiz[ 1 ] = width;

         meta.dst_stride_vert[0] = height;
         meta.dst_stride_vert[1] = height / 2;
    }
    if(to_cs == YV12 || to_cs == I420)
    {

         meta.dst_stride_horiz[ 0 ] = width;
         meta.dst_stride_horiz[ 1 ] = width / 2;
         meta.dst_stride_horiz[ 2 ] = width / 2;


         meta.dst_stride_vert[0] = height;
         meta.dst_stride_vert[1] = height / 2;
         meta.dst_stride_vert[2] = height / 2;

    }
    if(to_cs == YV16 || to_cs == YUV422)
    {

         meta.dst_stride_horiz[ 0 ] = width;
         meta.dst_stride_horiz[ 1 ] = width / 2;
         meta.dst_stride_horiz[ 2 ] = width / 2;

         meta.dst_stride_vert[ 0 ] = height;
         meta.dst_stride_vert[ 1 ] = height;
         meta.dst_stride_vert[ 2 ] = height;

    }
    if(to_cs == P210 || to_cs == P010)
    {
         meta.dst_stride_horiz[ 0 ] = width * 2;
         meta.dst_stride_horiz[ 1 ] = width * 2;
    }
    if(to_cs == P210)
    {
        meta.dst_stride_vert[ 0 ] = height;
        meta.dst_stride_vert[ 1 ] = height;

    }
    if(to_cs == P010)
    {
        meta.dst_stride_vert[ 0 ] = height;
        meta.dst_stride_vert[ 1 ] = height / 2;
    }
    if(to_cs == V210)
    {
         meta.dst_stride_horiz[ 0 ] = width * 16 / 6;
         meta.dst_stride_vert[ 0 ] = height;

    }
    int siz = 0;
    siz += meta.dst_stride_horiz[ 0 ] * meta.dst_stride_vert[ 0 ];
    siz += meta.dst_stride_horiz[ 1 ] * meta.dst_stride_vert[ 1 ];
    siz += meta.dst_stride_horiz[ 2 ] * meta.dst_stride_vert[ 2 ];

    meta.dst_data[ 0 ] = (uint8_t*) TARGET_MEMALIGN( 16, siz );
    memset(meta.dst_data[0], 0, siz);

    meta.dst_data[ 1 ] = meta.dst_data[ 0 ] + meta.dst_stride_horiz[ 0 ] * meta.dst_stride_vert[ 0 ];
    meta.dst_data[ 2 ] = meta.dst_data[ 1 ] + meta.dst_stride_horiz[ 1 ] * meta.dst_stride_vert[ 1 ];
}

template< Colorspace cs > inline void get_pos(size_t& posa, size_t& posb, size_t& posc, const int cur_pos)
{
    posa = 0;
    posb = 0;
    posc = 0;
    if (cs == RGB24 || cs == BGR24)
    {
        posa = cur_pos * 3;
        return;
    }
    if( cs == YUV444)
    {
        posa = cur_pos;
        posb = cur_pos;
        posc = cur_pos;

    }
    if (cs == A2R10G10B10 || cs == A2B10G10R10 || cs == RGB32 || cs == BGR32) {
        posa = cur_pos * 4;
    }
    if( cs == YUYV || cs == YVYU || cs == UYVY )
    {
        posa =cur_pos * 2;
    }
    if(cs == YUV422 || cs == YV16 || cs == I420 || cs == YV12)
    {
        // Y Y
        // Y Y
        //U
        //*U
        //V
        //*V
        posa = cur_pos;
        posb = cur_pos / 2;
        posc = cur_pos / 2;
    }
    if(cs == NV21 || cs == NV12)
    {
        // Y Y
        // Y Y
        //U V
        posa = cur_pos;
        posb = cur_pos;

    }
    if(cs == P210 || cs == P010)
    {
        posa = cur_pos * 2;
        posb = cur_pos * 2;
    }
    if(cs == Y210)
    {
        posa = cur_pos * 2 * 2;
    }
    if(cs == V210)
    {
        posa = cur_pos * 16 / 6;
    }

}

#endif
