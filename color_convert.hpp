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
    uint8_t* src_data;
    uint8_t* dst_data;
    size_t cur_row;
    size_t cur_col;
};

//template < Standart st, bool isInverse> int16_t* set_matrix_koeff();
//template <Depth src_depth> void read_file(Context &ctx, char* file_name);
//template <Depth dst_depth> void write_file(Context &ctx, char* file_name);
//template <Pack data_pack, Colorspace cs> void unpack(Context &ctx);
//template<Pack data_pack, Colorspace cs> void pack(Context &ctx);
//template< bool isInverse> void multiple(Context &ctx);
#include "color_convert.hpp"
int16_t matrix_bt601_RGB_to_YUV[] =
{
    77, 150, 29,
    -44, -87, 131,
    131, -110, -21,
};
int16_t matrix_bt601_YUV_to_RGB[] =
{
    1, 2, 3,
    4, 5, 6,
    7, 8, 9
};

template < Standart st, bool isInverse>  inline  int16_t* set_matrix_koeff()
{
    if(isInverse)
    {
        //if(st == BT_709) return matrix_bt601;
        //if(st == BT_2020) return matrix_bt2020;
        return matrix_bt601_YUV_to_RGB;
    } else
        {
            //if(st == BT_709) return matrix_bt601;
            //if(st == BT_2020) return matrix_bt2020;
            return matrix_bt601_RGB_to_YUV;
        }
}
template <Pack data_pack, Colorspace cs> inline void unpack(const Convert_meta& meta, Context &ctx)
{
    int ch_a, ch_b, ch_c;
    //std::cout << "row is " << ctx.cur_row << " col is" << ctx.cur_col << std::endl;
    int cur_pos = meta.cur_row * meta.width + meta.cur_col;
    //std::cout << "pos is " << cur_pos << std::endl;

    if ( data_pack == Planar)
    {
        //std::cout <<"Planar unpack" << std::endl;
        int frame_size = meta.width * meta.height;
            ch_a = meta.src_data[cur_pos];
            ch_b = meta.src_data[cur_pos + frame_size];
            ch_c = meta.src_data[cur_pos + 2 * frame_size];
    } else
        if ( data_pack == Interleaved)
        {
                cur_pos *= 3;
            //std::cout <<"Interleaved unpack" << std::endl;
                ch_a = meta.src_data[cur_pos + 0];
                ch_b = meta.src_data[cur_pos + 1];
                ch_c = meta.src_data[cur_pos + 2];
                //std::cout << ch_a[i] << " " << ch_b[i] <<" "<<ch_c[i] << std::endl;

        }
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

template<Pack data_pack, Colorspace cs> inline void pack(const Convert_meta& meta  ,Context &ctx)
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
    if(data_pack  == Planar)
    {
        int plane_size = meta.width * meta.height * sizeof(uint8_t);
        int cur_pos = meta.cur_row * meta.width + meta.cur_col;
        uint8_t* dst_a = meta.dst_data + cur_pos;
        uint8_t* dst_b = meta.dst_data + cur_pos + plane_size;
        uint8_t* dst_c = meta.dst_data + cur_pos + 2 * plane_size;

            *dst_a = ch_a;
            *dst_b = ch_b;
            *dst_c = ch_c;
    }
}

template< bool isInverse> inline void multiple(Context &ctx, const int16_t* koeff_matrix)
{
    int src_a = 0;
    int src_b = 0;
    int src_c = 0;
    int dst_a, dst_b, dst_c;
        if(isInverse){
            src_a = ctx.y - 16;
            src_b = ctx.u -128;
            src_c = ctx.v -128;
        } else  {
                    src_a = ctx.r;
                    src_b = ctx.g;
                    src_c = ctx.b;
                }

        std::cout << koeff_matrix[0] << " " << koeff_matrix[1] << " " << koeff_matrix[2] << std::endl;
        std::cout << "****";
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

template<Colorspace from_cs , Pack from_pack, Colorspace to_cs, Pack to_pack, Standart st> void convert(Convert_meta& meta)
{
	Context ctx;
	int16_t* koeff_matrix;
    if( (from_cs == RGB)  && (to_cs == YUV444) )
        koeff_matrix = (int16_t*) set_matrix_koeff < st, false >();
            else
                koeff_matrix = (int16_t*) set_matrix_koeff < st, true >();
    for(meta.cur_row = 0; meta.cur_row < meta.height; ++meta.cur_row)
        for(meta.cur_col = 0; meta.cur_col < meta.width; ++meta.cur_col)
        {
            unpack < from_pack ,from_cs >(meta, ctx);

            if( (from_cs == RGB)  && (to_cs == YUV444) )
                multiple < false > (ctx, koeff_matrix);
                    else
                        multiple < true > (ctx, koeff_matrix);

            pack < to_pack, to_cs > (meta , ctx);
        }
}

#endif
