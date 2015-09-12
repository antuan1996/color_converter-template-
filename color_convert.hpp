#ifndef COLOR_CONVERT
#define COLOR_CONVERT

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <iostream>
#include <sstream>

enum Colorspace {
	//YUV420 = 1<<0,
	//YUV422 = 1<<1,
	YUV444 = 1<<2,
	RGB    = 1<<3,
	//ARGB   = 1<<4
};
enum Standart{
  BT_601,
  BT_709,
  BT_2020
};
enum Pack {
	Planar      = 1<<16,
	SemiPlanar  = 1<<17,
	Interleaved = 1<<18,
};

typedef int Format;
typedef int Depth;

struct Context
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
    // some additionall fields r g b
 	uint8_t y;
	uint8_t u;
	uint8_t v;
    // some additionall fields y u v
};
struct Convert_meta
{

    int width, height;

    uint8_t* src_data[3];
    int src_stride[3];


    uint8_t* dst_data[3];
    int dst_stride[3];

};
static const int16_t k_matrix_bt601_RGB_to_YUV[] =
{
    77, 150, 29,
    -44, -87, 131,
    131, -110, -21,
};
static const int16_t k_matrix_bt601_YUV_to_RGB[] =
{
    256, 0 , 351,
    256, -86, -179,
    256, 444, 0
};

template < Standart st, bool isInverse>  inline  const int16_t* set_matrix_koeff()
{
    if(isInverse)
    {
        //if(st == BT_709) return matrix_bt601;
        //if(st == BT_2020) return matrix_bt2020;
        return k_matrix_bt601_YUV_to_RGB;
    } else
        {
            //if(st == BT_709) return matrix_bt601;
            //if(st == BT_2020) return matrix_bt2020;
            return k_matrix_bt601_RGB_to_YUV;
        }
}
template <Colorspace cs> inline void unpack(const uint8_t* src_a, const uint8_t* src_b, const uint8_t* src_c, Context &ctx)
{
    int ch_a, ch_b, ch_c;
    //std::cout << "row is " << ctx.cur_row << " col is" << ctx.cur_col << std::endl;
    //std::cout << "pos is " << cur_pos << std::endl;
    ch_a = *src_a;
    ch_b = *src_b;
    ch_c = *src_c;
        if( cs == RGB)
        {
            ctx.r = ch_a;
            ctx.g = ch_b;
            ctx.b = ch_c;

        } else  {
                    ctx.y = ch_a;
                    ctx.u=  ch_b;
                    ctx.v = ch_c;
                }
}

template<Pack data_pack, Colorspace cs> inline void pack(uint8_t* dst_a, uint8_t* dst_b, uint8_t* dst_c, Context &ctx)
{
    int ch_a;
    int ch_b;
    int ch_c;

        if(cs == RGB)
        {
            ch_a = ctx.r;
            ch_b = ctx.g;
            ch_c = ctx.b;
        } else {
                    ch_a = ctx.y;
                    ch_b = ctx.u;
                    ch_c = ctx.v;
                }
    *dst_a = ch_a;
    *dst_b = ch_b;
    *dst_c = ch_c;
}

template< bool isInverse> inline void multiple(Context &ctx, const int16_t* koeff_matrix)
{
    int src_a = 0;
    int src_b = 0;
    int src_c = 0;
    int dst_a, dst_b, dst_c;
        if(isInverse){
            src_a = ctx.y;
            src_b = ctx.u -128;
            src_c = ctx.v -128;
        } else  {
                    src_a = ctx.r;
                    src_b = ctx.g;
                    src_c = ctx.b;
                }

        //std::cout << koeff_matrix[0] << " " << koeff_matrix[1] << " " << koeff_matrix[2] << std::endl;
        //std::cout << "****";
        dst_a = koeff_matrix[0] * src_a + koeff_matrix[1] * src_b + koeff_matrix[2] * src_c;
        dst_b = koeff_matrix[3] * src_a + koeff_matrix[4] * src_b + koeff_matrix[5] * src_c;
        dst_c = koeff_matrix[6] * src_a + koeff_matrix[7] * src_b + koeff_matrix[8] * src_c;
        dst_a >>= 8;
        dst_b >>= 8;
        dst_c >>= 8;

        std::cout << src_a << " " << src_b << " " << src_c << std::endl;
        std::cout << dst_a << " " << dst_b << " " << dst_c << std::endl;
        std::cout << "******" << std::endl;


        if(isInverse){
            ctx.r = dst_a;
            ctx.g = dst_b;
            ctx.b = dst_c;
        } else  {
                    ctx.y = dst_a;
                    ctx.u = dst_b + 128;
                    ctx.v = dst_c + 128;
                }
}

template<Colorspace from_cs , Pack from_pack, Colorspace to_cs, Pack to_pack, Standart st> void convert(const Convert_meta& meta)
{
	Context ctx;
	const int16_t* koeff_matrix;
    if( (from_cs == RGB)  && (to_cs == YUV444) )
        koeff_matrix = set_matrix_koeff < st, false >();
            else
                koeff_matrix =  set_matrix_koeff < st, true >();

    uint8_t* src_a = meta.src_data[0];
    uint8_t* src_b = meta.src_data[1];
    uint8_t* src_c = meta.src_data[2];
    uint8_t* dst_a = meta.dst_data[0];
    uint8_t* dst_b = meta.dst_data[1];
    uint8_t* dst_c = meta.dst_data[2];

    for(int row = 0; row < meta.height; ++row)
    {
        for(int col = 0; col < meta.width; ++col)
        {
            unpack <from_cs >( src_a, src_b, src_c, ctx);

            if( (from_cs == RGB)  && (to_cs == YUV444) )
                multiple < false > (ctx, koeff_matrix);
                    else
                        multiple < true > (ctx, koeff_matrix);

            pack < to_pack, to_cs > (dst_a, dst_b, dst_c, ctx);

            if(from_pack == Planar){
                src_a += 1;
                src_b += 1;
                src_c += 1;
            }
            if(from_pack == Interleaved){
                src_a += 3;
                src_b += 3;
                src_c += 3;
            }
            if(to_pack == Planar){
                dst_a += 1;
                dst_b += 1;
                dst_c += 1;
            }
            if(to_pack == Interleaved){
                dst_a += 3;
                dst_b += 3;
                dst_c += 3;
            }
        }
        src_a += meta.src_stride[0];
        src_b += meta.src_stride[1];
        src_c += meta.src_stride[2];
        dst_a += meta.dst_stride[0];
        dst_b += meta.dst_stride[1];
        dst_c += meta.dst_stride[2];
    }
}

#endif
