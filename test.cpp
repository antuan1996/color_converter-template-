//#define ENABLE_LOG
#define SSE2
#define PACK_A2R10G10B10(a, b, c) ( pack10_in_int(a, b, c)  | (3 << 30) )

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <ctime>

#ifdef SSE2
#include "color_convert_sse2.hpp"
using namespace ColorspaceConverter_SSE2;
#else
#include "color_convert.hpp"
using namespace ColorspaceConverter;
#endif


std::ostringstream fakestream;
#ifdef ENABLE_LOG
    #define DEBUG std::cout
#else
    #define DEBUG fakestream
#endif
static int fifo_test(int argc, char *argv[])
{
#ifdef ENABLE_LOG
    freopen("report.txt","w",stdout);
#endif

    ConvertMeta info;
	info.width = std::stoi(argv[3]);
    info.height = std::stoi(argv[4]);
	size_t stride = info.width;
	for(size_t i = 0; i < 3; ++i) {
		info.src_stride[i] = stride; // YUV 4:4:4
        info.dst_stride_horiz[i] = stride * 3; // RGB
	}

    size_t frame_size = stride * info.height;

    DEBUG << "Getting memory" << std::endl;
	std::shared_ptr <uint8_t> input_buffer  = std::shared_ptr<uint8_t> (new uint8_t[frame_size * 3], std::default_delete<uint8_t> ());
	std::shared_ptr <uint8_t> output_buffer = std::shared_ptr<uint8_t> (new uint8_t[frame_size * 3], std::default_delete<uint8_t> ());

	// Planar YUV444
	info.src_data[0] = input_buffer.get();
    info.src_data[1] = info.src_data[0] + frame_size;
    info.src_data[2] = info.src_data[1] + frame_size;

	// Interleaved RGB24
    info.dst_data[0] = output_buffer.get();
    info.dst_data[1] = nullptr;
    info.dst_data[2] = nullptr;

    std::cout << "Reading data" << std::endl;

	std::ifstream in;
	in.open(argv[1], std::ios::binary);
	if (!in.is_open()) {
		std::cout << "Can't open yuv file" << std::endl;
		return -3;
	}
	in.read(reinterpret_cast<char*>(input_buffer.get()), frame_size * 3);
	in.close();

    std::cout << "I'm ready" << std::endl;
    colorspace_convert<YUV444, RGB24, BT_601> (info);

    std::cout << "Writng results" << std::endl;

    std::ofstream out;
	out.open(argv[2], std::ios::binary);
	if (!out.is_open()) {
		std::cout << "Can't open rgb file" << std::endl;
		return -4;
	}
	out.write(reinterpret_cast<char*>(output_buffer.get()), frame_size * 3);
	out .close();

    std::cout << "Exit" << std::endl;
    return 0;
}

static void print_interleaved(ConvertMeta info)
{

    printf("Interleaved:\n");
    for (size_t y = 0; y < info.dst_stride_vert[ 0 ]; y++) {
        for (size_t x = 0; x < info.dst_stride_horiz[ 0 ]; x+=3) {
            printf("%d,%d,%d   ",
                   info.dst_data[ 0 ][ x + y * info.dst_stride_horiz[ 0 ] ],
                   info.dst_data[ 0 ][ x + 1 + y * info.dst_stride_horiz[ 0 ] ],
                   info.dst_data[ 0 ][ x + 2 + y * info.dst_stride_horiz[ 0 ] ]);
        }
        printf("\n");
    }
    printf("\n");

    for (size_t y = 0; y < info.dst_stride_vert[ 1 ]; y++) {
        for (size_t x = 0; x < info.dst_stride_horiz[ 1 ]; x+=3) {
            printf("%d,%d,%d   ",
                   info.dst_data[ 1 ][ x + y * info.dst_stride_horiz[ 1 ] ],
                   info.dst_data[ 1 ][ x + 1 + y * info.dst_stride_horiz[ 1 ] ],
                   info.dst_data[ 1 ][ x + 2 + y * info.dst_stride_horiz[ 1 ] ]);
        }
        printf("\n");
    }
    printf("\n");
    for (size_t y = 0; y < info.dst_stride_vert[ 2 ]; y++) {
        for (size_t x = 0; x < info.dst_stride_horiz[ 2 ]; x+=3) {
            printf("%d,%d,%d   ",
                   info.dst_data[ 2 ][ x + y * info.dst_stride_horiz[ 2 ] ],
                   info.dst_data[ 2 ][ x + 1 + y * info.dst_stride_horiz[ 2 ] ],
                   info.dst_data[ 2 ][ x + 2 + y * info.dst_stride_horiz[ 2 ] ]);

        }
        printf("\n");
    }
    printf("\n");
}


static void print_planar(ConvertMeta info)
{
    printf("Planar:\n");
    for (size_t y = 0; y < info.dst_stride_vert[ 0 ]; ++y) {
        for (size_t x = 0; x < info.dst_stride_horiz[ 0 ]; ++x) {
            printf("%3d, ", info.dst_data[ 0 ][ x + y * info.dst_stride_horiz[ 0 ] ]);
        }
        printf("\n");
    }
    printf("\n");

    for (size_t y = 0; y < info.dst_stride_vert[ 1 ]; ++y) {
        for (size_t x = 0; x < info.dst_stride_horiz[ 1 ]; ++x) {
            printf("%3d, ", info.dst_data[ 1 ][ x + y * info.dst_stride_horiz[ 1 ]]);
        }
        printf("\n");
    }
    printf("\n");

    for (size_t y = 0; y < info.dst_stride_vert[ 2 ]; ++y) {
        for (size_t x = 0; x < info.dst_stride_horiz[ 2 ]; ++x) {
            printf("%3d, ", info.dst_data[ 2 ][ x + y * info.dst_stride_horiz[ 2 ]]);
        }
        printf("\n");
    }
    printf("\n");

}

int check( uint8_t* a, uint8_t* b, int len )
{
    int res = 0;
    for( size_t x = 0 ; x < uint(len) ; ++x )
    {
        #ifdef ENABLE_LOG
            std::cout << int(a[ x ]) << " x " << int(b[ x ]) << "\n";
        #endif
        res += abs( a[ x ] - b[ x ] );
    }
    return res;
}



static int syntetic_test()
{
// p = planar
// s = semiplanar
// i = interleaved
     static uint8_t test_yuv422i_bt601[(8 * 2) * 2] =
	{
		235,128,    235,128,   81,90,   81,240,   145,54,   145,34,   41,240,  41,110,
        235,128,    235,128,   81,90,   81,240,   145,54,   145,34,   41,240,  41,110,
    };
    static uint8_t test_yuv444p_bt601[(8 * 2) * 3] =
	{
		// Y
		235, 81, 145, 41, 16, 210, 170, 106,
        107, 157, 74, 178, 94, 147, 115, 95,

        // Cb
		128, 90 , 54, 240, 128, 16, 166, 202,
        202, 111, 221, 34, 147, 52, 71, 147,

		// Cr
		128, 240, 34, 110, 128, 146, 16, 222,
        62, 25, 167, 92, 231, 192, 137, 71
	};

	static uint8_t test_yuv444p_bt709[(8 * 2) * 3] =
	{
		// Y
		235, 63, 173, 32, 16, 219, 188, 78,
        112, 181, 56, 196, 71, 143, 120, 104,

        // Cb
		128, 102, 42, 240, 128, 16, 154, 214,
        196, 99, 227, 29, 160, 58, 71, 141,

		// Cr
		128, 240, 26, 118, 128, 138, 16, 230,
        66, 21, 175, 83, 235, 188, 133, 71
    };
    static uint32_t test_A2R10G10B10_bt601[(8 * 2)] =
    {
        PACK_A2R10G10B10(0x3FC, 0x3FC, 0x3FC),    PACK_A2R10G10B10(0x3FC, 0,0),      PACK_A2R10G10B10(0, 0x3FC, 0),     PACK_A2R10G10B10(0, 0, 0x3FC),
        PACK_A2R10G10B10(0x3FC, 0x3FC, 0),        PACK_A2R10G10B10(0, 0x3FC, 0x3FC), PACK_A2R10G10B10(0x3FC, 0, 0x3FC), PACK_A2R10G10B10(0x3FC, 0x3FC, 0x3FC),
        PACK_A2R10G10B10(0x3FC, 0x3FC, 0x3FC),    PACK_A2R10G10B10(0x3FC, 0,0),      PACK_A2R10G10B10(0, 0x3FC, 0),     PACK_A2R10G10B10(0, 0, 0x3FC),
        PACK_A2R10G10B10(0x3FC, 0x3FC, 0),        PACK_A2R10G10B10(0, 0x3FC, 0x3FC), PACK_A2R10G10B10(0x3FC, 0, 0x3FC), PACK_A2R10G10B10(0x3FC, 0x3FC, 0x3FC),
    };
    static uint8_t test_RGB24_to_A2R10G10B10 [16 * 3 * 2] =
    {
        255,255,255,   255,0,0,   0,255,0,   0,0,255,   255,255,0,   0,255,255,   255,0,255,   255,255,255,
        255,255,255,   255,0,0,   0,255,0,   0,0,255,   255,255,0,   0,255,255,   255,0,255,   255,255,255,
        255,255,255,   255,0,0,   0,255,0,   0,0,255,   255,255,0,   0,255,255,   255,0,255,   255,255,255,
        255,255,255,   255,0,0,   0,255,0,   0,0,255,   255,255,0,   0,255,255,   255,0,255,   255,255,255

    };
    static uint8_t test_RGB32 [8 * 4 * 2] =
    {
        255,255,255,0,   255,0,0,0,    0,255,0,0,   0,0,255,0,   255,255,0,0,   0,255,255,0,   255,0,255,0,   255,255,255,0,
        255,255,255,0,   255,0,255,0,  255,255,0,0, 0,255,255,0, 0,0,255,0,     0,255,0,0,     255,0,0,0,     255,255,255,0
    };
    static uint8_t test_RGB32_to_YUV420 [8 * 4 * 2] =
    {
        255,255,255,0, 255,255,255,0,     255,0,0,0, 255,0,0,0,     0,255,0,0, 0,255,0,0,      0,0,255,0, 0,0,255,0,
        255,255,255,0, 255,255,255,0,     255,0,0,0, 255,0,0,0,     0,255,0,0, 0,255,0,0,      0,0,255,0, 0,0,255,0,
    };
    static uint8_t test_RGB24_to_YUV420 [8 * 4 * 2] =
    {
        255,255,255,    255,255,255,     255,0,0,    255,0,0,     0,255,0,      0,255,0,      0,0,255,   0,0,255,
        255,255,255,    255,255,255,     255,0,0,    255,0,0,     0,255,0,      0,255,0,      0,0,255,   0,0,255
    };

	static uint8_t test_yuv444p_bt2020[(8 * 2) * 3] =
	{
		// Y
		235, 74, 164, 29, 16, 222, 177, 87,
        105, 171, 58, 194, 80, 149, 121, 99,

        // Cb
		128, 97 , 47, 240, 128, 16, 159, 209,
        199, 104, 224, 31, 154, 55, 71, 144,

		// Cr
		128, 240, 25, 119, 128, 137, 16, 231,
        66, 20, 176, 82, 235, 187, 133, 71
	};

    static uint8_t test_yuv422p_bt601[(8 * 2) + 8 + 8] =
    {
        235, 235, 81, 81, 145, 145, 41, 41,
        41, 41, 145, 145, 81, 81, 235, 235,
        128, 90,  54, 240,   240, 54, 90,128,
        128, 240, 34, 110,  110, 34,  240, 128
    };


    static uint8_t test_yuv420p_bt601[(8 * 2) + 4 + 4] =
    {
        235, 235, 81, 81, 145, 145, 41, 41,
        235, 235, 81, 81, 145, 145, 41, 41,
        128, 90,  54, 240,
        128, 240, 34, 110
    };

    static uint8_t test_yuv420s_bt601[(8 * 2) + 4 + 4] =
    {
        235, 235, 81, 81, 145, 145, 41, 41,
        235, 235, 81, 81, 145, 145, 41, 41,
        128,128,  90, 240,54,34,240,110

    };

    static uint8_t test_rgbf[(8 * 2) * 3] =
    {
        255,255,255,  255,0,0,  0,255,0,   0,0,255,  0,0,0,    255,255,0,  0,255,255,  255,0,255,
        0,128,255,   0,255,128, 128,0,255,  128,255,0, 255,0,128,  255,128,0,  128,128,0,  0,128,128
    };

    static uint16_t test_P210[ 8 * 2 + 4 * 2 + 4 * 2 ]
    {
        940 << 6, 940 << 6, 324 << 6, 324 << 6, 580 << 6, 580 << 6, 164 << 6, 164 << 6,
        64 << 6, 64 << 6, 840 << 6, 840 << 6, 680 << 6, 680 << 6, 424 << 6, 424 << 6,

        512 << 6, 512 << 6, 360 << 6, 960 << 6, 216 << 6, 136 << 6, 960 << 6, 440 << 6,
        512 << 6, 512 << 6, 64 << 6, 584 << 6, 664 << 6, 64 << 6, 808 << 6, 888 << 6,
    };
    static uint16_t test_Y210[ 8 * 2 + 4 * 2 + 4 * 2 ]
    {
        940 << 6, 512 << 6, 940 << 6, 512 << 6,
        324 << 6, 360 << 6, 324 << 6, 960 << 6,
        580 << 6, 216 << 6, 580 << 6, 136 << 6,
        164 << 6, 960 << 6, 164 << 6, 440 << 6,
        64 << 6, 512 << 6, 64 << 6, 512 << 6,
        840 << 6, 64 << 6, 840 << 6,584 << 6,
        680 << 6, 664 << 6, 680 << 6, 64 << 6,
        424 << 6, 808 << 6, 424 << 6,888 << 6,
    };

    static uint8_t test_yuv444p_to_yuv420[ 3 * 8 * 2]
    {
        235, 235, 81, 81, 145, 145, 41, 41,
        235, 235, 81, 81, 145, 145, 41, 41,

        128, 128, 90, 90, 54, 54 ,240, 240,
        128, 128, 90, 90, 54, 54 ,240, 240,

        128, 128, 240, 240, 34, 34, 110, 110,
        128, 128, 240, 240, 34, 34, 110, 110,

    };
    static uint32_t test_V210[ 4 * 2 ]
    {
        pack10_in_int( 512, 940, 512 ),
        pack10_in_int( 324, 360, 940 ),
        pack10_in_int( 216, 324, 960 ),
        pack10_in_int( 580, 136, 580 ),

        pack10_in_int( 512, 940, 512 ),
        pack10_in_int( 324, 360, 940 ),
        pack10_in_int( 216, 324, 960 ),
        pack10_in_int( 580, 136, 580 )
    };
    static uint8_t test_yuv444p_to_V210[ 3 * 6 * 2]
    {
        235, 235,    81,81, 145, 145,
        235, 235,    81,81, 145, 145,

        128, 128,   90, 90,   54, 54,
        128, 128,   90, 90,   54, 54,

        128, 128, 240, 240,   34, 34,
        128, 128, 240, 240,   34, 34

    };
    static uint8_t test_yuv444p_to_yuv422[ 3 * 8 * 2]
    {
        235, 235,    81,81, 145, 145,    41, 41,
          16, 16,  210,210, 170, 170,   106,106,

        128, 128,   90, 90,   54, 54,  240, 240,
        128, 128,   16, 16, 166, 166,  202, 202,

        128, 128, 240, 240,   34, 34,  110, 110,
        128, 128, 146, 146,   16, 16,  222, 222
    };

    static uint16_t test_P010[ 8 * 2 + 4 + 4 ]
    {
        940 << 6, 940 << 6, 324 << 6, 324 << 6, 580 << 6, 580 << 6, 164 << 6, 164 << 6,
        940 << 6, 940 << 6, 324 << 6, 324 << 6, 580 << 6, 580 << 6, 164 << 6, 164 << 6,

        512 << 6, 512 << 6, 360 << 6, 960 << 6, 216 << 6, 136 << 6, 960 << 6, 440 << 6,
    };


    ConvertMeta info;
    /*
    std::cout << "Repack test\n";
    std::cout << "YUYV to YVYU-------------------------\n";
    set_meta <YUYV, UYVY >(info, 8, 2, test_yuv422i_bt601);
    colorspace_convert<YUYV, YVYU, BT_601> (info);
    print_planar( info ) ;
    */

    std::cout << "A2R10G10B10 to RGB32 interleaved\n";
    set_meta < A2R10G10B10, RGB32>(info, 8, 2, (uint8_t*)test_A2R10G10B10_bt601);
    colorspace_convert<A2R10G10B10, RGB32, BT_601> ( info );
    print_planar( info );

    /*
    std::cout << "RGB32 to A2R10G10B10 interleaved\n";
    set_meta <RGB32, A2R10G10B10>(info, 8, 2, test_RGB32_to_A2R10G10B10);
    colorspace_convert<RGB32, A2R10G10B10, BT_601> ( info );
    print_planar( info );
    std::cout << "Diff is " << check( (uint8_t*)(test_A2R10G10B10_bt601), info.dst_data[ 0 ], 8 * 4 * 2 ) << "\n";
    */

/*
    std::cout << "NV12 to NV12-------------------------\n";
    set_meta <NV12, NV12 >(info, 8, 2, test_yuv420s_bt601, test_yuv420s_bt601 + 8 * 2);
    colorspace_convert<NV12, NV12, BT_601> (info);
    print_planar( info ) ;

    std::cout << "YUYV to NV12-------------------------\n";
    set_meta <YUYV, NV12 >(info, 8, 2, test_yuv422i_bt601 );
    colorspace_convert<YUYV, NV12, BT_601> (info);
    print_planar( info ) ;
*/
    std::cout << "YUYV to YVYU-------------------------\n";
    set_meta <YUYV, YVYU >(info, 8, 2, test_yuv422i_bt601 );
    colorspace_convert<YUYV, YVYU, BT_601> (info);
    print_planar( info ) ;
/*
    std::cout << "NV12 to YUYV-------------------------\n";
    set_meta <NV12, YUYV >(info, 8, 2, test_yuv420s_bt601, test_yuv420s_bt601 + 8 * 2);
    colorspace_convert<NV12, YUYV, BT_601> (info);
    print_planar( info ) ;


    std::cout << "NV12 to RGB32-------------------------\n";
    set_meta <NV12, RGB32 >(info, 8, 2, test_yuv420s_bt601, test_yuv420s_bt601 + 8 * 2);
    colorspace_convert<NV12, RGB32, BT_601> (info);
    print_planar( info ) ;


    std::cout << "RGB32 to NV12-------------------------\n";
    set_meta <RGB32, NV12 >(info, 8, 2, test_RGB32_to_YUV420);
    colorspace_convert<RGB32, NV12, BT_601> (info);
    print_planar( info ) ;

    std::cout << "RGB24 to NV12-------------------------\n";
    set_meta <RGB24, NV12 >(info, 8, 2, test_RGB24_to_YUV420);
    colorspace_convert<RGB24, NV12, BT_601> (info);
    print_planar( info ) ;


    std::cout << "RGB32 to RGB24-------------------------\n";
    set_meta <RGB32, RGB24 >(info, 8, 2, test_RGB32);
    colorspace_convert<RGB32, RGB24, BT_601> (info);
    print_interleaved( info ) ;
*/
    std::cout << "RGB24 to RGB32-------------------------\n";
    set_meta <RGB24, RGB32 >(info, 16, 2, test_RGB24_to_A2R10G10B10);
    colorspace_convert<RGB24, RGB32, BT_601> (info);
    print_planar( info ) ;


    //*****************************************************************
    /*
    std::cout << "RGB to YUV tests\n";
    std::cout << " RGB Interleaved to YUV444 planar (different standards) \n";
    set_meta <RGB24, YUV444>(info, 8, 2, test_rgbf);

    std::cout << "BT601-------------------------\n";
    colorspace_convert< RGB24, YUV444, BT_601> (info);
    print_planar( info );

    set_meta <RGB24, YUV444>(info, 8, 2, test_rgbf);
    std::cout << "BT709-------------------------\n";
    colorspace_convert< RGB24, YUV444, BT_709> (info);
    print_planar( info );

    set_meta <RGB24, YUV444>(info, 8, 2, test_rgbf);
    std::cout << "BT2020-------------------------\n";
    colorspace_convert< RGB24, YUV444, BT_2020> (info);
    print_planar( info);

//*****************************************************************

    std::cout << "YUV to RGB tests\n";
    std::cout << "YUV444 planar  to  RGB Interleaved  (different standards) \n";

    set_meta <YUV444, RGB24>(info, 8, 2, test_yuv444p_bt601, test_yuv444p_bt601 + 8 * 2 , test_yuv444p_bt601 + 8 * 2 + 8 * 2 );
    std::cout << "BT601-------------------------\n";
    colorspace_convert< YUV444, RGB24, BT_601> (info);
    print_interleaved( info );

    set_meta <YUV444, RGB24>(info, 8, 2, test_yuv444p_bt709, test_yuv444p_bt709 + 8 * 2 , test_yuv444p_bt709 + 8 * 2 + 8 * 2 );
    std::cout << "BT709-------------------------\n";
    colorspace_convert< YUV444, RGB24, BT_709> (info);
    print_interleaved( info );

    set_meta <YUV444, RGB24>(info, 8, 2, test_yuv444p_bt2020, test_yuv444p_bt2020 + 8 * 2 , test_yuv444p_bt2020 + 8 * 2 + 8 * 2 );
    std::cout << "BT2020-------------------------\n";
    colorspace_convert< YUV444, RGB24, BT_2020> (info);
    print_interleaved( info );

//**************************************************************************


    std::cout << "YUV422 to RGB24-------------------------\n";
    set_meta <YUV422, RGB24 >(info, 8, 2, test_yuv422p_bt601, test_yuv422p_bt601 + 8 * 2, test_yuv422p_bt601 + 8 * 2 + 4 * 2 );
    colorspace_convert<YUV422, RGB24, BT_601> (info);
    print_interleaved( info ) ;


    std::cout << "YUV420 to RGB24-------------------------\n";
    set_meta <I420, RGB24 >(info, 8, 2, test_yuv420p_bt601, test_yuv420p_bt601 + 8 * 2, test_yuv420p_bt601 + 8 * 2 + 4  );
    colorspace_convert<I420, RGB24, BT_601> (info);
    print_interleaved( info ) ;


//*****************************************************************
    std::cout << "Same formats test (RGB)\n";

    std::cout << "A2R10G10B10 interleaved to RGB24\n";
    set_meta < A2R10G10B10, RGB24>(info, 8, 2, (uint8_t*)test_A2R10G10B10_bt601);
    colorspace_convert<A2R10G10B10, RGB24, BT_601> ( info );
    print_interleaved( info );

//*****************************************************************
    std::cout << "Same formats test (YUV)\n";
    std::cout << "YUV422 to YUV444-------------------------\n";
    set_meta <YUV422, YUV444 >(info, 8, 2, test_yuv422p_bt601, test_yuv422p_bt601 + 8 * 2, test_yuv422p_bt601 + 8 * 2 + 4 * 2 );
    colorspace_convert<YUV422, YUV444, BT_601> (info);
    print_planar( info ) ;

    std::cout << "YUV420 to YUV444-------------------------\n";
    set_meta <I420, YUV444 >(info, 8, 2, test_yuv420p_bt601, test_yuv420p_bt601 + 8 * 2, test_yuv420p_bt601 + 8 * 2 + 4  );
    colorspace_convert<I420, YUV444, BT_601> (info);
    print_planar( info ) ;
//*****************************************************************
    std::cout << "Absolutely same formats test (YUV)\n";
    std::cout << "YUV444 to YUV444-------------------------\n";
    set_meta <YUV444, YUV444 >(info, 8, 2, test_yuv444p_bt601, test_yuv444p_bt601 + 8 * 2, test_yuv444p_bt601 + 8 * 2 + 8 * 2 );
    colorspace_convert<YUV444, YUV444, BT_601> (info);
    print_planar( info ) ;

    std::cout << "YUV422 to YUV422-------------------------\n";
    set_meta <YUV422, YUV422 >(info, 8, 2, test_yuv422p_bt601, test_yuv422p_bt601 + 8 * 2, test_yuv422p_bt601 + 8 * 2 + 4 * 2 );
    colorspace_convert<YUV422, YUV422, BT_601> (info);
    print_planar( info ) ;

    std::cout << "YUV420 to YUV420-------------------------\n";
    set_meta <I420, I420 >(info, 8, 2, test_yuv420p_bt601, test_yuv420p_bt601 + 8 * 2, test_yuv420p_bt601 + 8 * 2 + 4 );
    colorspace_convert<I420, I420, BT_601> (info);
    print_planar( info ) ;

//*****************************************************************
    std::cout << "Repack test\n";
    std::cout << "YUYV to UYVY-------------------------\n";
    set_meta <YUYV, UYVY >(info, 8, 2, test_yuv422i_bt601);
    colorspace_convert<YUYV, UYVY, BT_601> (info);
    print_planar( info ) ;

    std::cout << "NV12 to NV21-------------------------\n";
    set_meta <NV12, NV21 >(info, 8, 2, test_yuv420s_bt601, test_yuv420s_bt601 + 8 * 2);
    colorspace_convert<NV12, NV21, BT_601> (info);
    print_planar( info ) ;

    std::cout << "I420 to YV12-------------------------\n";
    set_meta <I420, YV12 >(info, 8, 2, test_yuv420p_bt601, test_yuv420p_bt601 + 8 * 2, test_yuv420p_bt601 + 8 * 2 + 4);
    colorspace_convert<I420, YV12, BT_601> (info);
    print_planar( info ) ;

    std::cout << "YUV422 to YV16-------------------------\n";
    set_meta <YUV422, YV16 >(info, 8, 2, test_yuv422p_bt601, test_yuv422p_bt601 + 8 * 2, test_yuv422p_bt601 + 8 * 2 + 4 * 2);
    colorspace_convert<YUV422, YV16, BT_601> (info);
    print_planar( info ) ;

//*****************************************************************
    std::cout << "P210 and P010 test\n" << std::endl;
    std::cout << "P210 to YUV444-------------------------\n";
    set_meta <P210, YUV444 >(info, 8, 2, (uint8_t*)test_P210, (uint8_t*)test_P210 + 8 * 2 * 2);
    colorspace_convert<P210, YUV444, BT_601> (info);
    print_planar( info ) ;

    std::cout << "P010 to YUV444-------------------------\n";
    set_meta <P010, YUV444 >(info, 8, 2, (uint8_t*)test_P010, (uint8_t*)test_P010 + 8 * 2 * 2);
    colorspace_convert<P010, YUV444, BT_601> ( info );
    print_planar( info ) ;

    std::cout << "Y210 to YUV444-------------------------\n";
    set_meta <Y210, YUV444 >(info, 8, 2, (uint8_t*)test_Y210);
    colorspace_convert<Y210, YUV444, BT_601> ( info );
    print_planar( info ) ;

    std::cout << "V210 to YUV444-------------------------\n";
    set_meta <V210, YUV444 >(info, 6, 2, (uint8_t*)test_V210);
    colorspace_convert<V210, YUV444, BT_601> ( info );
    print_planar( info ) ;

//*****************************************************************
    std::cout << "YUV444 to P010-------------------------\n";
    set_meta <YUV444, P010 >(info, 8, 2, test_yuv444p_to_yuv420, test_yuv444p_to_yuv420 + 8 * 2, test_yuv444p_to_yuv420 + 8 * 2 + 8 * 2 );
    colorspace_convert<YUV444, P010, BT_601> ( info );
    print_planar( info ) ;
    std::cout << "Diff is " << check( (uint8_t*)(test_P010), info.dst_data[ 0 ], 8 * 2 * 2 + 8 * 2 ) << "\n";

    std::cout << "YUV444 to P210-------------------------\n";
    set_meta <YUV444, P210 >(info, 8, 2, test_yuv444p_to_yuv422, test_yuv444p_to_yuv422 + 8 * 2, test_yuv444p_to_yuv422 + 8 * 2 + 8 * 2 );
    colorspace_convert<YUV444, P210, BT_601> ( info );
    print_planar( info ) ;
    std::cout << "Diff is " << check( (uint8_t*)(test_P210), info.dst_data[ 0 ], 8 * 2 * 2 + 8 * 2 * 2 ) << "\n";

    std::cout << "YUV444 to V210-------------------------\n";
    set_meta <YUV444, V210 >(info, 6, 2, test_yuv444p_to_V210, test_yuv444p_to_V210 + 6 * 2, test_yuv444p_to_V210 + 6 * 2 + 6 * 2 );
    colorspace_convert<YUV444, V210, BT_601> ( info );
    print_planar( info ) ;
    std::cout << "Diff is " << check( (uint8_t*)(test_V210), info.dst_data[ 0 ], 16 + 16 ) << "\n";

    std::cout << "RGB24 to A2R10G10B10 interleaved\n";
    set_meta <RGB24, A2R10G10B10>(info, 8, 2, test_RGB24_to_A2R10G10B10);
    colorspace_convert<RGB24, A2R10G10B10, BT_601> ( info );
    print_planar( info );
    std::cout << "Diff is " << check( (uint8_t*)(test_A2R10G10B10_bt601), info.dst_data[ 0 ], 8 * 4 * 2 ) << "\n";
//*****************************************************************
    std::cout << "RGB24 to RGB32 interleaved\n";
    set_meta <RGB24, RGB32 >(info, 8, 2, test_RGB24_to_A2R10G10B10);
    colorspace_convert<RGB24, RGB32, BT_601> ( info );
    print_planar( info );
    //std::cout << "Diff is " << check( (uint8_t*)(test_A2R10G10B10_bt601), info.dst_data[ 0 ], 8 * 4 * 2 ) << "\n";

//*****************************************************************


    std::cout << "Repack test\n";
    std::cout << "YUYV to RGB32-------------------------\n";
    set_meta <YUYV, RGB32 >(info, 8, 2, test_yuv422i_bt601);
    colorspace_convert<YUYV, RGB32, BT_601> (info);
    print_planar( info ) ;

*/

    std::cout << "Multiframe test\n";
    //std::cout << "RGB32 to RGB24-------------------------\n";
    //std::cout << "RGB32 to A2R10G10B10-------------------------\n";
    //std::cout << "RGB24 to NV12-------------------------\n";

    srand( time( NULL ) );
    int fnum, wid, virt_wid, hei;
    fnum = 5;
    hei = 480;
    virt_wid = 640;
    wid = virt_wid * 3;
    uint8_t* frame = (uint8_t*) malloc( wid * hei * fnum );
    for(int fr = 0; fr < fnum; ++fr)
        for( int y = 0; y < hei; ++y )
            for( int x = 0; x < wid; ++x ) {
                frame[ fr * wid * hei  + y * wid + x] = rand() % 256;
            }

    //set_meta <YUYV, YVYU >(info, virt_wid, hei * fnum, frame);
    //set_meta <YUYV, NV12 >(info, virt_wid, hei * fnum, frame);
    set_meta <RGB24, RGB32>(info, virt_wid, hei * fnum, frame);
    //set_meta <RGB24, A2R10G10B10>(info, virt_wid, hei * fnum, frame);
    //set_meta <RGB32, RGB24>(info, wid, hei * fnum, frame);
    //set_meta <RGB24, NV12>(info, wid, hei * fnum, frame);
    //set_meta <RGB32, A2R10G10B10>(info, wid, hei * fnum, frame);

    clock_t t1 = clock();
    //colorspace_convert< YUYV, YVYU, BT_601> ( info );
    //colorspace_convert< RGB24, A2R10G10B10, BT_601> ( info );
    //colorspace_convert<RGB32, RGB24, BT_601> (info);
    colorspace_convert<RGB24, RGB32, BT_601> (info);
    //colorspace_convert<RGB32, A2R10G10B10, BT_601> (info);
    //colorspace_convert<RGB24, NV12, BT_601> (info);
    clock_t t2 = clock();
    std::cout << "Time diff = " <<  t2 - t1 << std::endl;
    free( frame );

    return 0;
}
static void print_usage()
{
	std::cout << "Usage:" << std::endl;
	std::cout << "    colorspace_converter " << std::endl;
	std::cout << "    colorspace_converter yuv_file rgb_file width height" << std::endl << std::endl;
}

int main(int argc, char *argv[])
{
	if (argc == 5) {
		return fifo_test(argc, argv);
	} else if (argc == 1) {
		return syntetic_test();
	}
	print_usage();
	return -1;
}

