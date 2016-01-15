#ifndef COLOR_CONVERT
#define COLOR_CONVERT

#define IS_RGB(cs) (cs == RGB24 || cs == A2R10G10B10)
#define IS_YUV(cs) (cs == YUV444 || cs == YUYV || cs == YVYU || cs == UYVY || cs == NV21 || cs == NV12 || cs == YV12 || cs == I420 || cs == YV16 || cs == YUV422 || cs == P210 || cs == P010)
#define IS_8BIT(cs) (cs == RGB24 || cs == YUV444 || cs == YUYV || cs == YVYU || cs == UYVY || cs == NV21 || cs == NV12 || cs == YV12 || cs == I420 || cs == YV16 || cs == YUV422)
#define IS_10BIT(cs) (cs == A2R10G10B10 || cs == P210 || cs == P010)
#define IS_INTERLEAVED(cs) (cs == A2R10G10B10 ||  cs == RGB24 || cs == YUV444 || cs == YUYV || cs == YVYU || cs == UYVY)
#define IS_PLANAR(cs) (cs == YV12 || cs == I420 || cs == YV16)
#define IS_SEMIPLANAR( cs ) (cs == NV21 || cs == NV12)
#define IS_YUV422( cs ) (cs == YUYV || cs == YVYU || cs == UYVY || cs == YV16 || cs == YUV422 || cs == P210 || cs == YV16 )
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
    YUV444, // Composite Y->U->V    (4:4:4)
    YUYV,   // Composite Y->U->Y->V (4:2:2)
    YUY2 = YUYV,   // Composite Y->U->Y->V (4:2:2) (duplicate)
    YVYU,   // Composite Y->V->Y->U (4:2:2)
    UYVY,   // Composite U->Y->V->Y (4:2:2)
    Y210,   // Composite Y->U->Y->V (4:2:2) (10 bit p/s)

    NV21, // Planar Y, merged V->U  (4:2:0)
    NV12, // Planar Y, merged U->V  (4:2:0)

    YV12,   // Planar Y, V, U (4:2:0)
    I420,   // Planar Y, U, V (4:2:0)
    YV16,   // Planar Y, V, U (4:2:2)
    YUV422, // Planar Y, U, V (4:2:2)
    P210,   // Planar Y, U, V (4:2:2)  10 bit
    P010,   // Planar,Y, U, V (4:2:0)  10-bit
    V210,   // Interleaved, 12  10bit values packed in 128bit
    RGB24,
    A2R10G10B10 // 32bit
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
	uint8_t *src_data[3];
	uint8_t *dst_data[3];
	size_t width;
	size_t height;
    int32_t offset_y;
    int32_t offset_u;
    int32_t offset_v;
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

/*
static const size_t k_rgb_steps[3] = { 3, 3, 3 };

static const size_t k_yuv420_steps[3] = { 2, 1, 1 }; // ?

static const size_t k_yuv422_planar_steps[3] = { 1, 1, 1 };
static const size_t k_yuv422_interleaved_steps[3] = { 2, 1, 1 };

static const size_t k_yuv444_planar_steps[3] = { 1, 1, 1 };
static const size_t k_yuv444_interleaved_steps[3] = { 3, 3, 3 };
*/


template<Colorspace cs> inline void get_pos(size_t& posa, size_t& posb, size_t& posc, const int cur_pos)
{
    posa = 0;
    posb = 0;
    posc = 0;
    if (cs == RGB24 )
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
	if (cs == A2R10G10B10) {
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
    // TODO add new formats
        /*
        if(pack == SemiPlanar){
            posa = cur_pos;
            posb = cur_pos * 2;
        }
        if(pack == Interleaved){
            posa =cur_pos * 3;
        }
	}
    */
    /*
        if(pack == SemiPlanar){
            posa = cur_pos;
            posb = cur_pos;
        }
        if(pack == Interleaved)
            posa = cur_pos * 2;
	}
	if (cs == YUV420) {
        if(pack == Planar)
        {
            // Y Y
            // Y Y
            //U
            //V
            posa = cur_pos;
            posb = cur_pos / 2;
            posc = cur_pos / 2;
        }
        if(pack == SemiPlanar){
            posa =cur_pos;
            posb = cur_pos;
        }
	}
    */
    // TODO: Usupported transform
    //assert(0);
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
/*
static inline void unpack_V210(Context& ctx, uint32_t vala, uint32_t valb, uint32_t valc, uint32_t vald)
{
    ctx.a1 =;

}
*/
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
static inline void pack_V210(Context& ctx1, Context& ctx2, Context& ctx3)
{
    uint32_t data1 = 0, data2 = 0, data3 = 0, data4  = 0;

    data1 = ((ctx1.c1 & 0X3FF) << 20) | ((ctx1.a1 & 0X3FF) << 10) | (ctx1.b1 & 0X3FF) ;
    data2 = ((ctx2.a1 & 0X3FF) << 20) | ((ctx2.b1 & 0X3FF) << 10) | (ctx1.a2 & 0X3FF) ;
    data3 = ((ctx3.b1 & 0X3FF) << 20) | ((ctx2.a2 & 0X3FF) << 10) | (ctx2.c1 & 0X3FF) ;
    data4 = ((ctx3.a2 & 0X3FF) << 20) | ((ctx3.c1 & 0X3FF) << 10) | (ctx3.a1 & 0X3FF) ;

    ctx1.a1 = data1;
    ctx1.b1 = data2;
    ctx1.a2 = data3;
    ctx1.b2 = data4;

    data1 = 0;
    data2 = 0;
    data3 = 0;
    data4 = 0;

    data1 = ((ctx1.c3 & 0X3FF) << 20) | ((ctx1.a3 & 0X3FF) << 10) | (ctx1.b3 & 0X3FF) ;
    data2 = ((ctx2.a3 & 0X3FF) << 20) | ((ctx2.b3 & 0X3FF) << 10) | (ctx1.a4 & 0X3FF) ;
    data3 = ((ctx3.b3 & 0X3FF) << 20) | ((ctx2.a4 & 0X3FF) << 10) | (ctx2.c3 & 0X3FF) ;
    data4 = ((ctx3.a4 & 0X3FF) << 20) | ((ctx3.c3 & 0X3FF) << 10) | (ctx3.a3 & 0X3FF) ;

    ctx1.a3 = data1;
    ctx1.b3 = data2;
    ctx1.a4 = data3;
    ctx1.b4 = data4;

}

static inline void unpack_A2R10G10B10(int32_t& vala, int32_t& valb, int32_t& valc, const uint32_t buf)
{
            //in memory
            // rrrrrraa ggggrrrr bbgggggg bbbbbbbb
            //TO DO define R_FROM_A2R10G10 e.t.c
            // in register
            // bbbbbbbb bbgggggg ggggrrrr rrrrrraa
          /*
            // red
            vala = (( (buf >> 16) & (0xF)) << 6 ) |  ((buf >> 26) & (0x3F));

            //green
            valb = (( (buf >> 8) & (0x3F) )<< 4) | ((buf >> 20) & (0xF));

            //blue
            valc =  ((buf & 0xFF) << 2) | (( buf >> 14) & (0x3));
          */
            vala = (buf >> 2) & 0x3FF;
            valb = (buf >> 12) & 0x3FF;
            valc = (buf >> 22) & 0x3FF;


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
    if(cs == A2R10G10B10)
    {
        // Optimize ???
        memcpy( &ctx.a1, srca,4);
        memcpy( &ctx.a2, srca + 4,4);
        memcpy( &ctx.a3, src_na,4);
        memcpy( &ctx.a4, src_na + 4,4);
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
        ctx.a1 = (srca[0] << 8) | srca[1];
        ctx.b1 = (srca[2] << 8) | srca[3];
        ctx.a2 = (srca[4] << 8) | srca[5];
        ctx.c1 = (srca[6] << 8) | srca[7];

        ctx.a3 = (src_na[0] << 8) | src_na[1];
        ctx.b3 = (src_na[2] << 8) | src_na[3];
        ctx.a4 = (src_na[4] << 8) | src_na[5];
        ctx.c3 = (src_na[6] << 8) | src_na[7];

    }
    if(cs == P210 || cs == P010)
    {
        // TODO OPTIMIZE
        ctx.a1 = (srca[0] << 8) | srca[1];
        ctx.a2 = (srca[2] << 8) | srca[3];

        ctx.a3 = (src_na[0] << 8) | src_na[1];
        ctx.a4 = (src_na[2] << 8) | src_na[3];

        ctx.b1 = (srcb[0] << 8) | srcb[1];
        ctx.c1 = (srcb[2] << 8) | srcc[3];


    }
    if(cs == P210)
    {
        ctx.b3 = (src_nb[0] << 8) | src_nb[1];
        ctx.c3 = (src_nb[2] << 8) | src_nc[3];

    }
    if(cs == V210)
    {
        memcpy( &ctx.a1, srca, 4);
        memcpy( &ctx.b1, srca + 4, 4);
        memcpy( &ctx.a2, srca + 8, 4);
        memcpy( &ctx.b2, srca + 12, 4);

        memcpy( &ctx.a3, src_na, 4);
        memcpy( &ctx.b3, src_na + 4, 4);
        memcpy( &ctx.a4, src_na + 8, 4);
        memcpy( &ctx.b4, src_na + 12, 4);

    }
}
    /*
    if (pack == Interleaved)
    {
        if(cs == RGB || cs == YUV444){
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
        if(cs == YUV422){
            ctx.a1 = srca[0];
            ctx.b1 = srca[1];
            ctx.c1 = srca[3];

            ctx.a2 = srca[2];

            ctx.a3 = src_na[0];
            ctx.b3 = src_na[1];
            ctx.c3 = src_na[3];

            ctx.a4 = src_na[2];
        }
        if(cs == A2R10G10B10)
        {
            // Optimize ???

            ctx.c1 = (*(srca) << 24) | (*(srca+1) << 16) | (*(srca+2) << 8) | (*(srca + 3)) ;
            ctx.c2 = (*(srca+4) << 24) | (*(srca+5) << 16) | (*(srca+6) << 8) | (*(srca + 7)) ;
            ctx.c3 = (*(src_na) << 24) | (*(src_na+1) << 16) | (*(src_na+2) << 8) | (*(src_na + 3)) ;
            ctx.c4 = (*(src_na+4) << 24) | (*(src_na + 5) << 16) | (*(src_na + 6) << 8) | (*(src_na + 7)) ;

        }
	}
	else  if( pack == Planar)
	{
	  if(cs == RGB || cs == YUV444){
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
        if(cs == YUV422)
        {
            ctx.a1 = srca[0];
            ctx.a2 = srca[1];
            ctx.a3 = src_na[0];
            ctx.a4 = src_na[1];

            ctx.b1 = srcb[0];
            ctx.b3 = src_nb[0];

            ctx.c1 = srcc[0];
            ctx.c3 = src_nc[0];



        }
        if(cs == YUV420)
        {
            ctx.a1 = srca[0];
            ctx.a2 = srca[1];
            ctx.a3 = src_na[0];
            ctx.a4 = src_na[1];

            ctx.b1 = srcb[0];
            ctx.c1 = srcc[0];



        }
	}
	else
	{
	  if(cs == RGB || cs == YUV444){
            ctx.a1 = srca[0];
            ctx.b1 = srcb[0];
            ctx.c1 = srcb[1];

            ctx.a2 = srca[1];
            ctx.b2 = srcb[2];
            ctx.c2 = srcb[3];

            ctx.a3 = src_na[0];
            ctx.b3 = src_nb[0];
            ctx.c3 = src_nb[1];

            ctx.a4 = src_na[1];
            ctx.b4 = src_nb[2];
            ctx.c4 = src_nb[3];
        }
        if(cs == YUV422)
        {
            ctx.a1 = srca[0];
            ctx.b1 = srcb[0];
            ctx.c1 = srcb[1];

            ctx.a2 = srca[1];

            ctx.a3 = src_na[0];
            ctx.b3 = src_nb[0];
            ctx.c3 = src_nb[1];

            ctx.a4 = src_na[1];
        }
        if(cs == YUV420)
        {
            ctx.a1 = srca[0];
            ctx.b1 = srcb[0];
            ctx.c1 = srcb[1];

            ctx.a2 = srca[1];

            ctx.a3 = src_na[0];

            ctx.a4 = src_na[1];
        }
	}
    */


static inline uint32_t pack_A2R10G10B10(int32_t vala, int32_t valb, int32_t valc)
{
    // rrrrrraa ggggrrrr bbgggggg bbbbbbbb
    // bbbbbbbb bbgggggg ggggrrrr rrrrrrAA
    int32_t res;
    /*
    res = 3l << 24;
    res |= (vala & 0x3F) << 26;
    res |= ( (vala >> 6) & 0xF) << 16;

    res |= (valb & 0xF) << 20;
    res |= ( (valb >> 4) & 0X3F) << 8;

    res |= (valc & 0x3) << 14;
    res |= (valc >> 2) & 0XFF;
    */
    res = ((valc & 0X3FF) << 22) | ((valb & 0X3FF) << 12) | ((vala & 0X3FF) << 2) | 3;
    return res;
}
template < Colorspace cs> inline void unpack (Context& ctx, Context& ctx2, Context& ctx3)
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
    if(cs == P210 || cs == Y210)
    {
        ctx.b3 >>= 6;
        ctx.c3 >>= 6;

    }
    if(cs == V210)
    {
        unpack_V210(ctx, ctx2, ctx3);
    }
    if(cs == A2R10G10B10)
    {
        unpack_A2R10G10B10(ctx.a1, ctx.b1, ctx.c1, ctx.a1);
        unpack_A2R10G10B10(ctx.a2, ctx.b2, ctx.c2, ctx.a2);
        unpack_A2R10G10B10(ctx.a3, ctx.b3, ctx.c3, ctx.a3);
        unpack_A2R10G10B10(ctx.a4, ctx.b4, ctx.c4, ctx.a4);
    }

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

    /*
    if (pack == Interleaved){
        if(cs == YUV422){
            ctx.c2 = ctx.c1;
            ctx.b2 = ctx.b1;

            ctx.c4 = ctx.c3;
            ctx.b4 = ctx.b3;
        }
        if(cs == A2R10G10B10)
        {
            unpack_A2R10G10B10(ctx.a1, ctx.b1, ctx.c1, ctx.c1);
            unpack_A2R10G10B10(ctx.a2, ctx.b2, ctx.c2, ctx.c2);
            unpack_A2R10G10B10(ctx.a3, ctx.b3, ctx.c3, ctx.c3);
            unpack_A2R10G10B10(ctx.a4, ctx.b4, ctx.c4, ctx.c4);
        }
	}
    else if( pack == Planar)
	{
        if(cs == YUV422)
        {
            ctx.b2 = ctx.b1;
            ctx.c2 = ctx.c1;

            ctx.b4 = ctx.b3;
            ctx.c4 = ctx.c3;

        }
        if(cs == YUV420)
        {
            ctx.b2 = ctx.b1;
            ctx.c2 = ctx.c1;

            ctx.b3 = ctx.b1;
            ctx.c3 = ctx.c1;

            ctx.b4 = ctx.b1;
            ctx.c4 = ctx.c1;

        }
	}
	else
	{
        if(cs == YUV422)
        {
            ctx.b2 = ctx.b1;
            ctx.c2 = ctx.c1;
            ctx.b4 = ctx.b3;
            ctx.c4 = ctx.c3;
        }
        if(cs == YUV420)
        {
            ctx.b2 = ctx.b1;
            ctx.c2 = ctx.c1;
            ctx.b3 = ctx.b1;
            ctx.c3 = ctx.c1;
            ctx.b4 = ctx.b1;
            ctx.c4 = ctx.c1;;
        }
	}
    */
//}
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

    if(cs == A2R10G10B10)
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
        dsta[ 0 ] = (ctx.a1 >> 8) & 0xFF;
        dsta[ 1 ] = (ctx.a1) & 0xFF;
        dsta[ 2 ] = (ctx.b1 >> 8) & 0xFF;
        dsta[ 3 ] = (ctx.b1) & 0xFF;
        dsta[ 4 ] = (ctx.a2 >> 8) & 0xFF;
        dsta[ 5 ] = (ctx.a2) & 0xFF;
        dsta[ 6 ] = (ctx.c1 >> 8) & 0xFF;
        dsta[ 7 ] = (ctx.c1) & 0xFF;


        dst_na[ 0 ] = (ctx.a3 >> 8) & 0xFF;
        dst_na[ 1 ] = (ctx.a3) & 0xFF;
        dst_na[ 2 ] = (ctx.b3 >> 8) & 0xFF;
        dst_na[ 3 ] = (ctx.b3) & 0xFF;
        dst_na[ 4 ] = (ctx.a4 >> 8) & 0xFF;
        dst_na[ 5 ] = (ctx.a4) & 0xFF;
        dst_na[ 6 ] = (ctx.c3 >> 8) & 0xFF;
        dst_na[ 7 ] = (ctx.c3) & 0xFF;

    }
    if(cs == P210 || cs == P010)
    {
        dsta[ 0 ] = (ctx.a1 >> 8) & 0xFF;
        dsta[ 1 ] = (ctx.a1) & 0xFF;
        dsta[ 2 ] = (ctx.a2 >> 8) & 0xFF;
        dsta[ 3 ] = (ctx.a2) & 0xFF;
        dst_na[ 0 ] = (ctx.a3 >> 8) & 0xFF;
        dst_na[ 1 ] = (ctx.a3) & 0xFF;
        dst_na[ 2 ] = (ctx.a4 >> 8) & 0xFF;
        dst_na[ 3 ] = (ctx.a4) & 0xFF;


        dstb[ 0 ] = (ctx.b1 >> 8) & 0xFF;
        dstb[ 1 ] = (ctx.b1) & 0xFF;
        dstb[ 2 ] = (ctx.c1 >> 8) & 0xFF;
        dstb[ 3 ] = (ctx.c1) & 0xFF;

    }
    if(cs == P210)
    {
        dst_nb[ 0 ] = (ctx.b3 >> 8) & 0xFF;
        dst_nb[ 1 ] = (ctx.b3) & 0xFF;

        dst_nb[ 2 ] = (ctx.c3 >> 8) & 0xFF;
        dst_nb[ 3 ] = (ctx.c3) & 0xFF;
    }
}
        /*
    if (pack == Interleaved)
    {
        if(cs == RGB || cs == YUV444){
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
        if(cs == A2R10G10B10)
        {
            memcpy(dsta, &ctx.a1, 4);
            memcpy(dsta + 4, &ctx.a2, 4);
            memcpy(dst_na, &ctx.a3, 4);
            memcpy(dst_na + 4, &ctx.a4, 4);
        }
        if(cs == YUV422){
            dsta[0] = ctx.a1;
            dsta[1] = ctx.b1;

            dsta[2] = ctx.a2;
            dsta[3] = ctx.c1;

            dst_na[0] = ctx.a3;
            dst_na[1] = ctx.b3;

            dst_na[2] = ctx.a4;
            dst_na[3] = ctx.c3;
        }
	} else if(pack == Planar) { // planar
		if(cs == RGB || cs == YUV444){
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
        }
        if(cs == YUV422){
            dsta[0] = ctx.a1;
            dsta[1] = ctx.a2;

            dst_na[0] = ctx.a3;
            dst_na[1] = ctx.a4;

            dstb[0] = ctx.b1;
            dst_nb[0] = ctx.b3;

            dstc[0] = ctx.c1;
            dst_nc[0] = ctx.c3;
        }
        if(cs == YUV420){
            dsta[0] = ctx.a1;
            dsta[1] = ctx.a2;
            dst_na[0] = ctx.a3;
            dst_na[1] = ctx.a4;

            dstb[0] = ctx.b1;

            dstc[0] = ctx.c1;
        }
	}
	else { // semiplanar
		if(cs == RGB || cs == YUV444){
            dsta[ 0 ] = ctx.a1;
            dsta[ 1 ] = ctx.a2;

            dst_na[ 0 ] = ctx.a3;
            dst_na[ 1 ] = ctx.a4;

            dstb[ 0 ] = ctx.b1;
            dstb[ 1 ] = ctx.c1;

            dstb[ 2 ] = ctx.b2;
            dstb[ 3 ] = ctx.c2;

            dst_nb[ 0 ] = ctx.b3;
            dst_nb[ 1 ] = ctx.c3;

            dst_nb[ 2 ] = ctx.b4;
            dst_nb[ 3 ] = ctx.c4;
        }
        if(cs == YUV422){
        // p1 = 1,2
        // p2 = 3
        // p3 = 5
            dsta[0] = ctx.a1;
            dsta[1] = ctx.a2;

            dst_na[0] = ctx.a3;
            dst_na[1] = ctx.a4;

            dstb[0] = ctx.b1;
            dstb[1] = ctx.c1;

            dst_nb[0] = ctx.b3;
            dst_nb[1] = ctx.c3;

        }
        if(cs == YUV420){
        // p1 = 1,2
        // p2 = 3
        // p3 = 5
            dsta[0] = ctx.a1;
            dsta[1] = ctx.a2;
            dst_na[0] = ctx.a3;
            dst_na[1] = ctx.a4;

            dstb[0] = ctx.b1;
            dstb[1] = ctx.c1;
        }
	}
    */

template <Colorspace cs> inline void pack(Context& ctx, Context& ctx2 = 0, Context& ctx3 = 0)
{
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

    if(cs == A2R10G10B10)
    {
        ctx.a1 = pack_A2R10G10B10(ctx.a1, ctx.b1, ctx.c1);
        ctx.a2 = pack_A2R10G10B10(ctx.a2, ctx.b2, ctx.c2);
        ctx.a3 = pack_A2R10G10B10(ctx.a3, ctx.b3, ctx.c3);
        ctx.a4 = pack_A2R10G10B10(ctx.a4, ctx.b4, ctx.c4);
    }
    if(cs == V210)
    {
        pack_V210(ctx, ctx2, ctx3);
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
template <Colorspace cs_from, Colorspace cs_to > inline void init_offset_yuv( ConvertMeta& meta )
{

    // y+= 16 - result of range manipulation
    meta.offset_y = 16;
    meta.offset_u = 128;
    meta.offset_v = 128;
    // use only for convert (RGB -> YUV)  or (YUV -> RGB)
    if(IS_YUV(cs_from))
        scale < YUV444, cs_from > (meta.offset_y, meta.offset_u, meta.offset_v);
    else
        scale < YUV444, cs_to > (meta.offset_y, meta.offset_u, meta.offset_v);

}

template <Colorspace cs> inline void offset_yuv (int32_t& y, int32_t& u, int32_t& v,bool is_left, ConvertMeta& meta)
{
    if(IS_YUV( cs ))
    {
        //offset left
        if(is_left)
        {
            y -= meta.offset_y;
            u -= meta.offset_u;
            v -= meta.offset_v;
        }
        else
        // offset right
        {
            y += meta.offset_y;
            u += meta.offset_u;
            v += meta.offset_v;
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

    /*
    else
    {
        if(cs == RGB || cs == A2R10G10B10 || cs == YUV444)
        {
            ptr_a += stride[0] * 2;
            ptr_b += stride[1] * 2;
            ptr_c += stride[2] * 2;
        }
        if( cs == YUV422)
        {
            ptr_a += stride[0] * 2;
            ptr_b += stride[1] * 2;
            ptr_c += stride[2] * 2;

        }
        if(cs == YUV420)
        {
            // w
            // w/2 (for 2 lines)
            // w/2 (for 2 lines)
            ptr_a += stride[0] * 2;
            ptr_b += stride[1];
            ptr_c += stride[2];
        }
	}
   */
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
