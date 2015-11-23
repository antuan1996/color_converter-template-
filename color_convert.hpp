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
enum Range
{
    FULL_RANGE,
    NORM_RANGE
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
        if(pack == SemiPlanar){
            posa = cur_pos;
            posb = cur_pos * 2;
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

    // TODO: Usupported transform
    //assert(0);
}

template <Colorspace from, Colorspace to, Standard st> void set_transfrom_coeffs(int32_t* res_matrix)
{
   const int32_t* matrix = e_matrix;
	if (st == BT_601) {
        if ((from == RGB || from == A2R10G10B10) && (to == YUV444 || to == YUV422 || to == YUV420) ) {
			matrix =  k_bt601_RGB_to_YUV;
		}
        else
		if ((from == YUV444 || from == YUV422 || from == YUV420) && (to == RGB || to == A2R10G10B10) ) {
			matrix =  k_bt601_YUV_to_RGB;
		}
	}
    else
    if (st == BT_709) {
		if ((from == RGB || from == A2R10G10B10) && (to == YUV444 || to == YUV422  || to == YUV420 ) ) {
			matrix =  k_bt709_RGB_to_YUV;
		}
        else
		if ((from == YUV444 || from == YUV422 || from == YUV420) && (to == RGB || to == A2R10G10B10)) {
			matrix = k_bt709_YUV_to_RGB;
		}
		// TODO: Usupported transform
		//else
        //assert(0);
	}
    else
    if (st == BT_2020) {
        if ((from == RGB || from == A2R10G10B10) && (to == YUV444 || to == YUV422  || to == YUV420) ) {
			matrix = k_bt2020_RGB_to_YUV;
		}
		if((from == YUV444 || from == YUV422 || from == YUV420) && (to == RGB || to == A2R10G10B10)) {
			matrix =  k_bt2020_YUV_to_RGB;
		}
		// TODO: Usupported transform
		//assert(0);
	}
	// copying from const to mutable
    for(int r=0; r < 3; ++r)
    {
        res_matrix[r*3 + 0] = matrix[ r*3 + 0 ];
        res_matrix[r*3 + 1] = matrix[ r*3 + 1 ];
        res_matrix[r*3 + 2] = matrix[ r*3 + 2 ];
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
template <Pack pack, Colorspace cs> inline void load(ConvertMeta& meta, Context& ctx,  const uint8_t *srca, const uint8_t *srcb, const uint8_t *srcc)
{
    const uint8_t* src_na = srca + meta.src_stride[0];
    const uint8_t* src_nb = srcb + meta.src_stride[1];
    const uint8_t* src_nc = srcc + meta.src_stride[2];

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
            unpack_A2R10G10B10(ctx.a1, ctx.b1, ctx.c1, srca);
            unpack_A2R10G10B10(ctx.a2, ctx.b2, ctx.c2, srca + 4);
            unpack_A2R10G10B10(ctx.a3, ctx.b3, ctx.c3, src_na);
            unpack_A2R10G10B10(ctx.a4, ctx.b4, ctx.c4, src_na + 4);
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
template <Pack pack, Colorspace cs> inline void unpack (Context &ctx)
{
    if (pack == Interleaved){
        if(cs == YUV422){
            ctx.c2 = ctx.c1;
            ctx.b2 = ctx.b1;

            ctx.c4 = ctx.c3;
            ctx.b4 = ctx.b3;
        }
	}
	else  if( pack == Planar)
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
}

template <Pack pack, Colorspace cs> inline void store(const ConvertMeta& meta, Context& ctx,  uint8_t* dsta, uint8_t* dstb, uint8_t* dstc)
{
    uint8_t* dst_na = dsta + meta.dst_stride[0];
    uint8_t* dst_nb = dstb + meta.dst_stride[1];
    uint8_t* dst_nc = dstc + meta.dst_stride[2];

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
            pack_A2R10G10B10(ctx.a1, ctx.b1, ctx.c1, dsta);
            pack_A2R10G10B10(ctx.a2, ctx.b2, ctx.c2, dsta + 4);
            pack_A2R10G10B10(ctx.a3, ctx.b3, ctx.c3, dst_na);
            pack_A2R10G10B10(ctx.a4, ctx.b4, ctx.c4, dst_na + 4);
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
}

template <Pack pack, Colorspace cs> inline void pack_in(Context &ctx)
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

template <Colorspace cs, Range range> inline void offset_yuv (int32_t& y, int32_t& u, int32_t& v, int32_t offset_y, int32_t offset_u, int32_t offset_v)
{
	if(range == FULL_RANGE)
        return;
    //if range == normal
	if (cs == YUV444 || cs == YUV422 || cs == YUV420) {
		y += offset_y;
		u += offset_u;
		v += offset_v;
	}
}

template <Colorspace cs, Range range> inline void offset_rgb (int32_t& r, int32_t& g, int32_t& b, int32_t offset_r, int32_t offset_g, int32_t offset_b)
{
	if(range == FULL_RANGE)
        return;
    // if range == normal
	if (cs == RGB || cs == A2R10G10B10) {
		r += offset_r;
		g += offset_g;
		b += offset_b;
	}
}

template <Colorspace cs> static inline void clip_result (int32_t& val_a, int32_t& val_b, int32_t& val_c)
{
    //puts("before");
    //std::cout << val_a << " "<< val_b << " " << val_c << std::endl;
    uint32_t extra_shift = (cs == A2R10G10B10)? 2 : 0;
    if(cs == RGB || cs == A2R10G10B10){
        val_a = clip(val_a, 16 << (8 + extra_shift), 235 << (8 + extra_shift));
        val_b = clip(val_b, 16 << (8 + extra_shift), 235 << (8 + extra_shift));
        val_c = clip(val_c, 16 << (8 + extra_shift), 235 << (8 + extra_shift));
    }
    else
    if(cs == YUV444 || cs == YUV422 || cs == YUV420 ){
        val_a = clip(val_a, 16 << (8 + extra_shift), 235 << (8 + extra_shift));
        val_b = clip(val_b, 16 << (8 + extra_shift), 240 << (8 + extra_shift));
        val_c = clip(val_c, 16 << (8 + extra_shift), 240 << (8 + extra_shift));
    }
    //puts("after");
    //std::cout << val_a << " "<< val_b << " " << val_c << std::endl;

}

/*
 * | a |   |                  |   | a'|
 * | b | * | transform_matrix | = | b'|
 * | c |   |                  |   | c'|
 */
static inline void mat_mul(int32_t &a, int32_t &b, int32_t &c, const int32_t transform_matrix[3 * 3])
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

template <Colorspace from, Colorspace to> inline void scale(int32_t& val_a, int32_t& val_b, int32_t& val_c)
{
    bool from8bit =  (from == RGB || from == YUV444 || from == YUV420 || from == YUV422);
    bool from10bit = (from == A2R10G10B10);
    bool to8bit =  (to == RGB || to == YUV444 || to == YUV420 || to == YUV422);
    bool to10bit = (to == A2R10G10B10);
    if( (from8bit && to8bit) || (from10bit && to10bit) )
        return;
    if(from8bit && to10bit)
    {
        val_a <<= 2;
        val_b <<= 2;
        val_c <<= 2;
        return;
    }
    if(from10bit && to8bit)
    {
        val_a >>= 2;
        val_b >>= 2;
        val_c >>= 2;
        return;
    }
    assert(0);
    //TODO unsupported formats
}

template <Colorspace cs_from, Range from_range, Colorspace cs_to,  Range to_range> inline void convert_range(int32_t* matrix)
{
    if(from_range == FULL_RANGE)
    {
        if(cs_from == RGB)
        {
            for(int r = 0 ; r<3; ++r)
            {
                matrix[r * 3 + 0] *= 219;
                matrix[r * 3 + 1] *= 219;
                matrix[r * 3 + 2] *= 219;

                matrix[r * 3 + 0] >>= 8;
                matrix[r * 3 + 1] >>= 8;
                matrix[r * 3 + 2] >>= 8;
            }
        }
        if( cs_from == YUV444 || cs_from == YUV422 || cs_from == YUV420)
        {
            for(int r = 0 ; r<3; ++r)
            {
                matrix[r * 3 + 0] *= 219;
                matrix[r * 3 + 1] *= 224;
                matrix[r * 3 + 2] *= 224;

                matrix[r * 3 + 0] >>= 8;
                matrix[r * 3 + 1] >>= 8;
                matrix[r * 3 + 2] >>= 8;
            }
        }
        if(cs_from == A2R10G10B10 )
        {
            for(int r = 0; r < 3; ++r)
            {
                matrix[r * 3 + 0] *= 219 << 2;
                matrix[r * 3 + 1] *= 219 << 2;
                matrix[r * 3 + 2] *= 219 << 2;

                matrix[r * 3 + 0] >>= 10;
                matrix[r * 3 + 1] >>= 10;
                matrix[r * 3 + 2] >>= 10;
            }
        }
    }// from_range == FULL_RANGE

    if( to_range == FULL_RANGE )
    {
        if(cs_to == RGB)
        {
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
        if( cs_to == YUV444 || cs_to == YUV422 || cs_to == YUV420 )
        {
            for(int r = 0; r < 3; ++r)
            {
                matrix[r * 3 + 0] <<= 8;
                matrix[r * 3 + 1] <<= 8;
                matrix[r * 3 + 2] <<= 8;

                matrix[r * 3 + 0] /= 219;
                matrix[r * 3 + 1] /= 224;
                matrix[r * 3 + 2] /= 224;
            }
        }
        if(cs_to == A2R10G10B10 )
        {
            for(int r = 0; r < 3; ++r)
            {
                matrix[r * 3 + 0] <<= 10;
                matrix[r * 3 + 1] <<= 10;
                matrix[r * 3 + 2] <<= 10;

                matrix[r * 3 + 0] /= 219 << 2;
                matrix[r * 3 + 1] /= 219 << 2;
                matrix[r * 3 + 2] /= 219 << 2;
            }
        }
    }//cs_to == FULL_RANGE

    // TODO Unsupported formats
    //assert(0);
}

template <Colorspace from_cs, Range from_range, Colorspace to_cs, Range to_range > inline void transform (Context &ctx, const int32_t transform_matrix[3 * 3])
{

#ifdef ENABLE_LOG
    std::cout << ctx.a1 << " "<< ctx.b1 << " "<< ctx.c1 << std::endl;
    std::cout << ctx.a2 << " "<< ctx.b2 << " "<< ctx.c2 << std::endl;
    std::cout << ctx.a3 << " "<< ctx.b3 << " "<< ctx.c3 << std::endl;
    std::cout << ctx.a4 << " "<< ctx.b4 << " "<< ctx.c4 << std::endl;
    std::cout << "1||||||||||||||||||||||||||||||||||||||||||||||" << std::endl;
#endif
    uint32_t extra_shift = (to_cs == A2R10G10B10)? 2 : 0;
    //offset_yuv <from_cs> (ctx.a1, ctx.b1, ctx.c1, 16, 128, 128); // Y offset is n't necessary in reference

//  matrix multiple
    {

    #ifdef ENABLE_LOG
        std::cout << ctx.a1 << " "<< ctx.b1 << " "<< ctx.c1 << std::endl;
        std::cout << ctx.a2 << " "<< ctx.b2 << " "<< ctx.c2 << std::endl;
        std::cout << ctx.a3 << " "<< ctx.b3 << " "<< ctx.c3 << std::endl;
        std::cout << ctx.a4 << " "<< ctx.b4 << " "<< ctx.c4 << std::endl;
        std::cout << "2||||||||||||||||||||||||||||||||||||||||||||||" << std::endl;
    #endif

        offset_rgb <from_cs, from_range> (ctx.a1, ctx.b1, ctx.c1, -16 << extra_shift, -16 << extra_shift, -16 << extra_shift);
        offset_rgb <from_cs, from_range> (ctx.a2, ctx.b2, ctx.c2, -16 << extra_shift, -16 << extra_shift, -16 << extra_shift);
        offset_rgb <from_cs, from_range> (ctx.a3, ctx.b3, ctx.c3, -16 << extra_shift, -16 << extra_shift, -16 << extra_shift);
        offset_rgb <from_cs, from_range> (ctx.a4, ctx.b4, ctx.c4, -16 << extra_shift, -16 << extra_shift, -16 << extra_shift);

        offset_yuv <from_cs, from_range> (ctx.a1, ctx.b1, ctx.c1, -16 << extra_shift, -128 << extra_shift, -128 << extra_shift);
        offset_yuv <from_cs, from_range> (ctx.a2, ctx.b2, ctx.c2, -16 << extra_shift, -128 << extra_shift, -128 << extra_shift);
        offset_yuv <from_cs, from_range> (ctx.a3, ctx.b3, ctx.c3, -16 << extra_shift, -128 << extra_shift, -128 << extra_shift);
        offset_yuv <from_cs, from_range> (ctx.a4, ctx.b4, ctx.c4, -16 << extra_shift, -128 << extra_shift, -128 << extra_shift);

    #ifdef ENABLE_LOG
        std::cout << ctx.a1 << " "<< ctx.b1 << " "<< ctx.c1 << std::endl;
        std::cout << ctx.a2 << " "<< ctx.b2 << " "<< ctx.c2 << std::endl;
        std::cout << ctx.a3 << " "<< ctx.b3 << " "<< ctx.c3 << std::endl;
        std::cout << ctx.a4 << " "<< ctx.b4 << " "<< ctx.c4 << std::endl;
        std::cout << "3||||||||||||||||||||||||||||||||||||||||||||||" << std::endl;
    #endif

        scale<from_cs, to_cs>(ctx.a1, ctx.b1, ctx.c1);
        scale<from_cs, to_cs>(ctx.a2, ctx.b2, ctx.c2);
        scale<from_cs, to_cs>(ctx.a3, ctx.b3, ctx.c3);
        scale<from_cs, to_cs>(ctx.a4, ctx.b4, ctx.c4);


    #ifdef ENABLE_LOG
        // koeffs shifted right on 8
        for(int r = 0 ; r < 3; ++r)
            std::cout << transform_matrix[r*3 + 0] << " " << transform_matrix[r*3 + 1] << " " << transform_matrix[r*3 + 2] << "\n";
    #endif

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

        offset_rgb <to_cs, to_range> (ctx.a1, ctx.b1, ctx.c1, 16 << (8 + extra_shift), 16 << (8 + extra_shift), 16 << (8 + extra_shift));
        offset_rgb <to_cs, to_range> (ctx.a2, ctx.b2, ctx.c2, 16 << (8 + extra_shift), 16 << (8 + extra_shift), 16 << (8 + extra_shift));
        offset_rgb <to_cs, to_range> (ctx.a3, ctx.b3, ctx.c3, 16 << (8 + extra_shift), 16 << (8 + extra_shift), 16 << (8 + extra_shift));
        offset_rgb <to_cs, to_range> (ctx.a4, ctx.b4, ctx.c4, 16 << (8 + extra_shift), 16 << (8 + extra_shift), 16 << (8 + extra_shift));


        offset_yuv <to_cs, to_range> (ctx.a1, ctx.b1, ctx.c1, 16 << (8 + extra_shift), 128 << (8 + extra_shift), 128 << (8 + extra_shift));
        offset_yuv <to_cs, to_range> (ctx.a2, ctx.b2, ctx.c2, 16 << (8 + extra_shift), 128 << (8 + extra_shift), 128 << (8 + extra_shift));
        offset_yuv <to_cs, to_range> (ctx.a3, ctx.b3, ctx.c3, 16 << (8 + extra_shift), 128 << (8 + extra_shift), 128 << (8 + extra_shift));
        offset_yuv <to_cs, to_range> (ctx.a4, ctx.b4, ctx.c4, 16 << (8 + extra_shift), 128 << (8 + extra_shift), 128 << (8 + extra_shift));



    #ifdef ENABLE_LOG
        std::cout << ctx.a1 << " "<< ctx.b1 << " "<< ctx.c1 << std::endl;
        std::cout << ctx.a2 << " "<< ctx.b2 << " "<< ctx.c2 << std::endl;
        std::cout << ctx.a3 << " "<< ctx.b3 << " "<< ctx.c3 << std::endl;
        std::cout << ctx.a4 << " "<< ctx.b4 << " "<< ctx.c4 << std::endl;
        std::cout << "5||||||||||||||||||||||||||||||||||||||||||||||" << std::endl;
    #endif
        clip_result <to_cs> (ctx.a1, ctx.b1, ctx.c1);
        clip_result <to_cs> (ctx.a2, ctx.b2, ctx.c2);
        clip_result <to_cs> (ctx.a3, ctx.b3, ctx.c3);
        clip_result <to_cs> (ctx.a4, ctx.b4, ctx.c4);

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
    } // if(from_cs_type != to_type)

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
	if (Interleaved) {
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
            // w/2 (for 2 lines)
            // w/2 (for 2 lines)
            ptr_a += stride[0] * 2;
            ptr_b += stride[1];
            ptr_c += stride[2];
        }
	}
}

template<Colorspace from_cs , Pack from_pack, Range from_range, Colorspace to_cs, Pack to_pack, Range to_range, Standard st> void colorspace_convert(ConvertMeta& meta)
{
	int32_t transform_matrix[ 3 * 3 ];
    set_transfrom_coeffs<from_cs, to_cs, st>(transform_matrix);

    #ifdef ENABLE_LOG
        // matrix koeffs
        for(int r = 0 ; r < 3; ++r)
            std::cout << transform_matrix[r*3 + 0] << " " << transform_matrix[r*3 + 1] << " " << transform_matrix[r*3 + 2] << "\n";

    #endif

    convert_range <from_cs, from_range, to_cs, to_range>(transform_matrix);

	uint8_t* src_a = meta.src_data[0];
	uint8_t* src_b = meta.src_data[1];
	uint8_t* src_c = meta.src_data[2];

	uint8_t* dst_a = meta.dst_data[0];
	uint8_t* dst_b = meta.dst_data[1];
	uint8_t* dst_c = meta.dst_data[2];
    Context context;

	for(size_t y = 0; y < meta.height; y += 2) {
		for(size_t x = 0; x < meta.width; x += 2) {
			// Process 2x2 pixels
			size_t shift_a, shift_b, shift_c;
            get_pos <from_pack, from_cs>(shift_a, shift_b, shift_c, x);
			load <from_pack, from_cs> (meta, context,
                                        src_a + shift_a,
                                        src_b + shift_b,
                                        src_c + shift_c);
            unpack <from_pack, from_cs>(context);
            #ifdef ENABLE_LOG
            std::cout  << "loaded" << std::endl;
            #endif // ENABLE_LOG
            transform <from_cs, from_range, to_cs, to_range> (context, transform_matrix);


			get_pos <to_pack, to_cs>(shift_a, shift_b, shift_c, x);

			#ifdef ENABLE_LOG
			std::cout << "converted \n";
            #endif // ENABLE_LOG

			pack_in<to_pack, to_cs > (context);

			#ifdef ENABLE_LOG
			std::cout << "packed \n";
            #endif // ENABLE_LOG


			store<to_pack, to_cs > (meta, context,
											 dst_a + shift_a,
											 dst_b + shift_b,
											 dst_c + shift_c);
            #ifdef ENABLE_LOG
			std::cout << "stored\n";
            #endif // ENABLE_LOG

		}
		next_row <from_cs, from_pack> (src_a, src_b, src_c, meta.src_stride);
		next_row <to_cs, to_pack> (dst_a, dst_b, dst_c, meta.dst_stride);
	}
}
} // namespace ColorspaceConverter;

#endif
