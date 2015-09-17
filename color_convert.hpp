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
	RGB
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
    uint8_t  buf1[2];
    uint8_t  buf2[2];
    uint8_t  buf3[2];
    uint8_t  buf4[2];
    uint8_t  buf5[2];
    uint8_t  buf6[2];
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
static const size_t k_yuv422_steps[3] = { 2, 1, 1 };
static const size_t k_yuv444_steps[3] = { 1, 1, 1 };

template<Pack pack, Colorspace cs> const size_t* get_pel_step ()
{
	if (cs == RGB) {
		return k_rgb_steps;
	}
	if (cs == YUV444) {
		return k_yuv444_steps;
	}
    // TODO: Usupported transform
    assert(0);
}

template <Colorspace from, Colorspace to, Standard Standard> const int16_t* get_transfrom_coeffs()
{
	if (from == to) {
		return nullptr;
	}
	if (Standard == BT_601) {
		if (from == RGB && to == YUV444) {
			return k_bt601_RGB_to_YUV;
		}
		if (from == YUV444 && to == RGB) {
			return k_bt601_YUV_to_RGB;
		}
		// TODO: Usupported transform
		assert(0);
	}
    if (Standard == BT_709) {
		if (from == RGB && to == YUV444) {
			return k_bt709_RGB_to_YUV;
		}
		if (from == YUV444 && to == RGB) {
			return k_bt709_YUV_to_RGB;
		}
		// TODO: Usupported transform
		assert(0);
	}
	// TODO: Usupported standard
	assert(0);
	return nullptr;
}
template <Pack pack> inline void load(ConvertMeta& meta, const uint8_t *src_a, const uint8_t *src_b, const uint8_t *src_c, const size_t stride[3]){
    if(pack == Interleaved ){
        memcpy(meta.buf1, src_a + 0, 2);
        memcpy(meta.buf2, src_a + 2, 2);
        memcpy(meta.buf3, src_a + 4, 2);
        
        memcpy(meta.buf3, src_a + stride[0] + 0, 2);
        memcpy(meta.buf3, src_a + stride[0] + 2 ,2);
        memcpy(meta.buf3, src_a + stride[0] + 4, 2);
    }
    else
        if(pack == Planar){
            memcpy(meta.buf1, src_a, 2);
            memcpy(meta.buf2, src_a + stride[0], 2);
            
            memcpy(meta.buf3, src_b, 2);
            memcpy(meta.buf4, src_b + stride[1], 2);
            
            memcpy(meta.buf5, src_c, 2);
            memcpy(meta.buf6, src_c + stride[2], 2);
        }
}
template <Pack pack> inline void unpack (ConvertMeta& meta, Context &ctx)
{
	if (pack == Interleaved) {
		ctx.a1 = meta.buf1[0];
		ctx.b1 = meta.buf1[1];
		ctx.c1 = meta.buf2[0];
		
		ctx.a2 = meta.buf2[1];
		ctx.b2 = meta.buf3[0];
		ctx.c2 = meta.buf3[1];
		
		ctx.a3 = meta.buf4[0];
		ctx.b3 = meta.buf4[1];
		ctx.c3 = meta.buf5[0];
		
		ctx.a4 = meta.buf5[1];
		ctx.b4 = meta.buf6[0];
		ctx.c4 = meta.buf6[1];;
	} else  { // planar
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
}

template <Pack pack, Colorspace cs> inline void pack_and_store (const Context &ctx, uint8_t *dst_a, uint8_t *dst_b, uint8_t *dst_c, const size_t stride[3])
{
	const bool interleaved = (pack == Interleaved);
	if (interleaved) {
		dst_a[0 + 0*3] = ctx.a1;
		dst_a[1 + 0*3] = ctx.b1;
		dst_a[2 + 0*3] = ctx.c1;
		
		dst_a[0 + 1*3] = ctx.a2;
		dst_a[1 + 1*3] = ctx.b2;
		dst_a[2 + 1*3] = ctx.c2;
		
		dst_a[0 + 0*3 + stride[0]] = ctx.a3;
		dst_a[1 + 0*3 + stride[0]] = ctx.b3;
		dst_a[2 + 0*3 + stride[0]] = ctx.c3;
		
		dst_a[0 + 1*3 + stride[0]] = ctx.a4;
		dst_a[1 + 1*3 + stride[0]] = ctx.b4;
		dst_a[2 + 1*3 + stride[0]] = ctx.c4;
	} else { // planar
		dst_a[0] = ctx.a1;
		dst_b[0] = ctx.b1;
		dst_c[0] = ctx.c1;
		
		dst_a[1] = ctx.a2;
		dst_b[1] = ctx.b2;
		dst_c[1] = ctx.c2;
		
		dst_a[0 + stride[0]] = ctx.a3;
		dst_b[0 + stride[1]] = ctx.b3;
		dst_c[0 + stride[2]] = ctx.c3;
		
		dst_a[1 + stride[0]] = ctx.a4;
		dst_b[1 + stride[1]] = ctx.b4;
		dst_c[1 + stride[2]] = ctx.c4;
	}
}
	
template <class T > inline T clip(T val_a, T min, T max)
{
    return std::max(std::min(val_a, max), min);
}
template <class T> inline T round_shift(T val, const size_t n)
{
    //return (val + (1 << (n - 1))) >> n;
    return val  >> n; 
}
	
template <Colorspace cs> inline void offset_yuv (int32_t &y, int32_t &u, int32_t &v, int32_t offset_y, int32_t offset_u, int32_t offset_v)
{
	if (cs == YUV444) {
		y += offset_y;
		u += offset_u;
		v += offset_v;
	}
}
	
template <Colorspace cs> static inline void clip_result (int32_t &val_a, int32_t &val_b, int32_t &val_c)
{
    if(cs == RGB){
        val_a = clip(val_a, 235, 16);
        val_b = clip(val_b, 235, 16);
        val_c = clip(val_c, 235, 16);
    }
    else
    if(cs == YUV444){
        val_a = clip(val_a, 235, 16);
        val_b = clip(val_b, 240, 16);
        val_c = clip(val_c, 240, 16);
    }
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
inline void complex_shift( int32_t& a, int32_t b, int32_t c, int n){
    a = round_shift(a, n);
	b = round_shift(b, n);
	c = round_shift(c, n);
}
template <Colorspace from, Colorspace to> inline void transform (Context &ctx, const int16_t transform_matrix[3 * 3])
{
	//offset_yuv <from> (ctx.a1, ctx.b1, ctx.c1, 16, 128, 128); // Y offset is n't necessary in reference
    offset_yuv <from> (ctx.a1, ctx.b1, ctx.c1, 0, -128, -128);
	offset_yuv <from> (ctx.a2, ctx.b2, ctx.c2, 0, -128, -128);
	offset_yuv <from> (ctx.a3, ctx.b3, ctx.c3, 0, -128, -128);
	offset_yuv <from> (ctx.a4, ctx.b4, ctx.c4, 0, -128, -128);

	mat_mul(ctx.a1, ctx.b1, ctx.c1, transform_matrix);
	mat_mul(ctx.a2, ctx.b2, ctx.c2, transform_matrix);
	mat_mul(ctx.a3, ctx.b3, ctx.c3, transform_matrix);
	mat_mul(ctx.a4, ctx.b4, ctx.c4, transform_matrix);
	
	
#ifdef ENABLE_LOG
	std::cout << src_a << " " << src_b << " " << src_c << std::endl;
	std::cout << dst_a << " " << dst_b << " " << dst_c << std::endl;
	std::cout << "******" << std::endl;
#endif
	
	//offset_yuv <to> (ctx.a1, ctx.b1, ctx.c1, 16, 128, 128); // Y offset is n't necessary in reference
	offset_yuv <to> (ctx.a1, ctx.b1, ctx.c1, 0, 128 << 8, 128 << 8);
    offset_yuv <to> (ctx.a2, ctx.b2, ctx.c2, 0, 128 << 8, 128 << 8);
	offset_yuv <to> (ctx.a3, ctx.b3, ctx.c3, 0, 128 << 8, 128 << 8);
	offset_yuv <to> (ctx.a4, ctx.b4, ctx.c4, 0, 128 << 8, 128 << 8);
	
    
    clip_result <to> (ctx.a1, ctx.b1, ctx.c1);
    clip_result <to> (ctx.a2, ctx.b2, ctx.c2);
    clip_result <to> (ctx.a3, ctx.b3, ctx.c3);
    clip_result <to> (ctx.a4, ctx.b4, ctx.c4);
    
    complex_shift(ctx.a1, ctx.b1, ctx.c1, 8);
    complex_shift(ctx.a2, ctx.b2, ctx.c2, 8);
    complex_shift(ctx.a3, ctx.b3, ctx.c3, 8);
    complex_shift(ctx.a4, ctx.b4, ctx.c4, 8);
 
}
	
template <Colorspace cs , Pack pack> inline void next_row (uint8_t* &ptr_a, uint8_t* &ptr_b, uint8_t* &ptr_c, const size_t stride[3])
{
	bool interleaved = (pack == Interleaved);
	if (interleaved) {
		ptr_a += stride[0] * 2;
	} else {
		ptr_a += stride[0] * 2;
		ptr_b += stride[1] * 2;
		ptr_c += stride[2] * 2;
	}
}

template<Colorspace from_cs , Pack from_pack, Colorspace to_cs, Pack to_pack, Standard st> void colorspace_convert(const ConvertMeta& meta)
{
	const int16_t *transform_matrix = get_transfrom_coeffs<from_cs, to_cs, st> ();
	
	uint8_t* src_a = meta.src_data[0];
	uint8_t* src_b = meta.src_data[1];
	uint8_t* src_c = meta.src_data[2];
	
	uint8_t* dst_a = meta.dst_data[0];
	uint8_t* dst_b = meta.dst_data[1];
	uint8_t* dst_c = meta.dst_data[2];
	
	const size_t *src_steps = get_pel_step<from_pack, from_cs> ();
	const size_t *dst_steps = get_pel_step<to_pack, to_cs> ();
	
	for(size_t y = 0; y < meta.height; y += 2) {
		for(size_t x = 0; x < meta.width; x += 2) {
			// Process 2x2 pixels
			Context context;
			load <from_pack> (meta,
                              src_a + src_steps[0] * x,
                              src_b + src_steps[1] * x,
                              src_c + src_steps[2] * x,
                              meta.src_stride );
            unpack <from_pack>(meta, context);
			
			if (from_cs != to_cs) {
				transform <from_cs, to_cs> (context, transform_matrix);
			}
			
			pack_and_store <to_pack, to_cs> (context,
											 dst_a + dst_steps[0] * x,
											 dst_b + dst_steps[1] * x,
											 dst_c + dst_steps[2] * x,
											 meta.dst_stride);
		}
		next_row <from_cs, from_pack> (src_a, src_b, src_c, meta.src_stride);
		next_row <to_cs, to_pack> (dst_a, dst_b, dst_c, meta.dst_stride);
	}
}
} // namespace ColorspaceConverter;

#endif
