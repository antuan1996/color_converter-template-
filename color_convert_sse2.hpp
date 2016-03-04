#ifndef COLOR_CONVERT_SSE2
#define COLOR_CONVERT_SSE2


#include "common.hpp"
#include <emmintrin.h>

#define IS_MULTISTEP(cs) ( cs == V210 )


namespace ColorspaceConverter_SSE2 {

typedef struct{
    __m128i a;
    __m128i b;
    __m128i c;
}VectorPixel;

typedef struct{
    __m128i row1;
    __m128i row2;
    __m128i row3;
    __m128i subrow1;
    __m128i subrow2;
    __m128i subrow3;
}Matrix;

/*
 * p1 p2
 * p3 p4
 */

struct ConvertSpecific
{
    __m128i voffset_y_from;
    __m128i voffset_u_from;
    __m128i voffset_v_from;

    __m128i voffset_y_to;
    __m128i voffset_u_to;
    __m128i voffset_v_to;
    __m128i stack[ 3 * 6];
    uint8_t sp = 0;
    Matrix t_matrix;
 };


struct Context
{
    VectorPixel data1;
    VectorPixel reserve;
};

TARGET_INLINE void push_stack( register VectorPixel& pix, ConvertSpecific& spec )
{
    spec.stack[ spec.sp * 3 + 0 ] =  pix.a;
    spec.stack[ spec.sp * 3 + 1 ] =  pix.b;
    spec.stack[ spec.sp * 3 + 2 ] =  pix.c;
    ++spec.sp;
}
TARGET_INLINE void pop_stack( register VectorPixel& pix, ConvertSpecific& spec)
{
    pix.c = spec.stack[ spec.sp * 3 - 1 ];
    pix.b = spec.stack[ spec.sp * 3 - 2 ];
    pix.a = spec.stack[ spec.sp * 3 - 3 ];
    --spec.sp;
}
template<Colorspace mainc, Colorspace otherc, int pix_x, int pix_y> inline void vget_pos(size_t& posa, size_t& posb, size_t& posc, int cur_pos)
{
    if( mainc != V210 && otherc != V210 && pix_x > 0 )
        return;
    if( !IS_YUV420( mainc ) && !IS_YUV420( otherc) && pix_y > 0 )
        return;
    cur_pos += pix_x * 8;
    get_pos< mainc  >( posa, posb, posc, cur_pos );
}

static inline uint32_t pack10_in_int(int32_t a, int32_t b, int32_t c)
{
    // xx aaaaaaaaaa bbbbbbbbbb cccccccccc
    return ((a & 0X3FF) << 20) | ((b & 0X3FF) << 10) | (c & 0X3FF);
}
template <Colorspace from, Colorspace to> TARGET_INLINE void scale( register __m128i& val_a, register __m128i& val_b, register __m128i& val_c)
{
    if((IS_8BIT(from) &&(IS_8BIT(to))) || (IS_10BIT(from) && IS_10BIT(to)) )
        return;
    if(IS_8BIT(from) && IS_10BIT(to))
    {
        val_a = _mm_slli_epi16( val_a, 2);
        val_b = _mm_slli_epi16( val_b, 2);
        val_c = _mm_slli_epi16( val_c, 2);
        return;
    }
    if(IS_10BIT(from) && IS_8BIT(to))
    {
        val_a = _mm_srai_epi16( val_a, 2);
        val_b = _mm_srai_epi16( val_b, 2);
        val_c = _mm_srai_epi16( val_c, 2);
        return;
    }
    assert(0);
    //TODO unsupported formats
}
template <Colorspace mainc, Colorspace otherc> TARGET_INLINE void init_offset_yuv( ConvertSpecific& spec)
{

    // y+= 16 - result of range manipulation
    __m128i offset_y = _mm_set1_epi16( 16 );
    __m128i offset_u = _mm_set1_epi16( 128 );
    __m128i offset_v = _mm_set1_epi16( 128 );

    if(IS_YUV(mainc))
        scale < YUV444, mainc> ( offset_y, offset_u, offset_v);

    spec.voffset_y_from = offset_y;
    spec.voffset_u_from = offset_u;
    spec.voffset_v_from = offset_v;

    /*
    _mm_storeu_si128( (__m128i* )spec.voffset_y_from, offset_y);
    _mm_storeu_si128( (__m128i* )spec.voffset_u_from, offset_u);
    _mm_storeu_si128( (__m128i* )spec.voffset_v_from, offset_v);
    */

    offset_y = _mm_set1_epi16( 16 );
    offset_u = _mm_set1_epi16( 128 );
    offset_v = _mm_set1_epi16( 128 );

    if(IS_YUV(mainc))
        scale < YUV444, otherc> ( offset_y, offset_u, offset_v);
    spec.voffset_y_to = offset_y;
    spec.voffset_u_to  = offset_u;
    spec.voffset_v_to = offset_v;

    /*
    _mm_storeu_si128( (__m128i* )spec.voffset_y_to, offset_y);
    _mm_storeu_si128( (__m128i* )spec.voffset_u_to, offset_u);
    _mm_storeu_si128( (__m128i* )spec.voffset_v_to, offset_v);
    */
}

template <Colorspace cs_from, Colorspace cs_to> TARGET_INLINE void convert_range(int32_t* matrix)
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
// TO DO new transform matrix
template <Colorspace from, Colorspace to, Standard st> void set_transform_coeffs(ConvertSpecific& spec)
{
   const int32_t* matrix = e_matrix;
   int32_t res_matrix[ 9 ];
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
    convert_range < from, to >( res_matrix );
    spec.t_matrix.row1 = _mm_set_epi16(res_matrix[ 0 ],res_matrix[ 1 ],res_matrix[ 0 ],res_matrix[ 1 ],res_matrix[ 0 ],res_matrix[ 1 ],res_matrix[ 0 ],res_matrix[ 1 ]);

    spec.t_matrix.row2 = _mm_set_epi16(res_matrix[ 3 ],res_matrix[ 4 ],res_matrix[ 3 ],res_matrix[ 4 ],res_matrix[ 3 ],res_matrix[ 4 ],res_matrix[ 3 ],res_matrix[ 4 ]);

    spec.t_matrix.row3 = _mm_set_epi16(res_matrix[ 6 ],res_matrix[ 7 ],res_matrix[ 6 ],res_matrix[ 7 ],res_matrix[ 6 ],res_matrix[ 7 ],res_matrix[ 6 ],res_matrix[ 7 ]);

    spec.t_matrix.subrow1 = _mm_set_epi16(0, res_matrix[ 2 ], 0, res_matrix[ 2 ], 0, res_matrix[ 2 ], 0, res_matrix[ 2 ]);

    spec.t_matrix.subrow2 = _mm_set_epi16(0, res_matrix[ 5 ], 0, res_matrix[ 5 ], 0, res_matrix[ 5 ], 0, res_matrix[ 5 ]);

    spec.t_matrix.subrow3 = _mm_set_epi16(0, res_matrix[ 8 ], 0, res_matrix[ 8 ], 0, res_matrix[ 8 ], 0, res_matrix[ 8 ]);
}
template < Colorspace mainc, Colorspace otherc, int pix_x = 0, int pix_y = 0 > TARGET_INLINE void load(ConvertMeta& meta, register Context& ctx, const uint8_t *srca, const uint8_t *srcb, const uint8_t *srcc)
{
    if( mainc != V210 && otherc != V210 && pix_x > 0 )
        return;
    if( !IS_YUV420( mainc ) && !IS_YUV420( otherc ) && pix_y > 0 )
        return;

    const uint8_t* src_na = srca + meta.src_stride[0];
    const uint8_t* src_nb = srcb + meta.src_stride[1];
    const uint8_t* src_nc = srcc + meta.src_stride[2];
    if( pix_y == 1 )
    {
        srca += meta.src_stride[0];
        srcb += meta.src_stride[0];
        srcc += meta.src_stride[0];

        /*
         * replace in convert
        _mm_storeu_si128( (__m128i* )(ctx.spec.stack), ctx.data1.a);
        _mm_storeu_si128( (__m128i* )(ctx.spec.stack + 16), ctx.data1.b);
        _mm_storeu_si128( (__m128i* )(ctx.spec.stack + 32), ctx.data1.c);
        */
    }
    if(mainc == YUV444)
    {
        ctx.reserve.a = _mm_loadl_epi64( ( __m128i* )( srca ) );
        ctx.reserve.b = _mm_loadl_epi64( ( __m128i* )( srcb ) );
        ctx.reserve.c = _mm_loadl_epi64( ( __m128i* )( srcc ) );
    }
    if(mainc == YUYV || mainc == YVYU )
    {
        ctx.reserve.a = _mm_loadu_si128( ( __m128i* )( srca ) );
        //ctx.data1.b = _mm_loadu_si128( ( __m128i* )( src_na ));
    }
    if( mainc == A2R10G10B10 || mainc == A2B10G10R10 || mainc == Y210 || mainc == RGB32)
    {
        ctx.reserve.a = _mm_loadu_si128( ( __m128i* )( srca ) );
        ctx.reserve.b = _mm_loadu_si128( ( __m128i* )( srca + 16 ));
    }
    if( mainc == NV12 )
    {
        ctx.reserve.a = _mm_loadl_epi64( ( __m128i* )( srca ) );
        //ctx.data1.c = _mm_loadl_epi64( ( __m128i* )( src_na ) );
        if( pix_y == 0 )
            ctx.reserve.b = _mm_loadl_epi64( ( __m128i* )( srcb ) );
        else
        {
            ctx.data1.b = ctx.reserve.b;
            ctx.data1.c = ctx.reserve.c;
        }
    }
    if ( mainc == RGB24 )
    {
        ctx.data1.a = _mm_loadu_si128( ( __m128i* )( srca ) );
        ctx.data1.b = _mm_loadu_si128( ( __m128i* )( srca + 16 ));
        ctx.data1.c = _mm_loadu_si128( ( __m128i* )( srca + 32));
    }
}

static TARGET_INLINE void unpack_YUV422_8bit( VectorPixel& data)
{
    data.b =  _mm_or_si128( data.b, _mm_slli_epi32( data.b, 16 ) ); // ...  0 U 0 U
    data.c = _mm_or_si128( data.c, _mm_slli_epi32( data.c, 16 ) ); // ... 0 V 0 V
}
static TARGET_INLINE void unpack_YUYV(register Context& ctx) {
    // data = v y u y v y u y v y u y v y u y
    ctx.reserve.c = _mm_set1_epi16( 255 ); // store of mask
    ctx.data1.a = _mm_and_si128( ctx.reserve.c, ctx.reserve.a);

    ctx.reserve.c = _mm_srli_epi32( ctx.reserve.c, 8 );
    // u =  ..... 00 00 FF 00, 00 00 FF 00,
    ctx.data1.b = _mm_and_si128( ctx.reserve.c, ctx.reserve.a);

    ctx.reserve.c = _mm_slli_epi32( ctx.reserve.c, 16 );
     // v =  ..... FF 00 00 00, FF 00 00 00,

    ctx.data1.c = _mm_and_si128( ctx.reserve.c, ctx.reserve.a);

    ctx.data1.b = _mm_srli_epi32( ctx.data1.b, 8 ); // ... 0 0 0 U
    ctx.data1.c = _mm_srli_epi32( ctx.data1.c, 24 ); // ... 0 0 0 V

    unpack_YUV422_8bit( ctx.data1 );
}
static TARGET_INLINE void unpack_RGB24(__m128i& vec, VectorPixel& data) {
    // vec = 0 0 0 0 b g r b g r b g r b g r
    data.a = _mm_unpacklo_epi8(  vec, _mm_bsrli_si128( vec, 6 )); // 1 and 3
    data.b = _mm_unpacklo_epi8( _mm_bsrli_si128( vec, 3 ), _mm_bsrli_si128( vec, 9 )); // 2 and 4
    vec = _mm_unpacklo_epi8( data.a, data.b );
    // vec = X X X X b b b b g g g g r r r r

    data.c = _mm_set_epi32( 0, 0, 0, -1); // store of mask

    data.a = _mm_and_si128( data.c, vec );
    data.b = _mm_and_si128( _mm_bsrli_si128( vec, 4 ), data.c );
    data.c = _mm_and_si128( _mm_bsrli_si128( vec, 8 ), data.c );

    data.a = _mm_unpacklo_epi8( data.a, _mm_setzero_si128() );
    data.b = _mm_unpacklo_epi8( data.b, _mm_setzero_si128() );
    data.c = _mm_unpacklo_epi8( data.c, _mm_setzero_si128() );

    data.a = _mm_unpacklo_epi16( data.a, _mm_setzero_si128() );
    data.b = _mm_unpacklo_epi16( data.b, _mm_setzero_si128() );
    data.c = _mm_unpacklo_epi16( data.c, _mm_setzero_si128() );
}

static TARGET_INLINE void unpack_YVYU( register Context& ctx) {
    // vec = ... u y v y u y v y
    ctx.reserve.c = _mm_set1_epi16( 255 ); // store of mask
    ctx.data1.a = _mm_and_si128( ctx.reserve.c, ctx.reserve.a);

    ctx.reserve.c = _mm_srli_epi32( ctx.reserve.c, 8 );
    // u =  ..... 00 00 FF 00, 00 00 FF 00,
    ctx.data1.c = _mm_and_si128( ctx.reserve.c, ctx.reserve.a);

    ctx.reserve.c = _mm_slli_epi32( ctx.reserve.c, 16 );
     // v =  ..... FF 00 00 00, FF 00 00 00,
    ctx.data1.b = _mm_and_si128( ctx.reserve.c, ctx.reserve.a);

    ctx.data1.b = _mm_srli_epi32( ctx.data1.c, 24 ); // ... 0 0 0 V
    ctx.data1.c = _mm_srli_epi32( ctx.data1.b, 8 ); // ... 0 0 0 U

    unpack_YUV422_8bit( ctx.data1 );
}
static TARGET_INLINE void unpack_Y210( __m128i& vec, VectorPixel& data) {

    // vec = v1 v1 y3 y3    u1 u1 y2 y2    v0 v0 y1 y1    u0 u0 y0 y0
    data.c = _mm_set1_epi32( 0xFFFF ); // store of mask
    data.a = _mm_and_si128( data.c, vec );

    // u =  .....  00 00 FF 00, 00 00 FF 00,
    data.b = _mm_srli_epi64( data.c, 16 );
    data.b = _mm_and_si128( data.b, vec );

    // v =  ..... FF 00 00 00, FF 00 00 00,
    data.c = _mm_slli_epi64( data.c, 48 );
    data.c = _mm_and_si128( data.c, vec );

    data.b = _mm_srli_epi64( data.b, 16 ); // ... 0 0 0 0    0 0 U U
    data.c = _mm_srli_epi64( data.c, 48 ); // ... 0 0 0 0    0 0 V V

    data.b =  _mm_or_si128( data.b, _mm_slli_epi64( data.b, 32 ) ); // ...  0 0 U U 0 0 U U
    data.c =  _mm_or_si128( data.c, _mm_slli_epi64( data.c, 32 ) ); // ...  0 0 V V 0 0 V V

    //  [Y] = 0 0 0 Y 0 0 0 Y 0 0 0 Y 0 0 0 Y
    //  [U] = 0 0 0 U 0 0 0 U 0 0 0 U 0 0 0 U
    //  [V] = 0 0 0 V 0 0 0 V 0 0 0 V 0 0 0 V

}

static TARGET_INLINE VectorPixel unpack_A2R10G10B10( __m128i& vec1, __m128i& vec2, VectorPixel& data)
{
    data.a = _mm_set1_epi32( 0x3FF );

    data.c = _mm_packs_epi32( _mm_and_si128( data.a, vec1 ), _mm_and_si128( data.a, vec2 ));
    vec1 = _mm_srai_epi32( vec1, 10 );
    vec2 = _mm_srai_epi32( vec2, 10 );

    data.b = _mm_packs_epi32( _mm_and_si128( data.a, vec1 ), _mm_and_si128( data.a, vec2 ));
    vec1 = _mm_srai_epi32( vec1, 10 );
    vec2 = _mm_srai_epi32( vec2, 10 );

    data.a = _mm_packs_epi32( _mm_and_si128( data.a, vec1 ), _mm_and_si128( data.a, vec2 ));
}

static TARGET_INLINE VectorPixel unpack_A2B10G10R10( __m128i& vec1 , VectorPixel& data)
{
    data.c = _mm_set1_epi32( 0x3FF );
    data.a = _mm_and_si128( data.c, vec1 );
    data.b = _mm_and_si128( _mm_srli_epi32( vec1, 10 ), data.c );
    data.a = _mm_and_si128( _mm_srli_epi32( vec1, 20 ), data.c );
}
static TARGET_INLINE void unpack_NV12( register Context& ctx )
{
    //in register
    // y y y y
    // v u v u
    ctx.data1.a = _mm_unpacklo_epi8( ctx.reserve.a, _mm_setzero_si128() );
    ctx.reserve.b = _mm_unpacklo_epi8( ctx.reserve.b, _mm_setzero_si128() );
    // ... 0 v 0 u 0 v 0 u
    ctx.reserve.c = _mm_set1_epi32( 0xFF );

    ctx.data1.b = _mm_and_si128( ctx.reserve.b, ctx.reserve.c);
    ctx.reserve.c = _mm_srai_epi32( ctx.reserve.c, 16 );

    ctx.data1.c = _mm_and_si128( ctx.reserve.b, ctx.reserve.c);
    ctx.data1.c = _mm_srai_epi32( ctx.data1.c, 16 );

    unpack_YUV422_8bit( ctx.data1 );
}

static TARGET_INLINE void separate_RGB24( VectorPixel& data, __m128i& last)
{
    //  4 - 12
    //  8 -  8
    // 12 -  4
    last = _mm_bsrli_si128( data.c, 4);
    __m128i prev_a = data.a;
    __m128i prev_b = data.b;
    data.a = _mm_bsrli_si128( _mm_bslli_si128( data.a, 4), 4);
    __m128i buf = _mm_bsrli_si128( _mm_bslli_si128( data.b, 8), 4);
    data.b = _mm_or_si128( buf ,_mm_bsrli_si128( prev_a, 12 ));
    buf = _mm_bsrli_si128( _mm_bslli_si128( data.c, 12), 4);
    data.c = _mm_or_si128( buf, _mm_bsrli_si128( prev_b, 8 ));
}
/*
template < Colorspace mainc, Colorspace otherc, int pix_x = 0, int pix_y = 0> TARGET_INLINE void unpack ( register Context& ctx, register Context& ctx2 )
{
    if( mainc != V210 && otherc != V210 && pix_x > 0 )
        return;
    if( !IS_YUV420( mainc ) && !IS_YUV420( otherc ) && pix_y > 0 )
        return;

    if( mainc == NV12)
    {
        // |-------->
        // |      ctx
        // |
        // \/data
        // prepare
        ctx.data1.a = _mm_unpacklo_epi8( ctx.data1.a, _mm_setzero_si128() );
        ctx.reserve.a = _mm_unpacklo_epi8( ctx.reserve.a, _mm_setzero_si128() );
        ctx.data1.b = _mm_unpacklo_epi8( ctx.data1.b, _mm_setzero_si128() );
        ctx.reserve.b = ctx.data1.b;

        ctx2.data1.a = _mm_unpackhi_epi16( ctx.data1.a, _mm_setzero_si128());
        //ctx2.reserve.a = _mm_unpackhi_epi16( ctx.reserve.a, _mm_setzero_si128());
        ctx2.data1.b = _mm_unpackhi_epi16( ctx.data1.b, _mm_setzero_si128());
        //ctx2.reserve.b = _mm_unpackhi_epi16( ctx.reserve.b, _mm_setzero_si128());

        ctx.data1.a = _mm_unpacklo_epi16( ctx.data1.a, _mm_setzero_si128());
        ctx.reserve.a = _mm_unpacklo_epi16( ctx.reserve.a, _mm_setzero_si128());
        ctx.data1.b = _mm_unpacklo_epi16( ctx.data1.b, _mm_setzero_si128());
        ctx.reserve.b = _mm_unpacklo_epi16( ctx.reserve.b, _mm_setzero_si128());

        ctx.data1 = unpack_NV12( ctx );
        //ctx.reserve = unpack_NV12( ctx );

        ctx2.data1 = unpack_NV12( ctx2 );
        //ctx2.reserve = unpack_NV12( ctx2 );
    }
    if( mainc == RGB24 )
    {
        separate_RGB24( ctx.data1, ctx.reserve.a );
        VectorPixel pixl, pixh;
        unpack_RGB24( ctx.data1.c, pixl );
        unpack_RGB24( ctx.reserve.a, pixh );
        ctx2.data1.a = _mm_packs_epi32( pixl.a, pixh.a );
        ctx2.data1.b = _mm_packs_epi32( pixl.b, pixh.b );
        ctx2.data1.c = _mm_packs_epi32( pixl.c, pixh.c );

        unpack_RGB24( ctx.data1.a, pixl );
        unpack_RGB24( ctx.data1.b, pixh );
        ctx.data1.a = _mm_packs_epi32( pixl.a, pixh.a );
        ctx.data1.b = _mm_packs_epi32( pixl.b, pixh.b );
        ctx.data1.c = _mm_packs_epi32( pixl.c, pixh.c );

    }
}
*/
template < Colorspace mainc, Colorspace otherc, int pix_x = 0, int pix_y = 0> TARGET_INLINE void unpack ( register Context& ctx)
{
    if( mainc != V210 && otherc != V210 && pix_x > 0 )
        return;
    if( !IS_YUV420( mainc ) && !IS_YUV420( otherc ) && pix_y > 0 )
        return;
    if( mainc == YUV444)
    {
        ctx.data1.a = _mm_unpacklo_epi8( ctx.reserve.a, _mm_setzero_si128() );
        ctx.data1.b = _mm_unpacklo_epi8( ctx.reserve.b, _mm_setzero_si128() );
        ctx.data1.c = _mm_unpacklo_epi8( ctx.reserve.c, _mm_setzero_si128() );
    }
    if(mainc == YUYV)
    {
        unpack_YUYV( ctx );
        //unpack_YUYV( ctx.reserve.a, ctx.reserve );
        return;
    }
    if( mainc == A2R10G10B10)
    {
        unpack_A2R10G10B10( ctx.data1.a, ctx.data1.b, ctx.data1 );
        //unpack_A2R10G10B10( ctx.reserve.a, ctx.reserve.b, ctx.reserve);
    }
    if( mainc == A2B10G10R10)
    {
        unpack_A2B10G10R10( ctx.data1.a, ctx.data1);
        //unpack_A2B10G10R10( ctx.reserve.a, ctx.reserve);
    }
    if( mainc == RGB32)
    {
        // X B G R

        ctx.data1.c = _mm_set1_epi32( 0xFF );
        ctx.reserve.a = ctx.data1.a;
        ctx.reserve.b = ctx.data1.b;

        ctx.data1.a = _mm_and_si128( ctx.reserve.a, ctx.data1.c );
        ctx.reserve.c = _mm_and_si128( ctx.reserve.b, ctx.data1.c );
        ctx.data1.a = _mm_packs_epi32(ctx.data1.a, ctx.reserve.c);

        ctx.reserve.a = _mm_srai_epi32( ctx.reserve.a, 8 );
        ctx.reserve.b = _mm_srai_epi32( ctx.reserve.b, 8 );
        ctx.data1.b = _mm_and_si128( ctx.reserve.a, ctx.data1.c ),
        ctx.reserve.c = _mm_and_si128( ctx.reserve.b, ctx.data1.c );
        ctx.data1.b = _mm_packs_epi32( ctx.data1.b, ctx.reserve.c);

        ctx.reserve.a = _mm_srai_epi32( ctx.reserve.a, 8 );
        ctx.reserve.b = _mm_srai_epi32( ctx.reserve.b, 8 );
        ctx.reserve.c = _mm_and_si128( ctx.reserve.a, ctx.data1.c );
        ctx.data1.c = _mm_and_si128( ctx.reserve.b, ctx.data1.c );
        ctx.data1.c = _mm_packs_epi32( ctx.reserve.c, ctx.data1.c);

    }

    if( mainc == Y210 )
    {
        unpack_Y210( ctx.data1.a, ctx.data1 );
        //unpack_Y210( ctx.reserve.a, ctx.reserve );
    }
    if( mainc == NV12 )
    {
        //in register
        // y y y y
        // v u v u
        ctx.data1.a = _mm_unpacklo_epi8( ctx.reserve.a, _mm_setzero_si128() );

        if( pix_y == 0)
        {
            ctx.reserve.b = _mm_unpacklo_epi8( ctx.reserve.b, _mm_setzero_si128() );
            // ... 0 v 0 u 0 v 0 u
            ctx.reserve.c = _mm_set1_epi32( 0xFF );

            ctx.data1.b = _mm_and_si128( ctx.reserve.b, ctx.reserve.c);
            ctx.reserve.c = _mm_slli_epi32( ctx.reserve.c, 16 );

            ctx.data1.c = _mm_and_si128( ctx.reserve.b, ctx.reserve.c);
            ctx.data1.c = _mm_srai_epi32( ctx.data1.c, 16 );

            unpack_YUV422_8bit( ctx.data1 );
        }
        else
        {
        }
    }
    if( mainc == NV21 )
    {
        //in register
        // y y y y
        // u v u v
        ctx.data1.a = _mm_unpacklo_epi8( ctx.reserve.a, _mm_setzero_si128() );

        if( pix_y == 0)
        {
            ctx.reserve.b = _mm_unpacklo_epi8( ctx.reserve.b, _mm_setzero_si128() );
            // ... 0 u 0 v 0 u 0 v
            ctx.reserve.c = _mm_set1_epi32( 0xFF );

            ctx.data1.c = _mm_and_si128( ctx.reserve.b, ctx.reserve.c);
            ctx.reserve.c = _mm_srai_epi32( ctx.reserve.c, 16 );

            ctx.data1.b = _mm_and_si128( ctx.reserve.b, ctx.reserve.c);
            ctx.data1.b = _mm_srai_epi32( ctx.data1.c, 16 );

            unpack_YUV422_8bit( ctx.data1 );
        }
    }


}

static TARGET_INLINE __m128i pack_RGB24(VectorPixel rgb_data)
{
    // in memory    R G B
    // in register  B G R
    __m128i mask = _mm_set_epi8(0, 0, 0, 0,   0, 0, 0xFF, 0,    0, 0xFF, 0, 0,   0xFF, 0, 0, 0xFF);

    rgb_data.a = _mm_or_si128( rgb_data.a, _mm_bsrli_si128( rgb_data.a, 1));
    rgb_data.a = _mm_or_si128( rgb_data.a, _mm_bsrli_si128( rgb_data.a, 1));
    rgb_data.a = _mm_or_si128( rgb_data.a, _mm_bsrli_si128( rgb_data.a, 1));
    rgb_data.a = _mm_and_si128( rgb_data.a, mask );

    rgb_data.b = _mm_or_si128( rgb_data.b, _mm_bsrli_si128( rgb_data.b, 1));
    rgb_data.b = _mm_or_si128( rgb_data.b, _mm_bsrli_si128( rgb_data.b, 1));
    rgb_data.b = _mm_or_si128( rgb_data.b, _mm_bsrli_si128( rgb_data.b, 1));
    rgb_data.b = _mm_and_si128( rgb_data.b, mask );

    rgb_data.c = _mm_or_si128( rgb_data.c, _mm_bsrli_si128( rgb_data.c, 1));
    rgb_data.c = _mm_or_si128( rgb_data.c, _mm_bsrli_si128( rgb_data.c, 1));
    rgb_data.c = _mm_or_si128( rgb_data.c, _mm_bsrli_si128( rgb_data.c, 1));
    rgb_data.c = _mm_and_si128( rgb_data.c, mask );

    //BLUE
    __m128i vec = rgb_data.c;

    //GREEN
    vec = _mm_or_si128( vec, _mm_bslli_si128( rgb_data.b, 1));

    //RED
    vec = _mm_or_si128( vec, _mm_bslli_si128( rgb_data.a, 2));
    return vec;
}

static TARGET_INLINE void pack_YUV422(VectorPixel& yuv_data)
{
    yuv_data.b =_mm_avg_epu16( yuv_data.b ,_mm_slli_epi32(yuv_data.b, 16));
    yuv_data.b =_mm_srli_epi32(yuv_data.b, 16);
    //yuv_data.b = _mm_packs_epi32( yuv_data.b ,_mm_setzero_si128());

    yuv_data.c = _mm_avg_epu16( yuv_data.c ,_mm_slli_epi32(yuv_data.c, 16));
    yuv_data.c = _mm_srli_epi32(yuv_data.c, 16);
    //yuv_data.c = _mm_packs_epi32( yuv_data.c ,_mm_setzero_si128());
}
static TARGET_INLINE void pack_YUV420(register Context& ctx)
{
    ctx.data1.b = _mm_avg_epu16( ctx.data1.b, _mm_slli_epi32( ctx.data1.b, 16) );
    ctx.reserve.b = _mm_avg_epu16( ctx.reserve.b, _mm_slli_epi32( ctx.reserve.b, 16) );
    ctx.data1.b = _mm_srli_epi32( ctx.data1.b, 16);
    ctx.reserve.b = _mm_srli_epi32( ctx.reserve.b, 16);
    ctx.data1.b = _mm_avg_epu16( ctx.data1.b, ctx.reserve.b);
    //ctx.data1.b = _mm_packs_epi32( ctx.data1.b, _mm_setzero_si128() );

    ctx.data1.c = _mm_avg_epu16( ctx.data1.c, _mm_slli_epi32( ctx.data1.c, 16) );
    ctx.reserve.c = _mm_avg_epu16( ctx.reserve.c, _mm_slli_epi32( ctx.reserve.c, 16) );
    ctx.data1.c = _mm_srli_epi32( ctx.data1.c, 16);
    ctx.reserve.c = _mm_srli_epi32( ctx.reserve.c, 16);
    ctx.data1.c = _mm_avg_epu16( ctx.data1.c, ctx.reserve.c);
    //ctx.reserve.c = _mm_packs_epi32( ctx.reserve.c, _mm_setzero_si128() );
}

static TARGET_INLINE void pack_YVYU(register Context& ctx)
{
    // in memory    Y V Y U
    // in register  U Y V Y
    pack_YUV422( ctx.data1 );

    ctx.reserve.a = _mm_setzero_si128();

    //Y
    //yuv_data.a = _mm_packs_epi32( yuv_data.a, vec );
    ctx.reserve.a = _mm_or_si128( ctx.data1.a, ctx.reserve.a );

    //U
    // 0 0 0 u 0 0 0 u
    ctx.data1.b = _mm_slli_epi32(ctx.data1.b, 24);
    ctx.reserve.a = _mm_or_si128(ctx.data1.b, ctx.reserve.a );

    //V
    // 0 0 0 v 0 0 0 v
    ctx.data1.c = _mm_slli_epi32(ctx.data1.c, 8);
    ctx.reserve.a = _mm_or_si128( ctx.data1.c, ctx.reserve.a );
}
static TARGET_INLINE void pack_YUYV(register Context& ctx)
{
    // in memory    Y U Y V
    // in register  V Y U Y

     pack_YUV422( ctx.data1 );

    ctx.reserve.a = _mm_setzero_si128();

    //Y
    //yuv_data.a = _mm_packs_epi32( yuv_data.a, vec );
    ctx.reserve.a = _mm_or_si128( ctx.data1.a, ctx.reserve.a );

    //U
    // 0 0 0 u 0 0 0 u
    ctx.data1.b = _mm_slli_epi32(ctx.data1.b, 8);
    ctx.reserve.a = _mm_or_si128(ctx.data1.b, ctx.reserve.a );

    //V
    // 0 0 0 v 0 0 0 v
    ctx.data1.c = _mm_slli_epi32(ctx.data1.c, 24);
    ctx.reserve.a = _mm_or_si128( ctx.data1.c, ctx.reserve.a );
}
static TARGET_INLINE __m128i pack_A2B10G10R10( VectorPixel rgb_data )
{
    __m128i vec = _mm_set1_epi32( 0x3FF );
    rgb_data.a = _mm_and_si128( vec, rgb_data.a );
    rgb_data.b = _mm_and_si128( vec, rgb_data.b );
    rgb_data.c = _mm_and_si128( vec, rgb_data.c );
    vec = _mm_set1_epi32( 3 << 30);
    //RED
    vec = _mm_or_si128( vec, rgb_data.a );

    //GREEN
    rgb_data.b = _mm_slli_epi32( rgb_data.b , 10);
    vec = _mm_or_si128( vec, rgb_data.b );

    //BLUE
    rgb_data.c = _mm_slli_epi32( rgb_data.c,  20);
    vec = _mm_or_si128( vec, rgb_data.c );
    return vec;
}
static TARGET_INLINE __m128i pack_A2R10G10B10( VectorPixel rgb_data)
{
    __m128i vec = _mm_set1_epi32( 0x3FF );

    vec = _mm_set1_epi32( 3 << 30);

    //BLUE
    vec = _mm_or_si128( vec, rgb_data.c );

    //GREEN
    rgb_data.b = _mm_slli_epi32( rgb_data.b , 10);
    vec = _mm_or_si128( vec, rgb_data.b );

    //RED
    rgb_data.a = _mm_slli_epi32( rgb_data.a,  20);
    vec = _mm_or_si128( vec, rgb_data.a );
    return vec;
}
static TARGET_INLINE __m128i pack_NV12( register Context& ctx, ConvertSpecific& spec)
{
    pop_stack( ctx.reserve, spec);
    pack_YUV420( ctx );

    ctx.data1.b = _mm_packs_epi32( ctx.data1.b, ctx.reserve.b);
    ctx.data1.c = _mm_packs_epi32( ctx.data1.c, ctx.reserve.c);

    ctx.data1.b = _mm_or_si128( ctx.data1.b, _mm_slli_epi16( ctx.data1.c, 8 ) );
    ctx.data1.a = _mm_packus_epi16( ctx.data1.a, _mm_setzero_si128());
    ctx.reserve.a = _mm_packus_epi16( ctx.reserve.a, _mm_setzero_si128());

    ctx.reserve.b = ctx.data1.a;
    ctx.reserve.c = ctx.data1.b;
}
static TARGET_INLINE void  pack_NV21( register Context& ctx, ConvertSpecific& spec)
{
    // reg ...uvuvuv
    pop_stack( ctx.reserve, spec );
    pack_YUV420( ctx );

    ctx.data1.b = _mm_packs_epi32( ctx.data1.b, _mm_setzero_si128());
    ctx.data1.c = _mm_packs_epi32( ctx.data1.c, _mm_setzero_si128());

    ctx.data1.b = _mm_or_si128( ctx.data1.c, _mm_slli_epi16( ctx.data1.b, 8 ) );

    ctx.reserve.a = _mm_packus_epi16( ctx.reserve.a, _mm_setzero_si128());
    ctx.data1.a = _mm_packus_epi16( ctx.data1.a, _mm_setzero_si128());

    ctx.reserve.b = ctx.reserve.a;
    ctx.reserve.a = ctx.data1.a;
    ctx.reserve.c = ctx.data1.b;
}

template < Colorspace mainc, Colorspace otherc, int pix_x = 0, int pix_y = 0 > TARGET_INLINE void store(const ConvertMeta& meta, register Context& ctx, ConvertSpecific& spec,  uint8_t* dsta, uint8_t* dstb, uint8_t* dstc)
{

    if( mainc != V210 && otherc != V210 && pix_x > 0 )
        return;
    if( !IS_YUV420( mainc ) && !IS_YUV420( otherc ) && pix_y > 0 )
        return;

    uint8_t* dst_na = dsta + meta.dst_stride_horiz[0];
    uint8_t* dst_nb = dstb + meta.dst_stride_horiz[1];
    uint8_t* dst_nc = dstc + meta.dst_stride_horiz[2];
    if( pix_y == 1)
    {
        //dsta += meta.dst_stride_horiz[ 0 ];
    }
    if( mainc == YUV444 )
    {
        _mm_storel_epi64(( __m128i* )( dsta ), ctx.reserve.a );
        _mm_storel_epi64(( __m128i* )( dstb ), ctx.reserve.b );
        _mm_storel_epi64(( __m128i* )( dstc ), ctx.reserve.c );
    }
    if(mainc == RGB32 || mainc == BGR32 || mainc == A2R10G10B10 || mainc == A2B10G10R10)
    {
        if( pix_y == 1 )
            dsta = dst_na;
        _mm_store_si128( ( __m128i* )( dsta ), ctx.data1.a );
        _mm_store_si128( ( __m128i* )( dsta + 16 ), ctx.data1.b );
    }
    if(mainc == YVYU || mainc == YUYV)
    {
        if( pix_y == 1 )
            dsta = dst_na;

        //ctx.reserve.a = _mm_srli_si128( ctx.reserve.a, 8);
        //ctx.reserve.b = _mm_srli_si128( ctx.reserve.b, 8);
        _mm_store_si128(( __m128i* )( dsta ), ctx.reserve.a );
        //_mm_store_si128(( __m128i* )( dst_na ), ctx.data1.b );
        return;
    }
    if( mainc == NV12 || mainc == NV21)
    {
        if( pix_y == 1)
        {
            _mm_storel_epi64(( __m128i* )( dsta ), ctx.reserve.a );
            _mm_storel_epi64(( __m128i* )( dst_na ), ctx.reserve.b );
            _mm_storel_epi64(( __m128i* )( dstb ), ctx.reserve.c );
            return;
        }
        else
        {
            push_stack( ctx.data1, spec );
        }
    }
    if( mainc == RGB24 )
    {
        _mm_storeu_si128(( __m128i* )( dsta ), ctx.data1.a );
        _mm_storel_epi64(( __m128i* )( dsta + 16 ), ctx.data1.b );

        //_mm_storeu_si128(( __m128i* )( dst_na ), ctx.data1.b );
        //_mm_storel_epi64(( __m128i* )( dst_na + 16 ), ctx.reserve.b );
    }

}
/*
template <Colorspace mainc, Colorspace otherc, int pix_x = 0, int pix_y = 0 > TARGET_INLINE void pack(register Context& ctx, register Context& ctx2)
{
    if( mainc != V210 && otherc != V210 && pix_x > 0 )
        return;
    if( !IS_YUV420( mainc ) && !IS_YUV420( otherc ) && pix_y > 0 )
        return;

    if( mainc == RGB24 )
    {
        ctx.data1.a = pack_RGB24( ctx.data1 );
        ctx.data1.b = pack_RGB24( ctx2.data1 );
        //ctx.reserve.a = pack_RGB24( ctx.reserve );
        //ctx.reserve.b = pack_RGB24( ctx2.reserve );

        __m128i mask = _mm_set_epi32(0, 0, 0 , 0xFFFFFFFF );

        ctx.reserve.a = _mm_and_si128(mask, ctx.data1.b);
        ctx.data1.a = _mm_or_si128( _mm_bslli_si128( ctx.reserve.a, 12 ), ctx.data1.a );
        ctx.data1.b = _mm_bsrli_si128( ctx.data1.b, 4);

        ctx.reserve.b = _mm_and_si128(mask, ctx.reserve.b);
        ctx.reserve.a = _mm_or_si128( _mm_bslli_si128( ctx.reserve.b, 12 ), ctx.reserve.a );
        ctx.reserve.b = _mm_bsrli_si128( ctx.reserve.b, 4);

    }


}
*/

template <Colorspace mainc, Colorspace otherc, int pix_x = 0, int pix_y = 0 > TARGET_INLINE void pack(register Context& ctx, ConvertSpecific& spec)
{
    if( mainc != V210 && otherc != V210 && pix_x > 0 )
        return;
    if( !IS_YUV420( mainc ) && !IS_YUV420( otherc ) && pix_y > 0 )
        return;
    if( ( IS_YUV420( mainc ) || IS_YUV420( otherc )) && pix_y == 0 )
    {

    }
    if( mainc == RGB32 )
    {
        ctx.data1.b = _mm_or_si128(  _mm_slli_si128( ctx.data1.b, 1 ), ctx.data1.a); // G R
        //ctx.data1.c = _mm_slli_si128( ctx.data1.c, 1 ); // X B

        ctx.data1.a = _mm_unpacklo_epi16( ctx.data1.b, ctx.data1.c );
        ctx.data1.b = _mm_unpackhi_epi16( ctx.data1.b, ctx.data1.c );
    }
    if( mainc == BGR32 )
    {
        //ctx.reserve.a = _mm_slli_si128( ctx.data1.b, 1 );
        ctx.data1.b = _mm_or_si128( ctx.reserve.a,  ctx.data1.c); // G B
        ctx.data1.c =  ctx.data1.a; // X R

        ctx.data1.a = _mm_unpacklo_epi16( ctx.data1.b, ctx.data1.c );
        ctx.data1.b = _mm_unpackhi_epi16( ctx.data1.b, ctx.data1.c );
    }
    if(mainc == YVYU)
    {
        pack_YVYU( ctx );
        //ctx.reserve.b = pack_YVYU( ctx.reserve );
    }
    if(mainc == YUYV)
    {
        pack_YUYV( ctx );
        //ctx.reserve.b = pack_YUYV( ctx.reserve );
    }
    if( mainc == YUV444 )
    {
        ctx.reserve.a = _mm_packus_epi16( ctx.data1.a, _mm_setzero_si128() );
        ctx.reserve.b = _mm_packus_epi16( ctx.data1.b, _mm_setzero_si128() );
        ctx.reserve.c = _mm_packus_epi16( ctx.data1.c, _mm_setzero_si128() );

    }
    if( mainc == A2R10G10B10 )
    {
        VectorPixel pix;
        pix.a = _mm_unpacklo_epi16( ctx.data1.a, _mm_setzero_si128() );
        pix.b = _mm_unpacklo_epi16( ctx.data1.b, _mm_setzero_si128() );
        pix.c = _mm_unpacklo_epi16( ctx.data1.c, _mm_setzero_si128() );
        ctx.reserve.a = pack_A2R10G10B10( pix );

        pix.a = _mm_unpackhi_epi16( ctx.data1.a, _mm_setzero_si128() );
        pix.b = _mm_unpackhi_epi16( ctx.data1.b, _mm_setzero_si128() );
        pix.c = _mm_unpackhi_epi16( ctx.data1.c, _mm_setzero_si128() );
        ctx.reserve.b = pack_A2R10G10B10( pix);
    }
    if( mainc == A2B10G10R10 )
    {
        VectorPixel pix;
        pix.a = _mm_unpacklo_epi16( ctx.data1.a, _mm_setzero_si128() );
        pix.b = _mm_unpacklo_epi16( ctx.data1.b, _mm_setzero_si128() );
        pix.c = _mm_unpacklo_epi16( ctx.data1.c, _mm_setzero_si128() );
        ctx.reserve.a = pack_A2B10G10R10( pix );

        pix.a = _mm_unpackhi_epi16( ctx.data1.a, _mm_setzero_si128() );
        pix.b = _mm_unpackhi_epi16( ctx.data1.b, _mm_setzero_si128() );
        pix.c = _mm_unpackhi_epi16( ctx.data1.c, _mm_setzero_si128() );
        ctx.reserve.b = pack_A2B10G10R10( pix);

    }
    if( mainc == NV12 && pix_y == 1 )
    {
        pack_NV12( ctx, spec );
    }

    if( mainc == NV21 && pix_y == 1 )
    {
        pack_NV21( ctx, spec );
    }
}

static TARGET_INLINE __m128i  clip( __m128i data , const int32_t min_val, const int32_t max_val) {
    __m128i check_val = _mm_set1_epi16( max_val );
    __m128i mask = _mm_cmplt_epi16( data, check_val ); // if (reg[i] >= value)
    __m128i res = _mm_and_si128( data, mask );
    mask = _mm_andnot_si128( mask, check_val ); // reg[i] = value;
    res = _mm_or_si128( res , mask );

    check_val = _mm_set1_epi16( min_val );
    mask = _mm_cmpgt_epi16( res, check_val ); // if(reg[i] <= value)
    res = _mm_and_si128( res, mask );
    mask = _mm_andnot_si128( mask, check_val ); // reg[i] = value;
    res = _mm_or_si128( res , mask );

    return res;
}


template <Colorspace mainc> TARGET_INLINE void offset_yuv (VectorPixel& data, bool is_left, ConvertSpecific& spec)
{
    if(IS_YUV( mainc ))
    {

        //offset left
        if(is_left)
        {
            /*
            __m128i offset_y = _mm_loadu_si128( (__m128i*) spec.voffset_y_from );
            __m128i offset_u = _mm_loadu_si128( (__m128i*) spec.voffset_u_from );
            __m128i offset_v = _mm_loadu_si128( (__m128i*) spec.voffset_v_from );
            */
            data.a = _mm_sub_epi16( data.a, spec.voffset_y_from);
            data.b = _mm_sub_epi16( data.b, spec.voffset_u_from );
            data.c = _mm_sub_epi16( data.c, spec.voffset_v_from );
        }
        else
        // offset right
        {/*
            __m128i offset_y = _mm_loadu_si128( (__m128i*) spec.voffset_y_to);
            __m128i offset_u = _mm_loadu_si128( (__m128i*) spec.voffset_u_to);
            __m128i offset_v = _mm_loadu_si128( (__m128i*) spec.voffset_v_to);
        */
            data.a = _mm_add_epi16( data.a, spec.voffset_y_to);
            data.b = _mm_add_epi16( data.b, spec.voffset_u_to);
            data.c = _mm_add_epi16( data.c, spec.voffset_v_to);
        }
    }
}

template <Colorspace mainc> static TARGET_INLINE void clip_point(__m128i& val_a, __m128i& val_b, __m128i& val_c)
{
    //puts("before");
    //std::cout << val_a << " "<< val_b << " " << val_c << std::endl;
    if(IS_8BIT( mainc ) && IS_RGB( mainc ))
    {
        val_a = clip(val_a, 0, 255);
        val_b = clip(val_b, 0, 255);
        val_c = clip(val_c, 0, 255);
    }
    else if( IS_10BIT( mainc ) && IS_RGB( mainc ))
    {
        val_a = clip(val_a, 0, 255 << 2);
        val_b = clip(val_b, 0, 255 << 2);
        val_c = clip(val_c, 0, 255 << 2);
    }
    else if( IS_YUV( mainc ) && IS_8BIT( mainc ) )
    {
        val_a = clip(val_a, 16, 235);
        val_b = clip(val_b, 16, 240);
        val_c = clip(val_c, 16, 240);
        //puts("after");
        //std::cout << val_a << " "<< val_b << " " << val_c << std::endl;
    }
    else if( IS_YUV( mainc ) && IS_10BIT( mainc ) )
    {
        val_a = clip(val_a, 16 << 2, 235 << 2);
        val_b = clip(val_b, 16 << 2, 240 << 2);
        val_c = clip(val_c, 16 << 2, 240 << 2);
        //puts("after");
        //std::cout << val_a << " "<< val_b << " " << val_c << std::endl;
    }


}
/*
 * | a |   |                  |   | a'|
 * | b | * | transform_matrix | = | b'|
 * | c |   |                  |   | c'|
 */
static TARGET_INLINE __m128i multiple_add( const __m128i chan_a , const __m128i chan_b, __m128i chan_c, const __m128i koeffs_ab, const __m128i koeffs_c ) {
    __m128i mul_res = koeffs_ab;
    __m128i data = _mm_slli_epi32( chan_a, 16 );
    data = _mm_or_si128( data, chan_b ); // data = 0 a, 0 b, 0 a, 0 b, 0 a, 0 b, 0 a, 0 b
    mul_res = _mm_madd_epi16( mul_res, data ); // mul = s 0 a*b , s 0 a*b, s 0 a*b, s 0 a*b

    __m128i mul_res_last = _mm_madd_epi16( koeffs_c, chan_c);

    mul_res = _mm_add_epi32( mul_res, mul_res_last ); // (ka * a + kb * b) + (kc * c)
    //mul_res = _mm_srai_epi32( mul_res, 10 ); // shifting
    return mul_res;
}
static TARGET_INLINE void round_shift( __m128i& a, int n)
{
    __m128i sh = _mm_set1_epi32( 1 << (n - 1) );
    a = _mm_add_epi32( a, sh );
    a = _mm_srai_epi32( a, n );
}

static TARGET_INLINE void mat_mul(register Context& ctx, ConvertSpecific& spec)
{
    ctx.reserve.a = _mm_unpackhi_epi16( ctx.data1.a, _mm_setzero_si128() );
    ctx.reserve.b = _mm_unpackhi_epi16( ctx.data1.b, _mm_setzero_si128() );
    ctx.reserve.c = _mm_unpackhi_epi16( ctx.data1.c, _mm_setzero_si128() );;
    push_stack( ctx.reserve, spec );

    ctx.data1.a = _mm_unpacklo_epi16( ctx.data1.a, _mm_setzero_si128() );
    ctx.data1.b = _mm_unpacklo_epi16( ctx.data1.b, _mm_setzero_si128() );
    ctx.data1.c = _mm_unpacklo_epi16( ctx.data1.c, _mm_setzero_si128() );

    ctx.reserve.a = multiple_add( ctx.data1.a, ctx.data1.b, ctx.data1.c, spec.t_matrix.row1, spec.t_matrix.subrow1);
    ctx.reserve.b = multiple_add( ctx.data1.a, ctx.data1.b, ctx.data1.c, spec.t_matrix.row2, spec.t_matrix.subrow2);
    ctx.reserve.c = multiple_add( ctx.data1.a, ctx.data1.b, ctx.data1.c, spec.t_matrix.row3, spec.t_matrix.subrow3);

    pop_stack( ctx.data1, spec );
    push_stack( ctx.reserve, spec );

    ctx.reserve.a = multiple_add( ctx.data1.a, ctx.data1.b, ctx.data1.c, spec.t_matrix.row1, spec.t_matrix.subrow1);
    ctx.reserve.b = multiple_add( ctx.data1.a, ctx.data1.b, ctx.data1.c, spec.t_matrix.row2, spec.t_matrix.subrow2);
    ctx.reserve.c = multiple_add( ctx.data1.a, ctx.data1.b, ctx.data1.c, spec.t_matrix.row3, spec.t_matrix.subrow3);

    pop_stack( ctx.data1, spec );

    round_shift( ctx.data1.a, 8 );
    round_shift( ctx.data1.b, 8 );
    round_shift( ctx.data1.c, 8 );

    round_shift( ctx.reserve.a, 8 );
    round_shift( ctx.reserve.b, 8 );
    round_shift( ctx.reserve.c, 8 );

    ctx.data1.a = _mm_packs_epi32( ctx.data1.a, ctx.reserve.a );
    ctx.data1.b = _mm_packs_epi32( ctx.data1.b, ctx.reserve.b );
    ctx.data1.c = _mm_packs_epi32( ctx.data1.c, ctx.reserve.c );

}


template <Colorspace mainc, Colorspace otherc, int pix_x = 0, int pix_y = 0> TARGET_INLINE void transform (register Context& ctx, ConvertSpecific& spec)
{
    if( mainc != V210 && otherc != V210 && pix_x > 0 )
        return;
    if( !IS_YUV420( mainc ) && !IS_YUV420( otherc ) && pix_y > 0 )
        return;

    scale<mainc, otherc>( ctx.data1.a, ctx.data1.b, ctx.data1.c );
    //scale<from_cs, to_cs>( ctx.reserve.a, ctx.reserve.b, ctx.reserve.c );

    //  matrix multiple
    if( (IS_RGB(mainc) && IS_YUV(otherc)) || (IS_YUV(mainc) && IS_RGB(otherc)))
    {
        offset_yuv <mainc> (ctx.data1, SHIFT_LEFT, spec);
        //offset_yuv <from_cs> (ctx.reserve, SHIFT_LEFT, ctx);

        mat_mul( ctx, spec);
        //mat_mul( ctx.reserve, spec.t_matrix );

        offset_yuv <otherc> (ctx.data1, SHIFT_RIGHT, spec);
        //offset_yuv <to_cs> (ctx.reserve, SHIFT_RIGHT, ctx);

        clip_point <otherc> (ctx.data1.a, ctx.data1.b, ctx.data1.c);
        //clip_point <to_cs> (ctx.reserve.a, ctx.reserve.b, ctx.reserve.c);

    } // if(from_cs_type != to_type)
}
template <Colorspace mainc, Colorspace otherc > TARGET_INLINE void next_row (uint8_t* &ptr_a, uint8_t* &ptr_b, uint8_t* &ptr_c, const size_t stride[3])
{
    register int mn = 1;
    if( IS_YUV420( mainc ) || IS_YUV420( otherc ) )
    {
        mn = 2;
    }
    if (IS_INTERLEAVED( mainc ) ) {
        ptr_a += stride[0] * mn;
    }
    if( IS_PLANAR ( mainc ) )
    {
        ptr_a += stride[0] * mn;
        ptr_b += stride[1] * mn;
        ptr_c += stride[2] * mn;
    }
    if( IS_SEMIPLANAR ( mainc ) ) //YUV420
    {
        ptr_a += stride[0] * 2;
        ptr_b += stride[1];
    }

}
template< Colorspace from_cs, Colorspace to_cs, Standard st, int pix_x = 0, int pix_y = 0 > TARGET_INLINE void convert_tick( uint8_t* src_a, uint8_t* src_b, uint8_t* src_c,
                                                                                                                             uint8_t* dst_a, uint8_t* dst_b, uint8_t* dst_c,
                                                                                                                             ConvertMeta& meta, register Context& ctx, ConvertSpecific& spec,  int x)
{

    static size_t shift_a, shift_b, shift_c;
    vget_pos <from_cs, to_cs, pix_x, pix_y>(shift_a, shift_b, shift_c, x);
    load < from_cs, to_cs, pix_x, pix_y> (meta, ctx,
                            src_a + shift_a,
                            src_b + shift_b,
                            src_c + shift_c);

    unpack <from_cs, to_cs, pix_x, pix_y> ( ctx );


    transform <from_cs, to_cs, pix_x, pix_y > ( ctx, spec );

    pack< to_cs, from_cs, pix_x, pix_y>( ctx, spec );

    vget_pos < to_cs, from_cs, pix_x, pix_y>(shift_a, shift_b, shift_c, x);
    store< to_cs, from_cs, pix_x, pix_y> (meta, ctx, spec,
                         dst_a + shift_a,
                         dst_b + shift_b,
                         dst_c + shift_c);

}
template< Colorspace mainc, Colorspace otherc >  int vert_step()
{
    if( IS_YUV420( mainc ) || IS_YUV420( otherc ) )
    {
        return 2;
    }
    return 1;
}

template< Colorspace from_cs, Colorspace to_cs, Standard st > void colorspace_convert(ConvertMeta& meta)
{

    uint8_t* src_a = meta.src_data[0];
    uint8_t* src_b = meta.src_data[1];
    uint8_t* src_c = meta.src_data[2];

    uint8_t* dst_a = meta.dst_data[0];
    uint8_t* dst_b = meta.dst_data[1];
    uint8_t* dst_c = meta.dst_data[2];

    register Context context1;
    ConvertSpecific spec1;
    if( (IS_RGB( from_cs ) && IS_YUV(to_cs)) || (IS_YUV(from_cs) && IS_RGB(to_cs)) )
    {
        // BOTLLE NECK!!!
        init_offset_yuv < from_cs, to_cs >( spec1 );
        set_transform_coeffs <from_cs, to_cs, st>( spec1 );
    }
    int8_t step_x;
    if( IS_MULTISTEP( from_cs ) || IS_MULTISTEP( to_cs ))
        step_x = 24;
    else
        step_x = 8;
    size_t x,y;
    y = 0;
    do
    {
        for(x = 0; x+step_x <= meta.width; x += step_x)
        {

            convert_tick < from_cs, to_cs, st, 0, 0> ( src_a, src_b, src_c, dst_a, dst_b, dst_c, meta, context1, spec1, x);

            if( from_cs == V210 || to_cs == V210 )
            {
                convert_tick < from_cs, to_cs, st, 1, 0> ( src_a, src_b, src_c, dst_a, dst_b, dst_c, meta, context1, spec1, x);
                convert_tick < from_cs, to_cs, st, 2, 0> ( src_a, src_b, src_c, dst_a, dst_b, dst_c, meta, context1, spec1, x);
            }
            if( IS_YUV420( from_cs) || IS_YUV420( to_cs) )
            {
                convert_tick < from_cs, to_cs, st, 0, 1> ( src_a, src_b, src_c, dst_a, dst_b, dst_c, meta, context1, spec1, x);
                if( from_cs == V210 || to_cs == V210 )
                {
                    convert_tick < from_cs, to_cs, st, 1, 1> ( src_a, src_b, src_c, dst_a, dst_b, dst_c, meta, context1, spec1, x);
                    convert_tick < from_cs, to_cs, st, 2, 1> ( src_a, src_b, src_c, dst_a, dst_b, dst_c, meta, context1, spec1, x);
                }
            }

        }
        y += vert_step< from_cs, to_cs>();
        next_row <from_cs, to_cs > (src_a, src_b, src_c, meta.src_stride );
        next_row <from_cs, to_cs > ( dst_a, dst_b, dst_c, meta.dst_stride_horiz);
	}
    while( y < meta.height );
}

} // namespace ColorspaceConverter_SSE2;

#endif
