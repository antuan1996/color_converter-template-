#ifndef COLOR_CONVERT
#define COLOR_CONVERT

#define IS_RGB(cs) (cs == RGB24 || cs == BGR24 || cs == A2R10G10B10 || cs ==  A2B10G10R10 || cs == RGB32 || cs == BGR32)
#define IS_YUV(cs) (cs == YUV444 || cs == YUYV || cs == YVYU || cs == UYVY || cs == NV21 || cs == NV12 || cs == YV12 || cs == I420 || cs == YV16 || cs == YUV422 || cs == P210 || cs == P010)
#define IS_8BIT(cs) (cs == RGB24 || cs == BGR24|| cs == YUV444 || cs == YUYV || cs == YVYU || cs == UYVY || cs == NV21 || cs == NV12 || cs == YV12 || cs == I420 || cs == YV16 || cs == YUV422)
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

namespace ColorspaceConverter {

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

/*
 * p1 p2
 * p3 p4
 */
struct Context
{
	int32_t a1, b1, c1;
	int32_t a2, b2, c2;
	int32_t a3, b3, c3;
	int32_t a4, b4, c4;
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

    int32_t offset_y_from;
    int32_t offset_u_from;
    int32_t offset_v_from;


    int32_t offset_y_to;
    int32_t offset_u_to;
    int32_t offset_v_to;

    size_t src_stride[3];
    size_t dst_stride_horiz[3];
    size_t dst_stride_vert[3];

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

template <Colorspace from, Colorspace to> inline void scale(int32_t& val_a, int32_t& val_b, int32_t& val_c)
{
    if( (IS_8BIT(from) &&(IS_8BIT(to))) || (IS_10BIT(from) && IS_10BIT(to)) )
        return;
    if(IS_8BIT(from) && IS_10BIT(to))
    {
        val_a <<= 2;
        val_b <<= 2;
        val_c <<= 2;
        return;
    }
    if(IS_10BIT(from) && IS_8BIT(to))
    {
        val_a >>= 2;
        val_b >>= 2;
        val_c >>= 2;
        return;
    }
    assert(0);
    //TODO unsupported formats
}
template <Colorspace from_cs, Colorspace to_cs> inline void init_offset_yuv( ConvertMeta& meta )
{

    // y+= 16 - result of range manipulation
    meta.offset_y_from = 16;
    meta.offset_u_from = 128;
    meta.offset_v_from = 128;
    if(IS_YUV(from_cs))
        scale < YUV444, from_cs> (meta.offset_y_from, meta.offset_u_from, meta.offset_v_from);

    meta.offset_y_to = 16;
    meta.offset_u_to = 128;
    meta.offset_v_to = 128;
    if(IS_YUV(to_cs))
        scale < YUV444, to_cs> (meta.offset_y_to, meta.offset_u_to, meta.offset_v_to);

}

template <Colorspace from_cs, Colorspace to_cs> void set_meta( ConvertMeta& meta , int width, int height, uint8_t* src_a, uint8_t* src_b = nullptr,uint8_t* src_c = nullptr)
{
    meta.width = width;
    meta.height = height;
    meta.src_data[ 0 ] = src_a;
    meta.src_data[ 1 ] = src_b;
    meta.src_data[ 2 ] = src_c;
    init_offset_yuv< from_cs,  to_cs >( meta );

    meta.src_stride[ 0 ] = meta.src_stride[ 1 ] = meta.src_stride[ 2 ] = 0;
    meta.dst_stride_horiz[ 0 ] = meta.dst_stride_horiz[ 1 ] = meta.dst_stride_horiz[ 2 ] = 0;
    meta.dst_stride_vert[ 0 ] = meta.dst_stride_vert[ 1 ] = meta.dst_stride_vert[ 2 ] = 0;
    if( meta.dst_data[ 0 ] != nullptr)
        free( meta.dst_data[ 0 ]);
    meta.dst_data[ 0 ] = nullptr;
    meta.dst_data[ 1 ] = nullptr;
    meta.dst_data[ 2 ] = nullptr;
    if(from_cs == RGB24)
    {
        meta.src_stride[ 0 ] = width * 3;
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

    if(to_cs == RGB24)
    {
        meta.dst_stride_horiz[ 0 ] = width * 3;
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

    meta.dst_data[ 0 ] = (uint8_t*) malloc( siz );
    memset(meta.dst_data[0], 0, siz);

    meta.dst_data[ 1 ] = meta.dst_data[ 0 ] + meta.dst_stride_horiz[ 0 ] * meta.dst_stride_vert[ 0 ];
    meta.dst_data[ 2 ] = meta.dst_data[ 1 ] + meta.dst_stride_horiz[ 1 ] * meta.dst_stride_vert[ 1 ];
}

template<Colorspace cs> inline void get_pos(size_t& posa, size_t& posb, size_t& posc, const int cur_pos)
{
    posa = 0;
    posb = 0;
    posc = 0;
    if (cs == RGB24 || cs == BGR24 )
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

// TO DO new transform matrix
template <Colorspace from, Colorspace to, Standard st> void set_transform_coeffs(int32_t* res_matrix)
{
   const int32_t* matrix = e_matrix;
	if (st == BT_601) {
        if ( IS_RGB( from ) && IS_YUV( to ) ) {
			matrix =  k_bt601_RGB_to_YUV;
		}
        else
        if ( IS_YUV( from ) &&  IS_RGB( to ) ) {
			matrix =  k_bt601_YUV_to_RGB;
		}
	}
    else
    if (st == BT_709) {
        if ( IS_RGB( from ) && IS_YUV( to ) ) {
            matrix =  k_bt709_RGB_to_YUV;
        }
        else
        if ( IS_YUV( from ) &&  IS_RGB( to ) ) {
            matrix =  k_bt709_YUV_to_RGB;
        }
	}
    else
    if (st == BT_2020) {
        if ( IS_RGB( from ) && IS_YUV( to ) ) {
            matrix =  k_bt2020_RGB_to_YUV;
        }
        else
        if ( IS_YUV( from ) &&  IS_RGB( to ) ) {
            matrix =  k_bt2020_YUV_to_RGB;
        }
    }
    // copying from const to mutable
    for(int r=0; r < 3; ++r)
    {
       res_matrix[r*3 + 0] = matrix[ r*3 + 0 ];
       res_matrix[r*3 + 1] = matrix[ r*3 + 1 ];
       res_matrix[r*3 + 2] = matrix[ r*3 + 2 ];
    }
}
static inline void unpack_V210(Context& ctx1, Context& ctx2, Context& ctx3)
{
    uint32_t data1 = ctx1.a1;
    uint32_t data2 = ctx1.b1;
    uint32_t data3 = ctx1.a2;
    uint32_t data4 = ctx1.b2;

    ctx1.b1 = data1 & 0X3FF;
    ctx1.a1 = (data1 >> 10) & 0X3FF;
    ctx1.c1 = (data1 >> 20) & 0X3FF;

    ctx1.a2 = data2 & 0X3FF;
    ctx2.b1 = (data2 >> 10) & 0X3FF;
    ctx2.a1 = (data2 >> 20) & 0X3FF;

    ctx2.c1 = data3 & 0X3FF;
    ctx2.a2 = (data3 >> 10) & 0X3FF;
    ctx3.b1 = (data3 >> 20) & 0X3FF;

    ctx3.a1 = data4 & 0X3FF;
    ctx3.c1 = (data4 >> 10) & 0X3FF;
    ctx3.a2 = (data4 >> 20) & 0X3FF;

    data1 = ctx1.a3;
    data2 = ctx1.b3;
    data3 = ctx1.a4;
    data4 = ctx1.b4;

    ctx1.b3 = data1 & 0X3FF;
    ctx1.a3 = (data1 >> 10) & 0X3FF;
    ctx1.c3 = (data1 >> 20) & 0X3FF;

    ctx1.a4 = data2 & 0X3FF;
    ctx2.b3 = (data2 >> 10) & 0X3FF;
    ctx2.a3 = (data2 >> 20) & 0X3FF;

    ctx2.c3 = data3 & 0X3FF;
    ctx2.a4 = (data3 >> 10) & 0X3FF;
    ctx3.b3 = (data3 >> 20) & 0X3FF;

    ctx3.a3 = data4 & 0X3FF;
    ctx3.c3 = (data4 >> 10) & 0X3FF;
    ctx3.a4 = (data4 >> 20) & 0X3FF;

}
static inline uint32_t pack10_in_int(int32_t a, int32_t b, int32_t c)
{
    // xx aaaaaaaaaa bbbbbbbbbb cccccccccc
    return ((a & 0X3FF) << 20) | ((b & 0X3FF) << 10) | (c & 0X3FF);
}

static inline void pack_V210(Context& ctx1, Context& ctx2, Context& ctx3)
{
    //YUV422
    // Y0 U0 Y1 V0      Y2 U1 Y3 V1     Y4 U2 Y5 V2

    // v210:
    // V0 Y0 U0 - w1
    // Y2 U1 Y1 - w2
    // U2 Y3 V1 - w3
    // Y5 V2 Y4 - w4

    ctx1.a1 = pack10_in_int( ctx1.c1, ctx1.a1, ctx1.b1 );
    ctx1.b1 = pack10_in_int( ctx2.a1, ctx2.b1, ctx1.a2 );
    ctx1.a2 = pack10_in_int( ctx3.b1, ctx2.a2, ctx2.c1 );
    ctx1.b2 = pack10_in_int( ctx3.a2, ctx3.c1, ctx3.a1 );

    ctx1.a3 = pack10_in_int( ctx1.c3, ctx1.a3, ctx1.b3 );
    ctx1.b3 = pack10_in_int( ctx2.a3, ctx2.b3, ctx1.a4 );
    ctx1.a4 = pack10_in_int( ctx3.b3, ctx2.a4, ctx2.c3 );
    ctx1.b4 = pack10_in_int( ctx3.a4, ctx3.c3, ctx3.a3 );
}

static inline void unpack10_from_int(int32_t& vala, int32_t& valb, int32_t& valc, const uint32_t buf)
{
            // xx aaaaaaaaaa bbbbbbbbbb cccccccccc
            valc = (buf) & 0x3FF;
            valb = (buf >> 10) & 0x3FF;
            vala = (buf >> 20) & 0x3FF;
}
template < Colorspace cs> inline void load(ConvertMeta& meta, Context& ctx, const uint8_t *srca, const uint8_t *srcb, const uint8_t *srcc)
{
    const uint8_t* src_na = srca + meta.src_stride[0];
    const uint8_t* src_nb = srcb + meta.src_stride[1];
    const uint8_t* src_nc = srcc + meta.src_stride[2];
    if(cs == YUV444)
    {
        ctx.a1 = srca[0];
        ctx.b1 = srcb[0];
        ctx.c1 = srcc[0];

        ctx.a2 = srca[1];
        ctx.b2 = srcb[1];
        ctx.c2 = srcc[1];

        ctx.a3 = src_na[0];
        ctx.b3 = src_nb[0];
        ctx.c3 = src_nc[0];

        ctx.a4 = src_na[1];
        ctx.b4 = src_nb[1];
        ctx.c4 = src_nc[1];

    }
    if(cs == RGB24){
        ctx.a1 = srca[0];
        ctx.b1 = srca[1];
        ctx.c1 = srca[2];

        ctx.a2 = srca[3];
        ctx.b2 = srca[4];
        ctx.c2 = srca[5];

        ctx.a3 = src_na[0];
        ctx.b3 = src_na[1];
        ctx.c3 = src_na[2];

        ctx.a4 = src_na[3];
        ctx.b4 = src_na[4];
        ctx.c4 = src_na[5];
    }
    if(cs == BGR24){
        ctx.c1 = srca[0];
        ctx.b1 = srca[1];
        ctx.a1 = srca[2];

        ctx.c2 = srca[3];
        ctx.b2 = srca[4];
        ctx.a2 = srca[5];

        ctx.c3 = src_na[0];
        ctx.b3 = src_na[1];
        ctx.a3 = src_na[2];

        ctx.c4 = src_na[3];
        ctx.b4 = src_na[4];
        ctx.a4 = src_na[5];
    }

    if(cs == YUYV ){
        ctx.a1 = srca[0];
        ctx.b1 = srca[1];
        ctx.c1 = srca[3];

        ctx.a2 = srca[2];

        ctx.a3 = src_na[0];
        ctx.b3 = src_na[1];
        ctx.c3 = src_na[3];

        ctx.a4 = src_na[2];
    }
    if( cs == YVYU )
    {
        ctx.a1 = srca[0];
        ctx.b1 = srca[3];
        ctx.c1 = srca[1];

        ctx.a2 = srca[2];

        ctx.a3 = src_na[0];
        ctx.b3 = src_na[3];
        ctx.c3 = src_na[1];

        ctx.a4 = src_na[2];

    }
    if( cs == UYVY)
    {
        ctx.a1 = srca[1];
        ctx.b1 = srca[0];
        ctx.c1 = srca[2];

        ctx.a2 = srca[3];

        ctx.a3 = src_na[1];
        ctx.b3 = src_na[0];
        ctx.c3 = src_na[2];

        ctx.a4 = src_na[3];

    }
    if(cs == A2R10G10B10 || cs == A2B10G10R10 || cs == RGB32 || cs == BGR32)
    {
        ctx.a1 = *(uint32_t*)(srca);
        ctx.a2 = *(uint32_t*)(srca + 4);
        ctx.a3 = *(uint32_t*)(src_na);
        ctx.a4 = *(uint32_t*)(src_na + 4);
    }
    if(cs == I420 || cs == YV12)
    {
        ctx.a1 = srca[0];
        ctx.a2 = srca[1];
        ctx.a3 = src_na[0];
        ctx.a4 = src_na[1];
    }
    if(cs == I420)
    {
        ctx.b1 = srcb[0];
        ctx.c1 = srcc[0];
    }
    if( cs  == YV12)
    {
        ctx.b1 = srcc[0];
        ctx.c1 = srcb[0];
    }
    if( cs == YV16 || cs == YUV422 )
    {
        ctx.a1 = srca[0];
        ctx.a2 = srca[1];
        ctx.a3 = src_na[0];
        ctx.a4 = src_na[1];
    }
    if(cs == YV16)
    {
        ctx.b1 = srcc[0];
        ctx.b3 = src_nc[0];
        ctx.c1 = srcb[0];
        ctx.c3 = src_nb[0];
    }
    if(cs == YUV422)
    {
        ctx.b1 = srcb[0];
        ctx.b3 = src_nb[0];
        ctx.c1 = srcc[0];
        ctx.c3 = src_nc[0];
    }
    if( cs == NV21 || cs == NV12)
    {
        ctx.a1 = srca[0];
        ctx.a2 = srca[1];
        ctx.a3 = src_na[0];
        ctx.a4 = src_na[1];
    }
    if(cs == NV12)
    {
        ctx.b1 = srcb[0];
        ctx.c1 = srcb[1];
    }
    if(cs == NV21)
    {
        ctx.b1 = srcb[1];
        ctx.c1 = srcb[0];
    }
    if(cs == Y210)
    {
        ctx.a1 = *(uint16_t*)(srca);
        ctx.b1 = *(uint16_t*)(srca + 2);
        ctx.a2 = *(uint16_t*)(srca + 4 );
        ctx.c1 = *(uint16_t*)(srca + 6);

        ctx.a3 = *(uint16_t*)(src_na);
        ctx.b3 = *(uint16_t*)(src_na + 2);
        ctx.a4 = *(uint16_t*)(src_na + 4 );
        ctx.c3 = *(uint16_t*)(src_na + 6);

    }
    if(cs == P210 || cs == P010)
    {
        ctx.a1 = *(uint16_t*)(srca);
        ctx.a2 = *(uint16_t*)(srca + 2);
        ctx.a3 = *(uint16_t*)(src_na);
        ctx.a4 = *(uint16_t*)(src_na + 2);

        ctx.b1 = *(uint16_t*)(srcb);
        ctx.c1 = *(uint16_t*)(srcb + 2);
    }
    if(cs == P210)
    {
        ctx.b3 = *(uint16_t*)(src_nb);
        ctx.c3 = *(uint16_t*)(src_nb + 2);
    }
    if(cs == V210)
    {
        ctx.a1 = *(uint32_t*)(srca);
        ctx.b1 = *(uint32_t*)(srca + 4);
        ctx.a2 = *(uint32_t*)(srca + 8);
        ctx.b2 = *(uint32_t*)(srca + 12);

        ctx.a3 = *(uint32_t*)(src_na);
        ctx.b3 = *(uint32_t*)(src_na + 4);
        ctx.a4 = *(uint32_t*)(src_na + 8);
        ctx.b4 = *(uint32_t*)(src_na + 12);
    }
}

template < Colorspace cs> inline void unpack_finish (Context& ctx)
{
    if(IS_YUV422( cs ))
    {
        ctx.c2 = ctx.c1;
        ctx.b2 = ctx.b1;

        ctx.c4 = ctx.c3;
        ctx.b4 = ctx.b3;

    }
    if(IS_YUV420( cs ))
    {
        ctx.b2 = ctx.b1;
        ctx.c2 = ctx.c1;

        ctx.b3 = ctx.b1;
        ctx.c3 = ctx.c1;

        ctx.b4 = ctx.b1;
        ctx.c4 = ctx.c1;
    }

}

template < Colorspace cs> inline void unpack (Context& ctx, Context& ctx2, Context& ctx3 )
{

    if(cs == Y210 || cs == P210 || cs == P010)
    {
        ctx.a1 >>= 6;
        ctx.a2 >>= 6;
        ctx.a3 >>= 6;
        ctx.a4 >>= 6;

        ctx.b1 >>= 6;
        ctx.c1 >>= 6;

    }
    if(cs == RGB32)
    {
        ctx.c1 = (ctx.a1 >> 8) & 0XFF;
        ctx.b1 = (ctx.a1 >> 16) & 0XFF;
        ctx.a1 = (ctx.a1 >> 24) & 0XFF;

        ctx.c2 = (ctx.a2 >> 8) & 0XFF;
        ctx.b2 = (ctx.a2 >> 16) & 0XFF;
        ctx.a2 = (ctx.a2 >> 24) & 0XFF;

        ctx.c3 = (ctx.a3 >> 8) & 0XFF;
        ctx.b3 = (ctx.a3 >> 16) & 0XFF;
        ctx.a3 = (ctx.a3 >> 24) & 0XFF;
    }
    if(cs == BGR32)
    {

        ctx.c1 = (ctx.a1 >> 24) & 0XFF;
        ctx.b1 = (ctx.a1 >> 16) & 0XFF;
        ctx.a1 = (ctx.a1 >> 8) & 0XFF;

        ctx.c2 = (ctx.a2 >> 24) & 0XFF;
        ctx.b2 = (ctx.a2 >> 16) & 0XFF;
        ctx.a2 = (ctx.a2 >> 8) & 0XFF;

        ctx.c3 = (ctx.a3 >> 24) & 0XFF;
        ctx.b3 = (ctx.a3 >> 16) & 0XFF;
        ctx.a3 = (ctx.a3 >> 8) & 0XFF;
    }
    if(cs == P210 || cs == Y210)
    {
        ctx.b3 >>= 6;
        ctx.c3 >>= 6;

    }
    if(cs == V210)
    {
        unpack_V210(ctx, ctx2, ctx3);
        unpack_finish< cs >( ctx );
        unpack_finish< cs >( ctx2 );
        unpack_finish< cs >( ctx3 );
        return;
    }
    if(cs == A2R10G10B10)
    {
        unpack10_from_int(ctx.a1, ctx.b1, ctx.c1, ctx.a1);
        unpack10_from_int(ctx.a2, ctx.b2, ctx.c2, ctx.a2);
        unpack10_from_int(ctx.a3, ctx.b3, ctx.c3, ctx.a3);
        unpack10_from_int(ctx.a4, ctx.b4, ctx.c4, ctx.a4);
    }
    if(cs == A2B10G10R10)
    {
        unpack10_from_int(ctx.c1, ctx.b1, ctx.a1, ctx.a1);
        unpack10_from_int(ctx.c2, ctx.b2, ctx.a2, ctx.a2);
        unpack10_from_int(ctx.c3, ctx.b3, ctx.a3, ctx.a3);
        unpack10_from_int(ctx.c4, ctx.b4, ctx.a4, ctx.a4);
    }

    unpack_finish < cs > ( ctx );
}

template < Colorspace cs > inline void store(const ConvertMeta& meta, Context& ctx,  uint8_t* dsta, uint8_t* dstb, uint8_t* dstc)
{
    uint8_t* dst_na = dsta + meta.dst_stride_horiz[0];
    uint8_t* dst_nb = dstb + meta.dst_stride_horiz[1];
    uint8_t* dst_nc = dstc + meta.dst_stride_horiz[2];

    if(cs == RGB24)
    {
        dsta[0] = ctx.a1;
        dsta[1] = ctx.b1;
        dsta[2] = ctx.c1;

        dsta[3] = ctx.a2;
        dsta[4] = ctx.b2;
        dsta[5] = ctx.c2;

        dst_na[0] = ctx.a3;
        dst_na[1] = ctx.b3;
        dst_na[2] = ctx.c3;

        dst_na[3] = ctx.a4;
        dst_na[4] = ctx.b4;
        dst_na[5] = ctx.c4;
        return;
    }
    if(cs == BGR24)
    {
        dsta[0] = ctx.c1;
        dsta[1] = ctx.b1;
        dsta[2] = ctx.a1;

        dsta[3] = ctx.c2;
        dsta[4] = ctx.b2;
        dsta[5] = ctx.a2;

        dst_na[0] = ctx.c3;
        dst_na[1] = ctx.b3;
        dst_na[2] = ctx.a3;

        dst_na[3] = ctx.c4;
        dst_na[4] = ctx.b4;
        dst_na[5] = ctx.a4;
        return;
    }

    if(cs == YUV444)
    {
        dsta[0] = ctx.a1;
        dsta[1] = ctx.a2;
        dst_na[0] = ctx.a3;
        dst_na[1] = ctx.a4;

        dstb[0] = ctx.b1;
        dstb[1] = ctx.b2;
        dst_nb[0] = ctx.b3;
        dst_nb[1] = ctx.b4;

        dstc[0] = ctx.c1;
        dstc[1] = ctx.c2;
        dst_nc[0] = ctx.c3;
        dst_nc[1] = ctx.c4;

        return;
    }

    if(cs == A2R10G10B10 || cs == A2B10G10R10 || cs == RGB32 || cs == BGR32)
    {
        memcpy(dsta, &ctx.a1, 4);
        memcpy(dsta + 4, &ctx.a2, 4);
        memcpy(dst_na, &ctx.a3, 4);
        memcpy(dst_na + 4, &ctx.a4, 4);
    }
    if( cs == YUYV )
    {
        dsta[0] = ctx.a1;
        dsta[1] = ctx.b1;

        dsta[2] = ctx.a2;
        dsta[3] = ctx.c1;

        dst_na[0] = ctx.a3;
        dst_na[1] = ctx.b3;

        dst_na[2] = ctx.a4;
        dst_na[3] = ctx.c3;
    }
    if( cs == YVYU)
    {
        dsta[0] = ctx.a1;
        dsta[1] = ctx.c1;

        dsta[2] = ctx.a2;
        dsta[3] = ctx.b1;

        dst_na[0] = ctx.a3;
        dst_na[1] = ctx.c3;

        dst_na[2] = ctx.a4;
        dst_na[3] = ctx.b3;
    }
    if(cs == UYVY )
    {
        dsta[0] = ctx.b1;
        dsta[1] = ctx.a1;

        dsta[2] = ctx.c1;
        dsta[3] = ctx.a2;

        dst_na[0] = ctx.b3;
        dst_na[1] = ctx.a3;

        dst_na[2] = ctx.c3;
        dst_na[3] = ctx.a4;
    }
    if(cs == I420 || cs == YV12)
    {
        dsta[0] = ctx.a1;
        dsta[1] = ctx.a2;
        dst_na[0] = ctx.a3;
        dst_na[1] = ctx.a4;
    }
    if(cs == I420)
    {
        dstb[0] = ctx.b1;
        dstc[0] = ctx.c1;
    }
    if(cs == YV12)
    {
        dstb[0] = ctx.c1;
        dstc[0] = ctx.b1;
    }
    if(cs == YV16 || cs == YUV422)
    {
        dsta[0] = ctx.a1;
        dsta[1] = ctx.a2;
        dst_na[0] = ctx.a3;
        dst_na[1] = ctx.a4;
    }
    if(cs == YV16)
    {
        dstb[0] = ctx.c1;
        dst_nb[0] = ctx.c3;
        dstc[0] = ctx.b1;
        dst_nc[0] = ctx.b3;
    }
    if(cs == YUV422)
    {
        dstb[0] = ctx.b1;
        dst_nb[0] = ctx.b3;
        dstc[0] = ctx.c1;
        dst_nc[0] = ctx.c3;
    }
    if(cs == NV21 || cs == NV12)
    {
        dsta[0] = ctx.a1;
        dsta[1] = ctx.a2;
        dst_na[0] = ctx.a3;
        dst_na[1] = ctx.a4;
    }
    if(cs == NV12)
    {
        dstb[0] = ctx.b1;
        dstb[1] = ctx.c1;
    }
    if(cs == NV21)
    {
        dstb[0] = ctx.c1;
        dstb[1] = ctx.b1;
    }
    if(cs == Y210)
    {
        //TODO Optimize
        memcpy(dsta, &ctx.a1, 2);
        memcpy(dsta + 2, &ctx.b1, 2);
        memcpy(dsta + 4, &ctx.a2, 2);
        memcpy(dsta + 6, &ctx.c1, 2);

        memcpy(dst_na, &ctx.a3, 2);
        memcpy(dst_na + 2, &ctx.b3, 2);
        memcpy(dst_na + 4, &ctx.a4, 2);
        memcpy(dst_na + 6, &ctx.c3, 2);
    }
    if(cs == P210 || cs == P010)
    {
        // TODO OPTIMIZE
        // register b4 b3 b2 b1
        // memory   b1 b2 b3 b4
        memcpy(dsta, &ctx.a1, 2);
        memcpy(dsta + 2, &ctx.a2, 2);
        memcpy(dst_na, &ctx.a3, 2);
        memcpy(dst_na + 2, &ctx.a4, 2);

        memcpy(dstb, &ctx.b1, 2);
        memcpy(dstb + 2, &ctx.c1, 2);
    }
    if(cs == P210)
    {
        memcpy(dst_nb, &ctx.b3, 2);
        memcpy(dst_nb + 2, &ctx.c3, 2);

    }
    if(cs == V210)
    {
        memcpy(dsta, &ctx.a1, 4);
        memcpy(dsta + 4, &ctx.b1, 4);
        memcpy(dsta + 8, &ctx.a2, 4);
        memcpy(dsta + 12, &ctx.b2, 4);


        memcpy(dst_na, &ctx.a3, 4);
        memcpy(dst_na + 4, &ctx.b3, 4);
        memcpy(dst_na + 8, &ctx.a4, 4);
        memcpy(dst_na + 12, &ctx.b4, 4);
    }
}
template <Colorspace cs> inline void pack_init(Context& ctx)
{
    if(IS_YUV422( cs ))
    {
        ctx.b1 = ctx.b2 =  (ctx.b1 + ctx.b2) / 2;
        ctx.c1 = ctx.c2 =  (ctx.c1 + ctx.c2) / 2;
        ctx.b3 = ctx.b4 =  (ctx.b3 + ctx.b4) / 2;
        ctx.c3 = ctx.c4 =  (ctx.c3 + ctx.c4) / 2;
    }
    if( IS_YUV420( cs ))
    {
        ctx.b1 = (ctx.b1 + ctx.b2 + ctx.b3 + ctx.b4) / 4;
        ctx.c1 = (ctx.c1 + ctx.c2 + ctx.c3 + ctx.c4) / 4;
    }

}
template <Colorspace cs> inline void pack(Context& ctx, Context& ctx2 = 0, Context& ctx3 = 0)
{
    if(cs == V210)
    {
        pack_init < cs >( ctx );
        pack_init < cs >( ctx2 );
        pack_init < cs >( ctx3 );
        pack_V210(ctx, ctx2, ctx3);
        return;
    }

    pack_init < cs >( ctx );
    if(cs == Y210 || cs == P210 || cs == P010)
    {
        ctx.a1 <<= 6;
        ctx.a2 <<= 6;
        ctx.a4 <<= 6;
        ctx.a3 <<= 6;

        ctx.b1 <<= 6;
        ctx.c1 <<= 6;

     }
    if(cs == Y210 || cs == P210)
    {
        ctx.b3 <<= 6;
        ctx.c3 <<= 6;
    }
    if(cs == RGB32)
    {
        ctx.a1 = pack10_in_int(ctx.a1, ctx.b1, ctx.c1) << 10 ;
        ctx.a2 = pack10_in_int(ctx.a2, ctx.b2, ctx.c2) << 10 ;
        ctx.a3 = pack10_in_int(ctx.a3, ctx.b3, ctx.c3) << 10 ;
        ctx.a4 = pack10_in_int(ctx.a4, ctx.b4, ctx.c4) << 10 ;
    }
    if(cs == BGR32)
    {
        ctx.a1 = pack10_in_int(ctx.c1, ctx.b1, ctx.a1) << 10 ;
        ctx.a2 = pack10_in_int(ctx.c2, ctx.b2, ctx.a2) << 10 ;
        ctx.a3 = pack10_in_int(ctx.c3, ctx.b3, ctx.a3) << 10 ;
        ctx.a4 = pack10_in_int(ctx.c4, ctx.b4, ctx.a4) << 10 ;
    }

    if(cs == A2R10G10B10)
    {
        ctx.a1 = pack10_in_int(ctx.a1, ctx.b1, ctx.c1) | ( 3 << 30 );
        ctx.a2 = pack10_in_int(ctx.a2, ctx.b2, ctx.c2) | ( 3 << 30 );
        ctx.a3 = pack10_in_int(ctx.a3, ctx.b3, ctx.c3) | ( 3 << 30 );
        ctx.a4 = pack10_in_int(ctx.a4, ctx.b4, ctx.c4) | ( 3 << 30 );
    }
    if(cs == A2B10G10R10)
    {
        ctx.a1 = pack10_in_int(ctx.c1, ctx.b1, ctx.a1) | ( 3 << 30 );
        ctx.a2 = pack10_in_int(ctx.c2, ctx.b2, ctx.a2) | ( 3 << 30 );
        ctx.a3 = pack10_in_int(ctx.c3, ctx.b3, ctx.a3) | ( 3 << 30 );
        ctx.a4 = pack10_in_int(ctx.c4, ctx.b4, ctx.a4) | ( 3 << 30 );
    }


}

template <class T > inline T clip(T val_a, T min, T max)
{
    return std::max(std::min(val_a, max), min);
}
template <class T> inline T round_shift(T val, const size_t n)
{
    return (val + (1 << (n - 1))) >> n;
    //return val >> n;
}



template <Colorspace cs> inline void offset_yuv (int32_t& y, int32_t& u, int32_t& v,bool is_left, ConvertMeta& meta)
{
    if(IS_YUV( cs ))
    {
        //offset left
        if(is_left)
        {
            y -= meta.offset_y_from;
            u -= meta.offset_u_from;
            v -= meta.offset_v_from;
        }
        else
        // offset right
        {
            y += meta.offset_y_to;
            u += meta.offset_u_to;
            v += meta.offset_v_to;
        }
    }
}

template <Colorspace cs> static inline void clip_point(int32_t& val_a, int32_t& val_b, int32_t& val_c)
{
    //puts("before");
    //std::cout << val_a << " "<< val_b << " " << val_c << std::endl;
    if(cs == RGB24)
    {
        val_a = clip(val_a, 0, 255);
        val_b = clip(val_b, 0, 255);
        val_c = clip(val_c, 0, 255);
    }
    if( cs == A2R10G10B10)
    {
        val_a = clip(val_a, 0, 255 << 2);
        val_b = clip(val_b, 0, 255 << 2);
        val_c = clip(val_c, 0, 255 << 2);
    }
    else if( IS_YUV( cs ) )
    {
        val_a = clip(val_a, 16, 235);
        val_b = clip(val_b, 16, 240);
        val_c = clip(val_c, 16, 240);
        //puts("after");
        //std::cout << val_a << " "<< val_b << " " << val_c << std::endl;
    }
}
/*
 * | a |   |                  |   | a'|
 * | b | * | transform_matrix | = | b'|
 * | c |   |                  |   | c'|
 */
static inline void mat_mul(int32_t &a, int32_t &b, int32_t &c, const int32_t* transform_matrix)
{

    int32_t tmp1 = transform_matrix[0*3 + 0] * a + transform_matrix[0*3 + 1] * b + transform_matrix[0*3 + 2] * c;
    int32_t tmp2 = transform_matrix[1*3 + 0] * a + transform_matrix[1*3 + 1] * b + transform_matrix[1*3 + 2] * c;
    int32_t tmp3 = transform_matrix[2*3 + 0] * a + transform_matrix[2*3 + 1] * b + transform_matrix[2*3 + 2] * c;

    a = tmp1;
	b = tmp2;
	c = tmp3;
}
inline void round_shift( int32_t& a, int32_t& b, int32_t& c, int n){
    a = (a + (1 << (n - 1))) >> n;
    b = (b + (1 << (n - 1))) >> n;
    c = (c + (1 << (n - 1))) >> n;
}



template <Colorspace cs_from, Colorspace cs_to> inline void convert_range(int32_t* matrix)
{
    if(IS_RGB(cs_from) && IS_YUV(cs_to))
        for(int r = 0 ; r < 3; ++r)
        {
            matrix[r * 3 + 0] *= 219;
            matrix[r * 3 + 1] *= 219;
            matrix[r * 3 + 2] *= 219;

            matrix[r * 3 + 0] >>= 8;
            matrix[r * 3 + 1] >>= 8;
            matrix[r * 3 + 2] >>= 8;
        }
    if(IS_YUV(cs_from) && IS_RGB(cs_to))
        for(int r = 0 ; r<3; ++r)
        {
            matrix[r * 3 + 0] <<= 8;
            matrix[r * 3 + 1] <<= 8;
            matrix[r * 3 + 2] <<= 8;

            matrix[r * 3 + 0] /= 219;
            matrix[r * 3 + 1] /= 219;
            matrix[r * 3 + 2] /= 219;
        }
}

template <Colorspace from_cs, Colorspace to_cs> inline void transform (Context &ctx, int32_t const * transform_matrix, ConvertMeta& meta)
{
#ifdef ENABLE_LOG
    std::cout << "matrix transform inside\n";
    for(int r = 0 ; r < 3; ++r)
    {
        std::cout << transform_matrix[ r * 3 + 0 ] << ' ' << transform_matrix[ r * 3 + 1 ] << " " <<transform_matrix[ r * 3 + 2 ] << "\n";
    }
#endif

    scale<from_cs, to_cs>(ctx.a1, ctx.b1, ctx.c1);
    scale<from_cs, to_cs>(ctx.a2, ctx.b2, ctx.c2);
    scale<from_cs, to_cs>(ctx.a3, ctx.b3, ctx.c3);
    scale<from_cs, to_cs>(ctx.a4, ctx.b4, ctx.c4);

    //  matrix multiple
    if( (IS_RGB(from_cs) && IS_YUV(to_cs)) || (IS_YUV(from_cs) && IS_RGB(to_cs)))
    {
        offset_yuv <from_cs> (ctx.a1, ctx.b1, ctx.c1, SHIFT_LEFT, meta);
        offset_yuv <from_cs> (ctx.a2, ctx.b2, ctx.c2, SHIFT_LEFT, meta);
        offset_yuv <from_cs> (ctx.a3, ctx.b3, ctx.c3, SHIFT_LEFT, meta);
        offset_yuv <from_cs> (ctx.a4, ctx.b4, ctx.c4, SHIFT_LEFT, meta);

        mat_mul(ctx.a1, ctx.b1, ctx.c1, transform_matrix);
        mat_mul(ctx.a2, ctx.b2, ctx.c2, transform_matrix);
        mat_mul(ctx.a3, ctx.b3, ctx.c3, transform_matrix);
        mat_mul(ctx.a4, ctx.b4, ctx.c4, transform_matrix);

        round_shift(ctx.a1, ctx.b1, ctx.c1, 8);
        round_shift(ctx.a2, ctx.b2, ctx.c2, 8);
        round_shift(ctx.a3, ctx.b3, ctx.c3, 8);
        round_shift(ctx.a4, ctx.b4, ctx.c4, 8);

        offset_yuv <to_cs> (ctx.a1, ctx.b1, ctx.c1, SHIFT_RIGHT, meta);
        offset_yuv <to_cs> (ctx.a2, ctx.b2, ctx.c2, SHIFT_RIGHT, meta);
        offset_yuv <to_cs> (ctx.a3, ctx.b3, ctx.c3, SHIFT_RIGHT, meta);
        offset_yuv <to_cs> (ctx.a4, ctx.b4, ctx.c4, SHIFT_RIGHT, meta);

        clip_point <to_cs> (ctx.a1, ctx.b1, ctx.c1);
        clip_point <to_cs> (ctx.a2, ctx.b2, ctx.c2);
        clip_point <to_cs> (ctx.a3, ctx.b3, ctx.c3);
        clip_point <to_cs> (ctx.a4, ctx.b4, ctx.c4);

    } // if(from_cs_type != to_type)
}
template <Colorspace cs> inline void next_row (uint8_t* &ptr_a, uint8_t* &ptr_b, uint8_t* &ptr_c, const size_t stride[3])
{
    // TODO This define was used only once
    if (IS_INTERLEAVED( cs ) ) {
		ptr_a += stride[0] * 2;
    }
    if( IS_PLANAR ( cs ) )
    {
        ptr_a += stride[0] * 2;
        ptr_b += stride[1] * 2;
        ptr_c += stride[2] * 2;
    }
    if( IS_SEMIPLANAR ( cs ) ) //YUV420
    {
        ptr_a += stride[0] * 2;
        ptr_b += stride[1];
    }

}
template<Colorspace from_cs, Colorspace to_cs, Standard st> void colorspace_convert(ConvertMeta& meta)
{
	int32_t transform_matrix[ 3 * 3 ];
    set_transform_coeffs<from_cs, to_cs, st>(transform_matrix);
    convert_range <from_cs, to_cs>(transform_matrix);

	uint8_t* src_a = meta.src_data[0];
	uint8_t* src_b = meta.src_data[1];
	uint8_t* src_c = meta.src_data[2];

	uint8_t* dst_a = meta.dst_data[0];
	uint8_t* dst_b = meta.dst_data[1];
	uint8_t* dst_c = meta.dst_data[2];
    Context context1, context2, context3;
    int8_t step;
    if(from_cs == V210 || to_cs == V210)
        step = 6;
    else
        step = 2;
    size_t x,y;
    size_t shift_a, shift_b, shift_c;
    if(from_cs == V210 && to_cs == V210)
    {
        memcpy(dst_a, src_a, meta.width * meta.height * 16 / 6);
    }
    else
    for(y = 0; y < meta.height; y += 2) {
        for(x = 0; x+step <= meta.width; x += step) {
            // Process 2 x 2(Step) pixels

            if(to_cs == V210 && from_cs != V210)
            {
                get_pos <from_cs>(shift_a, shift_b, shift_c, x);
                load < from_cs> (meta, context1,
                                        src_a + shift_a,
                                        src_b + shift_b,
                                        src_c + shift_c);

                get_pos <from_cs>(shift_a, shift_b, shift_c, x + 2);
                load < from_cs> (meta, context2,
                                            src_a + shift_a,
                                            src_b + shift_b,
                                            src_c + shift_c);
                get_pos <from_cs>(shift_a, shift_b, shift_c, x + 4);
                load < from_cs> (meta, context3,
                                            src_a + shift_a,
                                            src_b + shift_b,
                                            src_c + shift_c);
                unpack <from_cs> (context1, context1, context1);
                unpack <from_cs> (context2, context2, context2);
                unpack <from_cs> (context3, context3, context3);

                transform <from_cs, to_cs> (context1, transform_matrix, meta);
                transform <from_cs, to_cs> (context2, transform_matrix, meta);
                transform <from_cs, to_cs> (context3, transform_matrix, meta);

                pack< to_cs >(context1, context2, context3);

                get_pos <to_cs>(shift_a, shift_b, shift_c, x);
                store<to_cs > (meta, context1,
                                     dst_a + shift_a,
                                     dst_b + shift_b,
                                     dst_c + shift_c);
            }
            else if(from_cs == V210 && to_cs != V210)
            {
                get_pos <from_cs>(shift_a, shift_b, shift_c, x);
                load < from_cs> (meta, context1,
                                            src_a + shift_a,
                                            src_b + shift_b,
                                            src_c + shift_c);
                unpack <from_cs> (context1, context2, context3);

                transform <from_cs, to_cs> (context1, transform_matrix, meta);
                transform <from_cs, to_cs> (context2, transform_matrix, meta);
                transform <from_cs, to_cs> (context3, transform_matrix, meta);

                pack< to_cs >(context1, context1, context1);
                pack< to_cs >(context2, context2, context2);
                pack< to_cs >(context3, context3, context3);

                get_pos <to_cs>(shift_a, shift_b, shift_c, x);
                store< to_cs > (meta, context1,
                                     dst_a + shift_a,
                                     dst_b + shift_b,
                                     dst_c + shift_c);
                get_pos <to_cs>(shift_a, shift_b, shift_c, x + 2);
                store< to_cs > (meta, context2,
                                     dst_a + shift_a,
                                     dst_b + shift_b,
                                     dst_c + shift_c);
                get_pos <to_cs>(shift_a, shift_b, shift_c, x + 4);
                store< to_cs > (meta, context3,
                                     dst_a + shift_a,
                                     dst_b + shift_b,
                                     dst_c + shift_c);
            }
            else if(from_cs != V210 && to_cs != V210)
            {
                get_pos <from_cs>(shift_a, shift_b, shift_c, x);
                load < from_cs> (meta, context1,
                                        src_a + shift_a,
                                        src_b + shift_b,
                                        src_c + shift_c);

                unpack <from_cs> (context1, context2, context3);

                transform <from_cs, to_cs> (context1, transform_matrix, meta);

                pack<to_cs > (context1, context1, context1);

                get_pos <to_cs>(shift_a, shift_b, shift_c, x);
                store<to_cs > (meta, context1,
                                             dst_a + shift_a,
                                             dst_b + shift_b,
                                             dst_c + shift_c);
            }
        }
        // finish line
        if(meta.width % step != 0  && to_cs == V210 && from_cs != V210)
        {
            context2.a1 = context2.a2 = context2.a3 = context2.a4 = 0;
            context2.b1 = context2.b2 = context2.b3 = context2.b4 = 0;
            context2.c1 = context2.c2 = context2.c3 = context2.c4 = 0;

            get_pos <from_cs>(shift_a, shift_b, shift_c, x);
            load < from_cs> (meta, context1,
                                    src_a + shift_a,
                                    src_b + shift_b,
                                    src_c + shift_c);

            if(meta.width % 6 >= 4) //==4
            {
                get_pos <from_cs>(shift_a, shift_b, shift_c, x + 2);
                load < from_cs> (meta, context2,
                                        src_a + shift_a,
                                        src_b + shift_b,
                                        src_c + shift_c);
            }

            unpack <from_cs> (context1, context1, context1);
            unpack <from_cs> (context2, context2, context2);
            unpack <from_cs> (context3, context3, context3);

            transform <from_cs, to_cs> (context1, transform_matrix, meta);
            transform <from_cs, to_cs> (context2, transform_matrix, meta);
            transform <from_cs, to_cs> (context3, transform_matrix, meta);

            pack< to_cs >(context1, context2, context3);

            get_pos <to_cs>(shift_a, shift_b, shift_c, x);
            store<to_cs > (meta, context1,
                                 dst_a + shift_a,
                                 dst_b + shift_b,
                                 dst_c + shift_c);
        }

        next_row <from_cs> (src_a, src_b, src_c, meta.src_stride);
        next_row <to_cs> (dst_a, dst_b, dst_c, meta.dst_stride_horiz);
	}
}

} // namespace ColorspaceConverter;

#endif
