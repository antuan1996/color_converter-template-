#ifndef COLOR_CONVERT_SSE2
#define COLOR_CONVERT_SSE2


#include "common.hpp"
#include <emmintrin.h>

#define IS_MULTISTEP(cs) ( cs == NV12 || cs == NV21 || cs == RGB24 )


namespace ColorspaceConverter_SSE2 {

typedef struct{
    __m128i a;
    __m128i b;
    __m128i c;
}VectorPixel;

typedef struct{
    uint8_t row1[ 16 ];
    uint8_t row2[ 16 ];
    uint8_t row3[ 16 ];
    uint8_t subrow1[ 16 ];
    uint8_t subrow2[ 16 ];
    uint8_t subrow3[ 16 ];
}Matrix;

/*
 * p1 p2
 * p3 p4
 */

struct ConvertSpecific
{
    uint8_t voffset_y_from[ 16 ];
    uint8_t voffset_u_from[ 16 ];
    uint8_t voffset_v_from[ 16 ];

    uint8_t voffset_y_to[ 16 ];
    uint8_t voffset_u_to[ 16 ];
    uint8_t voffset_v_to[ 16 ];
    uint8_t stack[ 3 * 16 ];
    Matrix t_matrix;
 };

struct Context
{
    VectorPixel data1;
    VectorPixel reserve;
    ConvertSpecific spec;

};

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
template <Colorspace from_cs, Colorspace to_cs> TARGET_INLINE void init_offset_yuv( Context& ctx )
{

    // y+= 16 - result of range manipulation
    __m128i offset_y = _mm_set1_epi16( 16 );
    __m128i offset_u = _mm_set1_epi16( 128 );
    __m128i offset_v = _mm_set1_epi16( 128 );

    if(IS_YUV(from_cs))
        scale < YUV444, from_cs> ( offset_y, offset_u, offset_v);

    _mm_storeu_si128( (__m128i* )ctx.spec.voffset_y_from, offset_y);
    _mm_storeu_si128( (__m128i* )ctx.spec.voffset_u_from, offset_u);
    _mm_storeu_si128( (__m128i* )ctx.spec.voffset_v_from, offset_v);


    offset_y = _mm_set1_epi16( 16 );
    offset_u = _mm_set1_epi16( 128 );
    offset_v = _mm_set1_epi16( 128 );

    if(IS_YUV(from_cs))
        scale < YUV444, to_cs> ( offset_y, offset_u, offset_v);

    _mm_storeu_si128( (__m128i* )ctx.spec.voffset_y_to, offset_y);
    _mm_storeu_si128( (__m128i* )ctx.spec.voffset_u_to, offset_u);
    _mm_storeu_si128( (__m128i* )ctx.spec.voffset_v_to, offset_v);

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
template <Colorspace from, Colorspace to, Standard st> void set_transform_coeffs(Context& ctx)
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
    __m128i buf = _mm_set_epi16(res_matrix[ 0 ],res_matrix[ 1 ],res_matrix[ 0 ],res_matrix[ 1 ],res_matrix[ 0 ],res_matrix[ 1 ],res_matrix[ 0 ],res_matrix[ 1 ]);
    _mm_store_si128( (__m128i* )ctx.spec.t_matrix.row1, buf);

    buf = _mm_set_epi16(res_matrix[ 3 ],res_matrix[ 4 ],res_matrix[ 3 ],res_matrix[ 4 ],res_matrix[ 3 ],res_matrix[ 4 ],res_matrix[ 3 ],res_matrix[ 4 ]);
    _mm_store_si128( (__m128i* )ctx.spec.t_matrix.row2, buf);

    buf = _mm_set_epi16(res_matrix[ 6 ],res_matrix[ 7 ],res_matrix[ 6 ],res_matrix[ 7 ],res_matrix[ 6 ],res_matrix[ 7 ],res_matrix[ 6 ],res_matrix[ 7 ]);
    _mm_store_si128( (__m128i* )ctx.spec.t_matrix.row3, buf);

    buf = _mm_set_epi16(0, res_matrix[ 2 ], 0, res_matrix[ 2 ], 0, res_matrix[ 2 ], 0, res_matrix[ 2 ]);
    _mm_store_si128( (__m128i* )ctx.spec.t_matrix.subrow1, buf);

    buf = _mm_set_epi16(0, res_matrix[ 5 ], 0, res_matrix[ 5 ], 0, res_matrix[ 5 ], 0, res_matrix[ 5 ]);
    _mm_store_si128( (__m128i* )ctx.spec.t_matrix.subrow2, buf);

    buf = _mm_set_epi16(0, res_matrix[ 8 ], 0, res_matrix[ 8 ], 0, res_matrix[ 8 ], 0, res_matrix[ 8 ]);
    _mm_store_si128( (__m128i* )ctx.spec.t_matrix.subrow3, buf);
}
template < Colorspace cs, int pix_num> TARGET_INLINE void load(ConvertMeta& meta, register Context& ctx, const uint8_t *srca, const uint8_t *srcb, const uint8_t *srcc)
{
    const uint8_t* src_na = srca + meta.src_stride[0];
    const uint8_t* src_nb = srcb + meta.src_stride[1];
    const uint8_t* src_nc = srcc + meta.src_stride[2];
    if( pix_num == 1 )
    {
        srca += meta.src_stride[0];
        /*
         * replace in convert
        _mm_storeu_si128( (__m128i* )(ctx.spec.stack), ctx.data1.a);
        _mm_storeu_si128( (__m128i* )(ctx.spec.stack + 16), ctx.data1.b);
        _mm_storeu_si128( (__m128i* )(ctx.spec.stack + 32), ctx.data1.c);
        */
    }
    if(cs == YUYV || cs == YVYU )
    {
        ctx.data1.a = _mm_loadu_si128( ( __m128i* )( srca ) );
        ctx.data1.b = _mm_loadu_si128( ( __m128i* )( src_na ));
    }
    if( cs == A2R10G10B10 || cs == A2B10G10R10 || cs == Y210 || cs == RGB32)
    {
        ctx.reserve.a = _mm_loadu_si128( ( __m128i* )( srca ) );
        ctx.reserve.b = _mm_loadu_si128( ( __m128i* )( srca + 16 ));
    }
    if( cs == NV12 )
    {
        ctx.data1.a = _mm_loadl_epi64( ( __m128i* )( srca ) );
        ctx.data1.c = _mm_loadl_epi64( ( __m128i* )( src_na ) );
        ctx.data1.b = _mm_loadl_epi64( ( __m128i* )( srcb ) );
    }
    if ( cs == RGB24 )
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

    /*
    data.a = _mm_unpacklo_epi16( data.a, _mm_setzero_si128() ); //  [Y] = 0 0 0 Y 0 0 0 Y 0 0 0 Y 0 0 0 Y
    data.b = _mm_unpacklo_epi16( data.b, _mm_setzero_si128() ); //  [U] = 0 0 0 U 0 0 0 U 0 0 0 U 0 0 0 U
    data.c = _mm_unpacklo_epi16( data.c, _mm_setzero_si128() ); //  [V] = 0 0 0 V 0 0 0 V 0 0 0 V 0 0 0 V
    */
}
static TARGET_INLINE void unpack_YUYV( __m128i& vec, VectorPixel& data) {
    // vec = v y u y v y u y v y u y v y u y
    data.c = _mm_set1_epi16( 255 ); // store of mask
    data.a = _mm_and_si128( data.c, vec );

    data.b = _mm_srli_epi32( data.c, 8 );
    // u =  ..... 00 00 FF 00, 00 00 FF 00,
    data.b = _mm_and_si128( data.b, vec );

    data.c = _mm_slli_epi32( data.c, 24 );
    // v =  ..... FF 00 00 00, FF 00 00 00,

    data.c = _mm_and_si128( data.c, vec );

    vec = _mm_setzero_si128();

    data.b = _mm_srli_epi32( data.b, 8 ); // ... 0 0 0 U
    data.c = _mm_srli_epi32( data.c, 24 ); // ... 0 0 0 V

    unpack_YUV422_8bit( data );
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

static TARGET_INLINE VectorPixel unpack_YVYU( __m128i& vec, VectorPixel& data ) {
    // vec = 0 0 0 0 0 0 0 0 u y v y u y v y
    data.c = _mm_set1_epi16( 255 ); // store of mask
    data.a = _mm_and_si128( data.c, vec );

    // u =  ..... FF 00 00 00, FF 00 00 00,
    data.b = _mm_slli_epi32( data.c, 24 );
    data.b = _mm_and_si128( data.b, vec );

    // v =  ..... 00 00 FF 00, 00 00 FF 00,
    data.c = _mm_srli_epi32( data.c, 8 );
    data.c = _mm_and_si128( data.c, vec );

    data.b = _mm_srli_epi32( data.b, 24 ); // ... 0 0 0 U
    data.c = _mm_srli_epi32( data.c, 8 ); // ... 0 0 0 V

    unpack_YUV422_8bit( data );
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
static TARGET_INLINE VectorPixel unpack_NV12( Context& ctx )
{
    // TODO  FIX IT
    /*
    ctx.reserve.a = _mm_set_epi32( 0, 0xFF, 0 , 0xFF );
    ctx.reserve.b = ctx.data1.b;

    ctx.data1.b = _mm_and_si128( ctx.data1.b, ctx.reserve.a);
    ctx.data1.b = _mm_or_si128( ctx.data1.b, _mm_slli_epi64( ctx.data1.b, 32 ) );

    ctx.reserveata.c = _mm_and_si128( uv, _mm_slli_epi64( y, 32 ) );
    data.c = _mm_or_si128( data.c, _mm_srli_epi64( data.c, 32 ) );
    */
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
template < Colorspace cs> TARGET_INLINE void unpack ( Context& ctx, Context& ctx2 )
{
    if( cs == NV12)
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
    if( cs == RGB24 )
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

template < Colorspace from_cs> TARGET_INLINE void unpack ( register Context& ctx) __attribute__((__vectorcall));
template < Colorspace from_cs> TARGET_INLINE void unpack ( register Context& ctx)
{
    if(from_cs == YUYV)
    {
        unpack_YUYV( ctx.data1.a, ctx.data1 );
        //unpack_YUYV( ctx.reserve.a, ctx.reserve );
        return;
    }
    if( from_cs == A2R10G10B10)
    {
        unpack_A2R10G10B10( ctx.data1.a, ctx.data1.b, ctx.data1 );
        //unpack_A2R10G10B10( ctx.reserve.a, ctx.reserve.b, ctx.reserve);
    }
    if( from_cs == A2B10G10R10)
    {
        unpack_A2B10G10R10( ctx.data1.a, ctx.data1);
        //unpack_A2B10G10R10( ctx.reserve.a, ctx.reserve);
    }
    if( from_cs == RGB32)
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

    if( from_cs == Y210 )
    {
        unpack_Y210( ctx.data1.a, ctx.data1 );
        //unpack_Y210( ctx.reserve.a, ctx.reserve );
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
static TARGET_INLINE void pack_YUV420(VectorPixel& yuv_data1, VectorPixel yuv_reserve)
{
    yuv_data1.b = _mm_avg_epu16( yuv_data1.b, _mm_slli_epi64( yuv_data1.b, 32) );
    yuv_reserve.b = _mm_avg_epu16( yuv_reserve.b, _mm_slli_epi64( yuv_reserve.b, 32) );
    yuv_data1.b = _mm_srli_epi64( yuv_data1.b, 32);
    yuv_reserve.b = _mm_srli_epi64( yuv_reserve.b, 32);
    yuv_data1.b = _mm_avg_epu16( yuv_data1.b, yuv_reserve.b);
    //yuv_data1.b = _mm_packs_epi32( yuv_data1.b, _mm_setzero_si128() );

    yuv_data1.c = _mm_avg_epu16( yuv_data1.c, _mm_slli_epi64( yuv_data1.c, 32) );
    yuv_reserve.c = _mm_avg_epu16( yuv_reserve.c, _mm_slli_epi64( yuv_reserve.c, 32) );
    yuv_data1.c = _mm_srli_epi64( yuv_data1.c, 32);
    yuv_reserve.c = _mm_srli_epi64( yuv_reserve.c, 32);
    yuv_data1.c = _mm_avg_epu16( yuv_data1.c, yuv_reserve.c);
    //yuv_reserve.c = _mm_packs_epi32( yuv_reserve.c, _mm_setzero_si128() );
}

static TARGET_INLINE __m128i pack_YVYU( VectorPixel yuv_data )
{
    // in memory    Y V Y U
    // in register  U Y V Y
    pack_YUV422( yuv_data );

    __m128i vec = _mm_setzero_si128();

    //Y
    //yuv_data.a = _mm_packs_epi32( yuv_data.a, vec );
    vec = _mm_or_si128( vec, yuv_data.a );

    //U
    // 0 0 0 u 0 0 0 u
    yuv_data.b = _mm_slli_epi32(yuv_data.b, 24);
    vec = _mm_or_si128(yuv_data.b, vec );

    //V
    // 0 0 0 v 0 0 0 v
    yuv_data.c = _mm_slli_epi32(yuv_data.c, 8);
    vec = _mm_or_si128( vec, yuv_data.c );
    return vec;
}
static TARGET_INLINE __m128i pack_YUYV(VectorPixel yuv_data)
{
    // in memory    Y U Y V
    // in register  V Y U Y

    pack_YUV422( yuv_data );

    __m128i vec = _mm_setzero_si128();

    //Y
    //yuv_data.a = _mm_packs_epi32( yuv_data.a, vec );
    vec = _mm_or_si128( vec, yuv_data.a );

    //U
    //yuv_data.b = _mm_packs_epi32( yuv_data.b ,_mm_setzero_si128());
    // 0 0 0 u 0 0 0 u
    yuv_data.b = _mm_slli_epi32(yuv_data.b, 8);
    vec = _mm_or_si128(yuv_data.b, vec );

    //V
    //yuv_data.c = _mm_packs_epi32( yuv_data.c ,_mm_setzero_si128());
    // 0 0 0 v 0 0 0 v
    yuv_data.c = _mm_slli_epi32(yuv_data.c, 24);
    vec = _mm_or_si128( vec, yuv_data.c );

    return vec;
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
static TARGET_INLINE __m128i pack_NV12( VectorPixel& yuv_data )
{
    yuv_data.b = _mm_or_si128( yuv_data.b, _mm_slli_epi64( yuv_data.c, 32));
}

template < Colorspace cs, int pix_num > TARGET_INLINE void store(const ConvertMeta& meta, register Context& ctx,  uint8_t* dsta, uint8_t* dstb, uint8_t* dstc)
{
    uint8_t* dst_na = dsta + meta.dst_stride_horiz[0];
    uint8_t* dst_nb = dstb + meta.dst_stride_horiz[1];
    uint8_t* dst_nc = dstc + meta.dst_stride_horiz[2];
    if( pix_num == 1)
    {
        dsta += meta.dst_stride_horiz[ 0 ];
    }
    if(cs == YUV444 || cs == RGB32 || cs == BGR32 || cs == A2R10G10B10 || cs == A2B10G10R10)
    {
        _mm_store_si128( ( __m128i* )( dsta ), ctx.data1.a );
        _mm_store_si128( ( __m128i* )( dsta + 16 ), ctx.data1.b );
    }
    if(cs == YVYU || cs == YUYV)
    {
        //ctx.reserve.a = _mm_srli_si128( ctx.reserve.a, 8);
        //ctx.reserve.b = _mm_srli_si128( ctx.reserve.b, 8);
        _mm_store_si128(( __m128i* )( dsta ), ctx.data1.a );
        _mm_store_si128(( __m128i* )( dst_na ), ctx.data1.b );
        return;
    }
    if(cs == NV12 || cs == NV21)
    {
        _mm_storel_epi64(( __m128i* )( dsta ), ctx.data1.a );
        _mm_storel_epi64(( __m128i* )( dst_na ), ctx.data1.c );
        _mm_storel_epi64(( __m128i* )( dstb ), ctx.data1.b );
        return;
    }
    if( cs == RGB24 )
    {
        _mm_storeu_si128(( __m128i* )( dsta ), ctx.data1.a );
        _mm_storel_epi64(( __m128i* )( dsta + 16 ), ctx.data1.b );

        //_mm_storeu_si128(( __m128i* )( dst_na ), ctx.data1.b );
        //_mm_storel_epi64(( __m128i* )( dst_na + 16 ), ctx.reserve.b );
    }

}

template <Colorspace cs> TARGET_INLINE void pack(register Context& ctx, register Context& ctx2)
{

    if( cs == NV12 )
    {
        //pack_YUV420( ctx.data1, ctx.reserve );
        //pack_YUV420( ctx2.data1, ctx2.reserve );

        ctx.data1.a = _mm_packs_epi32( ctx.data1.a, ctx2.data1.a );
        ctx.data1.a = _mm_packus_epi16( ctx.data1.a, _mm_setzero_si128());
        //ctx.reserve.a = _mm_packs_epi32( ctx.reserve.a, ctx2.reserve.a );
        ctx.reserve.a = _mm_packus_epi16( ctx.reserve.a, _mm_setzero_si128());

        ctx.data1.b = _mm_or_si128( _mm_slli_epi64( ctx.data1.c, 32), ctx.data1.b );
        ctx2.data1.b = _mm_or_si128( _mm_slli_epi64( ctx2.data1.c, 32), ctx2.data1.b );

        ctx.data1.b = _mm_packs_epi32( ctx.data1.b, ctx2.data1.b );
        ctx.data1.b = _mm_packus_epi16( ctx.data1.b, _mm_setzero_si128());
    }
    if( cs == RGB24 )
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
template <Colorspace cs> TARGET_INLINE void pack(register Context& ctx) __attribute__((__vectorcall));
template <Colorspace cs> TARGET_INLINE void pack(register Context& ctx)
{
    if( cs == RGB32 )
    {
        ctx.data1.b = _mm_or_si128(  _mm_slli_si128( ctx.data1.b, 1 ), ctx.data1.a); // G R
        ctx.data1.c = _mm_slli_si128( ctx.data1.c, 1 ); // X B

        ctx.data1.a = _mm_unpacklo_epi16( ctx.data1.b, ctx.data1.c );
        ctx.data1.b = _mm_unpackhi_epi16( ctx.data1.b, ctx.data1.c );
    }
    if( cs == BGR32 )
    {
        ctx.reserve.a = _mm_slli_si128( ctx.data1.b, 1 );
        ctx.data1.b = _mm_or_si128( ctx.reserve.a,  ctx.data1.c); // G B
        ctx.data1.c =  ctx.data1.a; // X R

        ctx.data1.a = _mm_unpacklo_epi16( ctx.data1.b, ctx.data1.c );
        ctx.data1.b = _mm_unpackhi_epi16( ctx.data1.b, ctx.data1.c );
    }
    if(cs == YVYU)
    {
        ctx.reserve.a = pack_YVYU( ctx.data1 );
        //ctx.reserve.b = pack_YVYU( ctx.reserve );
    }
    if(cs == YUYV)
    {
        ctx.reserve.a = pack_YUYV( ctx.data1 );
        //ctx.reserve.b = pack_YUYV( ctx.reserve );
    }

    if( cs == A2R10G10B10 )
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
    if( cs == A2B10G10R10 )
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


template <Colorspace cs> TARGET_INLINE void offset_yuv (VectorPixel& data, bool is_left, Context& ctx)
{
    if(IS_YUV( cs ))
    {

        //offset left
        if(is_left)
        {
            __m128i offset_y = _mm_loadu_si128( (__m128i*) ctx.spec.voffset_y_from );
            __m128i offset_u = _mm_loadu_si128( (__m128i*) ctx.spec.voffset_u_from );
            __m128i offset_v = _mm_loadu_si128( (__m128i*) ctx.spec.voffset_v_from );

            data.a = _mm_sub_epi16( data.a, offset_y );
            data.b = _mm_sub_epi16( data.b, offset_u );
            data.c = _mm_sub_epi16( data.c, offset_v );
        }
        else
        // offset right
        {
            __m128i offset_y = _mm_loadu_si128( (__m128i*) ctx.spec.voffset_y_to);
            __m128i offset_u = _mm_loadu_si128( (__m128i*) ctx.spec.voffset_u_to);
            __m128i offset_v = _mm_loadu_si128( (__m128i*) ctx.spec.voffset_v_to);

            data.a = _mm_add_epi16( data.a, offset_y );
            data.b = _mm_add_epi16( data.b, offset_u );
            data.c = _mm_add_epi16( data.c, offset_v );
        }
    }
}

template <Colorspace cs> static TARGET_INLINE void clip_point(__m128i& val_a, __m128i& val_b, __m128i& val_c)
{
    //puts("before");
    //std::cout << val_a << " "<< val_b << " " << val_c << std::endl;
    if(IS_8BIT( cs ) && IS_RGB( cs ))
    {
        val_a = clip(val_a, 0, 255);
        val_b = clip(val_b, 0, 255);
        val_c = clip(val_c, 0, 255);
    }
    else if( IS_10BIT( cs ) && IS_RGB( cs ))
    {
        val_a = clip(val_a, 0, 255 << 2);
        val_b = clip(val_b, 0, 255 << 2);
        val_c = clip(val_c, 0, 255 << 2);
    }
    else if( IS_YUV( cs ) && IS_8BIT( cs ) )
    {
        val_a = clip(val_a, 16, 235);
        val_b = clip(val_b, 16, 240);
        val_c = clip(val_c, 16, 240);
        //puts("after");
        //std::cout << val_a << " "<< val_b << " " << val_c << std::endl;
    }
    else if( IS_YUV( cs ) && IS_10BIT( cs ) )
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

static TARGET_INLINE void mat_mul(VectorPixel& data, Matrix& mat)
{
    VectorPixel pix1, pix2;
    pix1.a = _mm_unpacklo_epi16( data.a, _mm_setzero_si128() );
    pix1.b = _mm_unpacklo_epi16( data.b, _mm_setzero_si128() );
    pix1.c = _mm_unpacklo_epi16( data.c, _mm_setzero_si128() );

    pix2.a = _mm_unpackhi_epi16( data.a, _mm_setzero_si128() );
    pix2.b = _mm_unpackhi_epi16( data.b, _mm_setzero_si128() );
    pix2.c = _mm_unpackhi_epi16( data.c, _mm_setzero_si128() );

    __m128i r1, sub_r1;
    __m128i r2, sub_r2;
    __m128i r3, sub_r3;

    r1 = _mm_loadu_si128( (__m128i*)mat.row1 );
    r2 = _mm_loadu_si128( (__m128i*)mat.row2 );
    r3 = _mm_loadu_si128( (__m128i*)mat.row3 );

    sub_r1 = _mm_loadu_si128( (__m128i*)mat.subrow1 );
    sub_r2 = _mm_loadu_si128( (__m128i*)mat.subrow2);
    sub_r3 = _mm_loadu_si128( (__m128i*)mat.subrow3);

    __m128i tmp1 = multiple_add( pix1.a, pix1.b, pix1.c, r1, sub_r2);
    __m128i tmp2 = multiple_add( pix1.a, pix1.b, pix1.c, r2, sub_r2);
    __m128i tmp3 = multiple_add( pix1.a, pix1.b, pix1.c, r3, sub_r3);

    pix1.a = tmp1;
    pix1.b = tmp2;
    pix1.c = tmp3;

    tmp1 = multiple_add( pix2.a, pix2.b, pix2.c, r1, sub_r2);
    tmp2 = multiple_add( pix2.a, pix2.b, pix2.c, r2, sub_r2);
    tmp3 = multiple_add( pix2.a, pix2.b, pix2.c, r3, sub_r3);

    pix2.a = tmp1;
    pix2.b = tmp2;
    pix2.c = tmp3;

    round_shift( pix1.a, 8 );
    round_shift( pix1.b, 8 );
    round_shift( pix1.c, 8 );

    round_shift( pix2.a, 8 );
    round_shift( pix2.b, 8 );
    round_shift( pix2.c, 8 );

    data.a = _mm_packs_epi32( pix1.a, pix2.a );
    data.b = _mm_packs_epi32( pix1.b, pix2.b );
    data.c = _mm_packs_epi32( pix1.c, pix2.c );

}


template <Colorspace from_cs, Colorspace to_cs> TARGET_INLINE void transform (Context& ctx)
{

    scale<from_cs, to_cs>( ctx.data1.a, ctx.data1.b, ctx.data1.c );
    //scale<from_cs, to_cs>( ctx.reserve.a, ctx.reserve.b, ctx.reserve.c );

    //  matrix multiple
    if( (IS_RGB(from_cs) && IS_YUV(to_cs)) || (IS_YUV(from_cs) && IS_RGB(to_cs)))
    {
        offset_yuv <from_cs> (ctx.data1, SHIFT_LEFT, ctx);
        //offset_yuv <from_cs> (ctx.reserve, SHIFT_LEFT, ctx);

        mat_mul( ctx.data1, ctx.spec.t_matrix );
        //mat_mul( ctx.reserve, ctx.spec.t_matrix );

        offset_yuv <to_cs> (ctx.data1, SHIFT_RIGHT, ctx);
        //offset_yuv <to_cs> (ctx.reserve, SHIFT_RIGHT, ctx);

        clip_point <to_cs> (ctx.data1.a, ctx.data1.b, ctx.data1.c);
        //clip_point <to_cs> (ctx.reserve.a, ctx.reserve.b, ctx.reserve.c);

    } // if(from_cs_type != to_type)
}
template <Colorspace cs> TARGET_INLINE void next_row (uint8_t* &ptr_a, uint8_t* &ptr_b, uint8_t* &ptr_c, const size_t stride[3])
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

    uint8_t* src_a = meta.src_data[0];
    uint8_t* src_b = meta.src_data[1];
    uint8_t* src_c = meta.src_data[2];

    uint8_t* dst_a = meta.dst_data[0];
    uint8_t* dst_b = meta.dst_data[1];
    uint8_t* dst_c = meta.dst_data[2];

    register Context context1;
    //init_offset_yuv < from_cs, to_cs >( context1 );
    //set_transform_coeffs <from_cs, to_cs, st>( context1 );

    int8_t step;
    //if(from_cs == V210 || to_cs == V210)
    //    step = 6;
    if( IS_MULTISTEP( from_cs ) || IS_MULTISTEP( to_cs ))
        step = 16;
    else
        step = 8;
    size_t x,y;
    size_t shift_a, shift_b, shift_c;
    /*
    if(from_cs == V210 && to_cs == V210)
    {
        memcpy(dst_a, src_a, meta.width * meta.height * 16 / 6);
    }
    else
    */
    for(y = 0; y < meta.height; y += 2) {
        for(x = 0; x+step <= meta.width; x += step) {
            // Process 2 x 2(Step) pixels

            //if(to_cs == V210 && from_cs != V210)
            if( !IS_MULTISTEP( from_cs ) && IS_MULTISTEP( to_cs ) )
            {
                get_pos <from_cs>(shift_a, shift_b, shift_c, x);
                load < from_cs, 0> (meta, context1,
                                        src_a + shift_a,
                                        src_b + shift_b,
                                        src_c + shift_c);

                get_pos <from_cs>(shift_a, shift_b, shift_c, x + 4);
                load < from_cs, 0> (meta, context1,
                                            src_a + shift_a,
                                            src_b + shift_b,
                                            src_c + shift_c);
                unpack <from_cs> (context1, context1);
                unpack <from_cs> (context1, context1);

                transform <from_cs, to_cs> (context1 );
                transform <from_cs, to_cs> (context1 );

                pack< to_cs >(context1, context1);

                get_pos <to_cs>(shift_a, shift_b, shift_c, x);
                store<to_cs, 0> (meta, context1,
                                     dst_a + shift_a,
                                     dst_b + shift_b,
                                     dst_c + shift_c);
            }
            //else if(from_cs == V210 && to_cs != V210)
            else if( IS_MULTISTEP( from_cs ) && !IS_MULTISTEP( to_cs))
            {
                get_pos <from_cs>(shift_a, shift_b, shift_c, x);
                load < from_cs, 0> (meta, context1,
                                            src_a + shift_a,
                                            src_b + shift_b,
                                            src_c + shift_c);
                unpack <from_cs> (context1, context1);

                transform <from_cs, to_cs> (context1 );
                transform <from_cs, to_cs> (context1 );

                pack< to_cs >(context1, context1);
                pack< to_cs >(context1, context1);

                get_pos <to_cs>(shift_a, shift_b, shift_c, x);
                store< to_cs, 0> (meta, context1,
                                     dst_a + shift_a,
                                     dst_b + shift_b,
                                     dst_c + shift_c);
                get_pos <to_cs>(shift_a, shift_b, shift_c, x + step / 2);
                store< to_cs, 0> (meta, context1,
                                     dst_a + shift_a,
                                     dst_b + shift_b,
                                     dst_c + shift_c);
            }
            //else if(from_cs != V210 && to_cs != V210)
            if( !IS_MULTISTEP( from_cs ) && !IS_MULTISTEP( to_cs ) )
            {
                get_pos <from_cs>(shift_a, shift_b, shift_c, x);
                load < from_cs, 0> (meta, context1,
                                        src_a + shift_a,
                                        src_b + shift_b,
                                        src_c + shift_c);
                unpack <from_cs> (context1);


                scale<from_cs, to_cs>( context1.data1.a, context1.data1.b, context1.data1.c );
                if( (IS_RGB(from_cs) && IS_YUV(to_cs)) || (IS_YUV(from_cs) && IS_RGB(to_cs)))
                    transform <from_cs, to_cs> ( context1 );

                pack<to_cs > (context1);

                get_pos <to_cs>(shift_a, shift_b, shift_c, x);
                store<to_cs, 0> (meta, context1,
                                             dst_a + shift_a,
                                             dst_b + shift_b,
                                             dst_c + shift_c);
                get_pos <from_cs>(shift_a, shift_b, shift_c, x);
                load < from_cs, 1> (meta, context1,
                                        src_a + shift_a,
                                        src_b + shift_b,
                                        src_c + shift_c);
                unpack <from_cs> ( context1 );

                scale<from_cs, to_cs>( context1.data1.a, context1.data1.b, context1.data1.c );
                if( (IS_RGB(from_cs) && IS_YUV(to_cs)) || (IS_YUV(from_cs) && IS_RGB(to_cs)))
                    transform <from_cs, to_cs> ( context1 );


                pack<to_cs > ( context1 );
                get_pos <to_cs>(shift_a, shift_b, shift_c, x);
                store<to_cs, 1> (meta, context1,
                                             dst_a + shift_a,
                                             dst_b + shift_b,
                                             dst_c + shift_c);


            }
            if( IS_MULTISTEP( from_cs ) && IS_MULTISTEP( to_cs ) )
            {
                get_pos <from_cs>(shift_a, shift_b, shift_c, x);
                load < from_cs, 0> (meta, context1,
                                        src_a + shift_a,
                                        src_b + shift_b,
                                        src_c + shift_c);

                unpack <from_cs> (context1, context1);

                transform <from_cs, to_cs> (context1 );
                transform <from_cs, to_cs> (context1 );

                pack<to_cs > (context1, context1);

                get_pos <to_cs>(shift_a, shift_b, shift_c, x);
                store<to_cs, 0> (meta, context1,
                                             dst_a + shift_a,
                                             dst_b + shift_b,
                                             dst_c + shift_c);
            }

        }
        // finish line
        if(meta.width % step != 0  && to_cs == V210 && from_cs != V210)
        {
            context1.data1.a = _mm_setzero_si128();
            context1.data1.b = _mm_setzero_si128();
            context1.data1.c = _mm_setzero_si128();

            //context1.reserve.a = _mm_setzero_si128();
            //context1.reserve.b = _mm_setzero_si128();
            //context1.reserve.c = _mm_setzero_si128();

            get_pos <from_cs>(shift_a, shift_b, shift_c, x);
            load < from_cs, 0> (meta, context1,
                                    src_a + shift_a,
                                    src_b + shift_b,
                                    src_c + shift_c);

            if(meta.width % 6 >= 4) //==4
            {
                get_pos <from_cs>(shift_a, shift_b, shift_c, x + 2);
                load < from_cs, 0> (meta, context1,
                                        src_a + shift_a,
                                        src_b + shift_b,
                                        src_c + shift_c);
            }

            unpack <from_cs> (context1, context1);
            unpack <from_cs> (context1, context1);

            transform <from_cs, to_cs> (context1 );
            transform <from_cs, to_cs> (context1 );

            pack< to_cs >(context1, context1);

            get_pos <to_cs>(shift_a, shift_b, shift_c, x);
            store<to_cs, 0> (meta, context1,
                                 dst_a + shift_a,
                                 dst_b + shift_b,
                                 dst_c + shift_c);
        }

        next_row <from_cs> (src_a, src_b, src_c, meta.src_stride);
        next_row <to_cs> (dst_a, dst_b, dst_c, meta.dst_stride_horiz);
	}
}

} // namespace ColorspaceConverter_SSE2;

#endif
