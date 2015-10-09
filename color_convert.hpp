#ifndef COLOR_CONVERT
#define COLOR_CONVERT

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
	YUV444,
    YUV422,
    YUV420,
	RGB,
	A2R10G10B10
};

enum Standard
{
	BT_601,
	BT_709,
	BT_2020
};

enum Pack
{
	Planar,
	SemiPlanar,
	Interleaved
};

/*
 * 1 2
 * 3 4
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
	size_t src_stride[3];
	size_t dst_stride[3];
    uint8_t  buf1[16];
    uint8_t  buf2[16];
    uint8_t  buf3[16];
    uint8_t  buf4[16];
    uint8_t  buf5[16];
    uint8_t  buf6[16];
};

static const int16_t k_bt601_RGB_to_YUV[3 * 3] =
{
	77,  150,  29,
	-44,  -87, 131,
	131, -110, -21,
};

static const int16_t k_bt709_RGB_to_YUV[3 * 3] =
{
	54, 183, 18,
	-30, -101, 131,
	131, -119, -12
};

static const int16_t k_bt2020_RGB_to_YUV[3 * 3] =
{
    67, 174, 15,
    -37, -94, 131,
    131, -120, -10
};
static const int16_t k_bt601_YUV_to_RGB[3 * 3] =
{
	256,   0,  351,
	256, -86, -179,
	256, 444,    0
};

static const int16_t k_bt709_YUV_to_RGB[3 * 3] =
{
	256, 0 , 394,
	256, -47, -117,
	256, 465,    0
};
static const int16_t k_bt2020_YUV_to_RGB[3 * 3] =
{
    256, 0, 369,
    256, -41, -143,
    257, 471, 0
};


static const size_t k_rgb_steps[3] = { 3, 3, 3 };

static const size_t k_yuv420_steps[3] = { 2, 1, 1 }; // ?

static const size_t k_yuv422_planar_steps[3] = { 1, 1, 1 };
static const size_t k_yuv422_interleaved_steps[3] = { 2, 1, 1 };

static const size_t k_yuv444_planar_steps[3] = { 1, 1, 1 };
static const size_t k_yuv444_interleaved_steps[3] = { 3, 3, 3 };



template<Pack pack, Colorspace cs> inline void get_pos(size_t& posa, size_t& posb, size_t& posc, const int cur_pos)
{
	if (cs == RGB) {
        posa = cur_pos * 3;
        return;
	}
	if (cs == A2R10G10B10) {
        posa = cur_pos * 4;
	}

	if (cs == YUV444) {
        if(pack == Planar){
            posa = cur_pos;
            posb = cur_pos;
            posc = cur_pos;
        }
        if(pack == Interleaved){
            posa =cur_pos * 3;
        }
	}
    if (cs == YUV422) {
        if(pack == Planar){
            posa = cur_pos;
            posb = cur_pos / 2;
            posc = cur_pos / 2;
        }
        if(pack == Interleaved)
            posa = cur_pos * 2;
	}
	if (cs == YUV420) {
            // Y Y
            // Y Y
            //U
            //V
            posa = cur_pos;
            posb = cur_pos / 2;
            posc = cur_pos / 2;

	}

    // TODO: Usupported transform
    //assert(0);
}

template <Colorspace from, Colorspace to, Standard st> const int16_t* get_transfrom_coeffs()
{
    //std::cout <<"st is " << st << std::endl;
	if (from == to) {
		return nullptr;
	}
	if (st == BT_601) {
        if ((from == RGB || from == A2R10G10B10) && (to == YUV444 || to == YUV422 || to == YUV420) ) {
			return k_bt601_RGB_to_YUV;
		}
        else
		if ((from == YUV444 || from == YUV422 || from == YUV420) && (to == RGB || to == A2R10G10B10) ) {
			return k_bt601_YUV_to_RGB;
		}
		// TODO: Usupported transform
		//else
        //assert(0);
	}
    else
    if (st == BT_709) {
		if ((from == RGB || from == A2R10G10B10) && (to == YUV444 || to == YUV422 ) ) {
			return k_bt709_RGB_to_YUV;
		}
        else
		if ((from == YUV444 || from == YUV422) && (to == RGB || to == A2R10G10B10)) {
			return k_bt709_YUV_to_RGB;
		}
		// TODO: Usupported transform
		//else
        //assert(0);
	}
    else
    if (st == BT_2020) {
        if ((from == RGB || from == A2R10G10B10) && (to == YUV444 || to == YUV422 ) ) {
			return k_bt2020_RGB_to_YUV;
		}
		if((from == YUV444 || from == YUV422) && (to == RGB || to == A2R10G10B10)) {
			return k_bt2020_YUV_to_RGB;
		}
		// TODO: Usupported transform
		//assert(0);
	}
	// TODO: Usupported standard
	//assert(0);
	return nullptr;
}
template <Pack pack, Colorspace cs> inline void load(ConvertMeta& meta, const uint8_t *src_a, const uint8_t *src_b, const uint8_t *src_c, const size_t stride[3]){
    if(pack == Interleaved ){
        // 1,[2,3]
        // 4,[5,6]
        // TODO refactoring!!!
        //memcpy(meta.buf1, src_a , 2);
        //memcpy(meta.buf4, src_a + stride[0] + 0, 2);
        if(cs == YUV444 || cs == RGB){
                memcpy(meta.buf1, src_a , 6);
                memcpy(meta.buf4, src_a + stride[0] ,6);
        }
        if(cs == YUV422){
            memcpy(meta.buf1, src_a, 4);
            memcpy(meta.buf4, src_a + stride[0], 4);
        }
        if( cs == A2R10G10B10)
        {
            memcpy( meta.buf1, src_a, 8);
            memcpy( meta.buf4, src_a + stride[ 0 ], 8);
        }
    }
    else
        if(pack == Planar){
            if(cs == YUV444 || cs == RGB){
                memcpy(meta.buf1, src_a, 2);
                memcpy(meta.buf2, src_a + stride[0], 2);
                memcpy(meta.buf3, src_b, 2);
                memcpy(meta.buf4, src_b + stride[1], 2);
                memcpy(meta.buf5, src_c, 2);
                memcpy(meta.buf6, src_c + stride[2], 2);
            }
            if(cs == YUV422){
                memcpy(meta.buf1, src_a, 2);
                memcpy(meta.buf2, src_a + stride[ 0 ], 2);
                memcpy(meta.buf3, src_b, 1);
                memcpy(meta.buf4, src_b + stride[ 1 ], 1);
                memcpy(meta.buf5, src_c, 1);
                memcpy(meta.buf6, src_c + stride[ 2 ], 1);
            }
            if(cs == YUV420){
                memcpy(meta.buf1, src_a, 2);
                memcpy(meta.buf2, src_a + stride[0], 2);
                memcpy(meta.buf3, src_b, 1);
                memcpy(meta.buf5, src_c, 1);
            }
        }

}
static inline void unpack_A2R10G10B10(int32_t& vala, int32_t& valb, int32_t& valc, const uint8_t* buf)
{
            // rrrrrraa ggggrrrr bbgggggg bbbbbbbb

            int32_t val;
            val = buf[1] & (0xF);
            val <<= 6;
            val |= (buf[0] >> 2) & (0x3F);
            vala = val;

            val = ( buf[2] ) & (0x3F);
            val <<= 4;
            val |= (buf[1] >> 4) & (0xF);
            valb = val;

            val =  buf[3] ;
            val <<= 2;
            val |= ( buf[2] >> 6) & (0xF);
            valc = val;

}
static inline void pack_A2R10G10B10(int32_t vala, int32_t valb, int32_t valc, uint8_t* buf)
{
            // rrrrrraa ggggrrrr bbgggggg bbbbbbbb

            int8_t res[ 4 ];
            res[0] = 3;
            res[0] |= (vala & 0x3F) << 2;

            res[1] = (vala >> 6) & 0XF;
            res[1] |= (valb & 0xF) << 4;

            res[2] = (valb >> 4) & 0X3F;
            res[2] |= (valc & 3) << 6;

            res[3] = (valc >> 2) & 0XFF;

            memcpy(buf, res, 4);

}
template <Pack pack, Colorspace cs> inline void unpack (const ConvertMeta& meta, Context &ctx)
{
    if (pack == Interleaved){
        // 1,[2,3]
        // 4,[5,6]
        if(cs == RGB || cs == YUV444){
            ctx.a1 = meta.buf1[0];
            ctx.b1 = meta.buf1[1];
            ctx.c1 = meta.buf1[2];

            ctx.a2 = meta.buf1[3];
            ctx.b2 = meta.buf1[4];
            ctx.c2 = meta.buf1[5];

            ctx.a3 = meta.buf4[0];
            ctx.b3 = meta.buf4[1];
            ctx.c3 = meta.buf4[2];

            ctx.a4 = meta.buf4[3];
            ctx.b4 = meta.buf4[4];
            ctx.c4 = meta.buf4[5];
        }
        if(cs == YUV422){
            ctx.a1 = meta.buf1[0];
            ctx.b1 = meta.buf1[1];

            ctx.a2 = meta.buf1[2];
            ctx.c2 = meta.buf1[3];

            ctx.a3 = meta.buf4[0];
            ctx.b3 = meta.buf4[1];

            ctx.a4 = meta.buf4[2];
            ctx.c4 = meta.buf4[3];

            ctx.c1 = ctx.c2;
            ctx.b2 = ctx.b1;

            ctx.c3 = ctx.c4;
            ctx.b4 = ctx.b3;
        }
        if(cs == A2R10G10B10)
        {
            unpack_A2R10G10B10(ctx.a1, ctx.b1, ctx.c1, meta.buf1);
            unpack_A2R10G10B10(ctx.a2, ctx.b2, ctx.c2, meta.buf1 + 4);
            unpack_A2R10G10B10(ctx.a3, ctx.b3, ctx.c3, meta.buf4);
            unpack_A2R10G10B10(ctx.a4, ctx.b4, ctx.c4, meta.buf4 + 4);
        }
	} else  { // planar
		//p1 = 1_2
        //p2 = 3_4
        //p3 = 5_6
        if(cs == RGB || cs == YUV444){
            ctx.a1 = meta.buf1[0];
            ctx.b1 = meta.buf3[0];
            ctx.c1 = meta.buf5[0];

            ctx.a2 = meta.buf1[1];
            ctx.b2 = meta.buf3[1];
            ctx.c2 = meta.buf5[1];

            ctx.a3 = meta.buf2[0];
            ctx.b3 = meta.buf4[0];
            ctx.c3 = meta.buf6[0];

            ctx.a4 = meta.buf2[1];
            ctx.b4 = meta.buf4[1];
            ctx.c4 = meta.buf6[1];
        }
        if(cs == YUV422){
        // p1 = 1,2
        // p2 = 3,4
        // p3 = 5,6
            ctx.a1 = meta.buf1[0];
            ctx.b1 = meta.buf3[0];
            ctx.c1 = meta.buf5[0];

            ctx.a2 = meta.buf1[1];
            ctx.b2 = ctx.b1;
            ctx.c2 = ctx.c1;

            ctx.a3 = meta.buf2[0];
            ctx.b3 = meta.buf4[0];
            ctx.c3 = meta.buf6[0];

            ctx.a4 = meta.buf2[1];
            ctx.b4 = ctx.b3;
            ctx.c4 = ctx.c3;
        }
        if(cs == YUV420){
        // p1 = 1,2
        // p2 = 3
        // p3 = 5
            ctx.a1 = meta.buf1[0];
            ctx.b1 = meta.buf3[0];
            ctx.c1 = meta.buf5[0];

            ctx.a2 = meta.buf1[1];
            ctx.b2 = ctx.b1;
            ctx.c2 = ctx.c1;

            ctx.a3 = meta.buf2[0];
            ctx.b3 = ctx.b1;
            ctx.c3 = ctx.c1;;

            ctx.a4 = meta.buf2[1];
            ctx.b4 = ctx.b1;
            ctx.c4 = ctx.c1;;
        }
	}
}
template <Pack pack, Colorspace cs> inline void store(const ConvertMeta& meta, uint8_t* dst_a, uint8_t* dst_b, uint8_t* dst_c , const size_t stride[3])
{
    if(pack == Interleaved){
        // 1,[2,3]
        // 4,[5,6]
        // TODO refactoring!!!
        //memcpy(dst_a , meta.buf1 , 2);
        //memcpy(dst_a + stride[0] + 0, meta.buf4, 2);
        if(cs == YUV444 || cs == RGB){
            memcpy(dst_a, meta.buf1, 6);
            memcpy(dst_a + stride[0], meta.buf4, 6);
            return;
        }
        if(cs == YUV422){
            memcpy(dst_a, meta.buf1, 4);
            memcpy(dst_a + stride[0], meta.buf4, 4);
        }
        if(cs == A2R10G10B10)
        {
            memcpy(dst_a, meta.buf1, 8);
            memcpy(dst_a + stride[0], meta.buf4, 8);
        }
    }
    else
        if(pack == Planar){
        //1_2
        //3_4
        //5_6
            if(cs == YUV444 || cs == RGB){
                memcpy(dst_a, meta.buf1, 2);
                memcpy(dst_a + stride[0],  meta.buf2, 2);
                memcpy(dst_b, meta.buf3, 2);
                memcpy(dst_b + stride[1], meta.buf4, 2);
                memcpy(dst_c, meta.buf5, 2);
                memcpy(dst_c + stride[2], meta.buf6, 2);
            }
            if(cs == YUV422){
                memcpy(dst_a, meta.buf1, 2);
                memcpy(dst_a + stride[0],meta.buf2, 2);
                memcpy(dst_b, meta.buf3, 2);
                memcpy(dst_c, meta.buf5, 2);
            }
            if(cs == YUV420){
                memcpy(dst_a, meta.buf1, 2);
                memcpy(dst_a + stride[0],meta.buf2, 2);
                memcpy(dst_b, meta.buf3, 1);
                memcpy(dst_c, meta.buf5, 1);
            }
        }
}
template <Pack pack, Colorspace cs> inline void pack_in(Context &ctx, ConvertMeta& meta)
{
    if(cs == YUV422){
            ctx.b1 = ctx.b2 =  (ctx.b1 + ctx.b2) / 2;
            ctx.c1 = ctx.c2 =  (ctx.c1 + ctx.c2) / 2;
            ctx.b3 = ctx.b4 =  (ctx.b3 + ctx.b4) / 2;
            ctx.c3 = ctx.c4 =  (ctx.c3 + ctx.c4) / 2;
    }
    if(cs == YUV420){
            ctx.b1 = (ctx.b1 + ctx.b2 + ctx.b3 + ctx.b4) / 4;
            ctx.c1 = (ctx.c1 + ctx.c2 + ctx.c3 + ctx.c4) / 4;
    }
	if (pack == Interleaved){
        // 1,[2,3]
        // 4,[5,6]
        if(cs == RGB || cs == YUV444){
            meta.buf1[0] = ctx.a1;
            meta.buf1[1] = ctx.b1;
            meta.buf1[2] = ctx.c1;

            meta.buf1[3] = ctx.a2;
            meta.buf1[4] = ctx.b2;
            meta.buf1[5] = ctx.c2;

            meta.buf4[0] = ctx.a3;
            meta.buf4[1] = ctx.b3;
            meta.buf4[2] = ctx.c3;

            meta.buf4[3] = ctx.a4;
            meta.buf4[4] = ctx.b4;
            meta.buf4[5] = ctx.c4;
            return;
        }
        if(cs == A2R10G10B10)
        {
            pack_A2R10G10B10(ctx.a1, ctx.b1, ctx.c1, meta.buf1);
            pack_A2R10G10B10(ctx.a2, ctx.b2, ctx.c2, meta.buf1 + 4);
            pack_A2R10G10B10(ctx.a3, ctx.b3, ctx.c3, meta.buf4);
            pack_A2R10G10B10(ctx.a4, ctx.b4, ctx.c4, meta.buf4 + 4);
        }
        if(cs == YUV422){
            meta.buf1[0] = ctx.a1;
            meta.buf1[1] = ctx.b1;

            meta.buf1[2] = ctx.a2;
            meta.buf1[3] = ctx.c2;

            meta.buf4[0] = ctx.a3;
            meta.buf4[1] = ctx.b3;

            meta.buf4[2] = ctx.a4;
            meta.buf4[3] = ctx.c4;
        }
	} else  { // planar
		//p1 = 1_2
        //p2 = 3_4
        //p3 = 5_6
        if(cs == RGB || cs == YUV444){
            meta.buf1[0] = ctx.a1;
            meta.buf1[1] = ctx.a2;

            meta.buf2[0] = ctx.a3;
            meta.buf2[1] = ctx.a4;

            meta.buf3[0] = ctx.b1;
            meta.buf3[1] = ctx.b2;

            meta.buf4[0] = ctx.b3;
            meta.buf4[1] = ctx.b4;

            meta.buf5[0] = ctx.c1;
            meta.buf5[1] = ctx.c2;

            meta.buf6[0] = ctx.c3;
            meta.buf6[1] = ctx.c4;
        }
        if(cs == YUV422){
        // p1 = 1,2
        // p2 = 3
        // p3 = 5
            meta.buf1[0] = ctx.a1;
            meta.buf1[1] = ctx.a2;

            meta.buf2[0] = ctx.a3;
            meta.buf2[1] = ctx.a4;

            meta.buf3[0] = ctx.b1;
            meta.buf3[1] = ctx.b3;

            meta.buf5[0] = ctx.c1;
            meta.buf5[1] = ctx.c3;
        }
        if(cs == YUV420){
        // p1 = 1,2
        // p2 = 3
        // p3 = 5
            meta.buf1[0] = ctx.a1;
            meta.buf1[1] = ctx.a2;
            meta.buf2[0] = ctx.a3;
            meta.buf2[1] = ctx.a4;

            meta.buf3[0] = ctx.b1;

            meta.buf5[0] = ctx.c1;
        }
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

template <Colorspace cs> inline void offset_yuv (int32_t& y, int32_t& u, int32_t& v, int32_t offset_y, int32_t offset_u, int32_t offset_v)
{
	if (cs == YUV444 || cs == YUV422 || cs == YUV420) {
		y += offset_y;
		u += offset_u;
		v += offset_v;
	}

}

template <Colorspace cs> static inline void clip_result (int32_t& val_a, int32_t& val_b, int32_t& val_c)
{
    //puts("before");
    //std::cout << val_a << " "<< val_b << " " << val_c << std::endl;
    if(cs == RGB || cs == A2R10G10B10){
        val_a = clip(val_a, 16 << 10, 235 << 10);
        val_b = clip(val_b, 16 << 10, 235 << 10);
        val_c = clip(val_c, 16 << 10, 235 << 10);
    }
    else
    if(cs == YUV444 || cs == YUV422 || cs == YUV420 ){
        val_a = clip(val_a, 16 << 10, 235 << 10);
        val_b = clip(val_b, 16 << 10, 240 << 10);
        val_c = clip(val_c, 16 << 10, 240 << 10);
    }
    //puts("after");
    //std::cout << val_a << " "<< val_b << " " << val_c << std::endl;

}

/*
 * | a |   |                  |   | a'|
 * | b | * | transform_matrix | = | b'|
 * | c |   |                  |   | c'|
 */
static inline void mat_mul(int32_t &a, int32_t &b, int32_t &c, const int16_t transform_matrix[3 * 3])
{
	int32_t tmp1 = transform_matrix[0*3 + 0] * a + transform_matrix[0*3 + 1] * b + transform_matrix[0*3 + 2] * c;
	int32_t tmp2 = transform_matrix[1*3 + 0] * a + transform_matrix[1*3 + 1] * b + transform_matrix[1*3 + 2] * c;
	int32_t tmp3 = transform_matrix[2*3 + 0] * a + transform_matrix[2*3 + 1] * b + transform_matrix[2*3 + 2] * c;

    a = tmp1;
	b = tmp2;
	c = tmp3;
}
inline void complex_shift( int32_t& a, int32_t& b, int32_t& c, int n){
    a = round_shift(a, n);
	b = round_shift(b, n);
	c = round_shift(c, n);
}

template <Colorspace cs> inline void scale_from(int32_t& val_a, int32_t& val_b, int32_t& val_c)
{
    if(cs == RGB || cs == YUV444 || cs == YUV420 || cs == YUV422)
    {
        val_a <<= 2;
        val_b <<= 2;
        val_c <<= 2;
        return;
    }
    if(cs == A2R10G10B10)
    {
        val_a *= 219 << 2;
        val_b *= 219 << 2;
        val_c *= 219 << 2;

        val_a >>= 10;
        val_b >>= 10;
        val_c >>= 10;

        val_a += 16 << 2;
        val_b += 16 << 2;
        val_c += 16 << 2;
        return ;
    }
    assert(0);
    //TODO unsupported formats
}

template <Colorspace cs> inline void scale_to(int32_t& val_a, int32_t& val_b, int32_t& val_c)
{
    if(cs == RGB || cs == YUV444 || cs == YUV420 || cs == YUV422)
    {
        val_a >>= 2;
        val_b >>= 2;
        val_c >>= 2;
        return;
    }
    if(cs == A2R10G10B10)
    {
        val_a -= 16 << 2;
        val_b -= 16 << 2;
        val_c -= 16 << 2;

        val_a  <<= 10;
        val_b  <<= 10;
        val_c  <<= 10;

        val_a /= 219;
        val_b /= 219;
        val_c /= 219;

        return;

    }
    assert(0);
}
template <Colorspace from, Colorspace to> inline void transform (Context &ctx, const int16_t transform_matrix[3 * 3])
{


#ifdef ENABLE_LOG
    std::cout << ctx.a1 << " "<< ctx.b1 << " "<< ctx.c1 << std::endl;
    std::cout << ctx.a2 << " "<< ctx.b2 << " "<< ctx.c2 << std::endl;
    std::cout << ctx.a3 << " "<< ctx.b3 << " "<< ctx.c3 << std::endl;
    std::cout << ctx.a4 << " "<< ctx.b4 << " "<< ctx.c4 << std::endl;
    std::cout << "1||||||||||||||||||||||||||||||||||||||||||||||" << std::endl;
#endif

    //offset_yuv <from> (ctx.a1, ctx.b1, ctx.c1, 16, 128, 128); // Y offset is n't necessary in reference
    scale_from <from>(ctx.a1, ctx.b1, ctx.c1);
    scale_from <from>(ctx.a2, ctx.b2, ctx.c2);
    scale_from <from>(ctx.a3, ctx.b3, ctx.c3);
    scale_from <from>(ctx.a4, ctx.b4, ctx.c4);
    char from_type = (from == RGB || from == A2R10G10B10)? (1 << 1) : (1 << 2);
    char to_type = (to == RGB || to == A2R10G10B10)? (1 << 1) : (1 << 2);
    if (from_type != to_type)
    {
    #ifdef ENABLE_LOG
        std::cout << ctx.a1 << " "<< ctx.b1 << " "<< ctx.c1 << std::endl;
        std::cout << ctx.a2 << " "<< ctx.b2 << " "<< ctx.c2 << std::endl;
        std::cout << ctx.a3 << " "<< ctx.b3 << " "<< ctx.c3 << std::endl;
        std::cout << ctx.a4 << " "<< ctx.b4 << " "<< ctx.c4 << std::endl;
        std::cout << "2||||||||||||||||||||||||||||||||||||||||||||||" << std::endl;
    #endif

        offset_yuv <from> (ctx.a1, ctx.b1, ctx.c1, 0, -128 * 4, -128 * 4);
        offset_yuv <from> (ctx.a2, ctx.b2, ctx.c2, 0, -128 * 4, -128 * 4);
        offset_yuv <from> (ctx.a3, ctx.b3, ctx.c3, 0, -128 * 4, -128 * 4);
        offset_yuv <from> (ctx.a4, ctx.b4, ctx.c4, 0, -128 * 4, -128 * 4);

    #ifdef ENABLE_LOG
        std::cout << ctx.a1 << " "<< ctx.b1 << " "<< ctx.c1 << std::endl;
        std::cout << ctx.a2 << " "<< ctx.b2 << " "<< ctx.c2 << std::endl;
        std::cout << ctx.a3 << " "<< ctx.b3 << " "<< ctx.c3 << std::endl;
        std::cout << ctx.a4 << " "<< ctx.b4 << " "<< ctx.c4 << std::endl;
        std::cout << "3||||||||||||||||||||||||||||||||||||||||||||||" << std::endl;
    #endif


        // koeffs shifted right on 8

        mat_mul(ctx.a1, ctx.b1, ctx.c1, transform_matrix);
        mat_mul(ctx.a2, ctx.b2, ctx.c2, transform_matrix);
        mat_mul(ctx.a3, ctx.b3, ctx.c3, transform_matrix);
        mat_mul(ctx.a4, ctx.b4, ctx.c4, transform_matrix);

    #ifdef ENABLE_LOG
        std::cout << ctx.a1 << " "<< ctx.b1 << " "<< ctx.c1 << std::endl;
        std::cout << ctx.a2 << " "<< ctx.b2 << " "<< ctx.c2 << std::endl;
        std::cout << ctx.a3 << " "<< ctx.b3 << " "<< ctx.c3 << std::endl;
        std::cout << ctx.a4 << " "<< ctx.b4 << " "<< ctx.c4 << std::endl;
        std::cout << "4||||||||||||||||||||||||||||||||||||||||||||||" << std::endl;
    #endif
        //offset_yuv <to> (ctx.a1, ctx.b1, ctx.c1, 16, 128, 128); // Y offset is n't necessary in reference
        offset_yuv <to> (ctx.a1, ctx.b1, ctx.c1, 0, 128 << 10, 128 << 10);
        offset_yuv <to> (ctx.a2, ctx.b2, ctx.c2, 0, 128 << 10, 128 << 10);
        offset_yuv <to> (ctx.a3, ctx.b3, ctx.c3, 0, 128 << 10, 128 << 10);
        offset_yuv <to> (ctx.a4, ctx.b4, ctx.c4, 0, 128 << 10, 128 << 10);

    #ifdef ENABLE_LOG
        std::cout << ctx.a1 << " "<< ctx.b1 << " "<< ctx.c1 << std::endl;
        std::cout << ctx.a2 << " "<< ctx.b2 << " "<< ctx.c2 << std::endl;
        std::cout << ctx.a3 << " "<< ctx.b3 << " "<< ctx.c3 << std::endl;
        std::cout << ctx.a4 << " "<< ctx.b4 << " "<< ctx.c4 << std::endl;
        std::cout << "5||||||||||||||||||||||||||||||||||||||||||||||" << std::endl;
    #endif
        clip_result <to> (ctx.a1, ctx.b1, ctx.c1);
        clip_result <to> (ctx.a2, ctx.b2, ctx.c2);
        clip_result <to> (ctx.a3, ctx.b3, ctx.c3);
        clip_result <to> (ctx.a4, ctx.b4, ctx.c4);

    #ifdef ENABLE_LOG
        std::cout << ctx.a1 << " "<< ctx.b1 << " "<< ctx.c1 << std::endl;
        std::cout << ctx.a2 << " "<< ctx.b2 << " "<< ctx.c2 << std::endl;
        std::cout << ctx.a3 << " "<< ctx.b3 << " "<< ctx.c3 << std::endl;
        std::cout << ctx.a4 << " "<< ctx.b4 << " "<< ctx.c4 << std::endl;
        std::cout << "6||||||||||||||||||||||||||||||||||||||||||||||" << std::endl;
    #endif
        complex_shift(ctx.a1, ctx.b1, ctx.c1, 8);
        complex_shift(ctx.a2, ctx.b2, ctx.c2, 8);
        complex_shift(ctx.a3, ctx.b3, ctx.c3, 8);
        complex_shift(ctx.a4, ctx.b4, ctx.c4, 8);

    #ifdef ENABLE_LOG
        std::cout << ctx.a1 << " "<< ctx.b1 << " "<< ctx.c1 << std::endl;
        std::cout << ctx.a2 << " "<< ctx.b2 << " "<< ctx.c2 << std::endl;
        std::cout << ctx.a3 << " "<< ctx.b3 << " "<< ctx.c3 << std::endl;
        std::cout << ctx.a4 << " "<< ctx.b4 << " "<< ctx.c4 << std::endl;
        std::cout << "7||||||||||||||||||||||||||||||||||||||||||||||" << std::endl;
    #endif
    } // if(from_type != to_type)
    scale_to < to >(ctx.a1, ctx.b1, ctx.c1);
    scale_to < to >(ctx.a2, ctx.b2, ctx.c2);
    scale_to < to >(ctx.a3, ctx.b3, ctx.c3);
    scale_to < to >(ctx.a4, ctx.b4, ctx.c4);

#ifdef ENABLE_LOG
    std::cout << ctx.a1 << " "<< ctx.b1 << " "<< ctx.c1 << std::endl;
    std::cout << ctx.a2 << " "<< ctx.b2 << " "<< ctx.c2 << std::endl;
    std::cout << ctx.a3 << " "<< ctx.b3 << " "<< ctx.c3 << std::endl;
    std::cout << ctx.a4 << " "<< ctx.b4 << " "<< ctx.c4 << std::endl;
    std::cout << "8*************************************" << std::endl;
#endif

    }

template <Colorspace cs , Pack pack> inline void next_row (uint8_t* &ptr_a, uint8_t* &ptr_b, uint8_t* &ptr_c, const size_t stride[3])
{
	bool interleaved = (pack == Interleaved);
	if (interleaved) {
		ptr_a += stride[0] * 2;
	} else {
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
            // w/2 (for 2 line)
            // w/2 (for 2 line)
            ptr_a += stride[0] * 2;
            ptr_b += stride[1];
            ptr_c += stride[2];
        }
	}
}

template<Colorspace from_cs , Pack from_pack, Colorspace to_cs, Pack to_pack, Standard st> void colorspace_convert(ConvertMeta& meta)
{
	const int16_t *transform_matrix = get_transfrom_coeffs<from_cs, to_cs, st> ();

	uint8_t* src_a = meta.src_data[0];
	uint8_t* src_b = meta.src_data[1];
	uint8_t* src_c = meta.src_data[2];

	uint8_t* dst_a = meta.dst_data[0];
	uint8_t* dst_b = meta.dst_data[1];
	uint8_t* dst_c = meta.dst_data[2];

	for(size_t y = 0; y < meta.height; y += 2) {
		for(size_t x = 0; x < meta.width; x += 2) {
			// Process 2x2 pixels
			Context context;
            size_t shift_a, shift_b, shift_c;
            get_pos <from_pack, from_cs>(shift_a, shift_b, shift_c, x);
			load <from_pack, from_cs> (meta,
                              src_a + shift_a,
                              src_b + shift_b,
                              src_c + shift_c,
                              meta.src_stride);
            unpack <from_pack, from_cs>(meta, context);

				transform <from_cs, to_cs> (context, transform_matrix);


			get_pos <to_pack, to_cs>(shift_a, shift_b, shift_c, x);
			std::cout << "converted \n";
			pack_in<to_pack, to_cs > (context, meta);
			std::cout  << "packed\n";
			store<to_pack, to_cs > (meta,
											 dst_a + shift_a,
											 dst_b + shift_b,
											 dst_c + shift_c,
											 meta.dst_stride);
            std::cout  << "stored\n";
		}
		next_row <from_cs, from_pack> (src_a, src_b, src_c, meta.src_stride);
		next_row <to_cs, to_pack> (dst_a, dst_b, dst_c, meta.dst_stride);
	}
}
} // namespace ColorspaceConverter;

#endif
