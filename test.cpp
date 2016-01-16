//#define ENABLE_LOG


#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include "color_convert.hpp"



using namespace ColorspaceConverter;

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



template <Colorspace from_cs, Colorspace to_cs> void set_meta( ConvertMeta& meta , int width, int height, uint8_t* src_a, uint8_t* src_b = nullptr,uint8_t* src_c = nullptr)
{
    meta.width = width;
    meta.height = height;
    meta.src_data[ 0 ] = src_a;
    meta.src_data[ 1 ] = src_b;
    meta.src_data[ 2 ] = src_c;
    init_offset_yuv<from_cs, to_cs >( meta );
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
        meta.src_stride[ 0 ] = width * 4;
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
        meta.src_stride[ 0 ] = width * 16 / 6;
    }

    //destination settings

    if(to_cs == RGB24)
    {
        meta.dst_stride_horiz[ 0 ] = width * 3;
        meta.dst_stride_vert[0] = height;

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


static int syntetic_test()
{
// p = planar
// s = semiplanar
// i = interleaved
     static uint8_t test_yuv422i_bt601[(8 * 2) * 2] =
	{
		235,128,    235,128,   81,90,   81,240,   145,54,   145,34,   41,240,  41,110,
        16,128,     16,128,    210,16,  210,146,   170,166,  170,16,  106,202, 106,222
	};
	 static uint8_t test_yuv444s_bt601[(8 * 2) + 8 * 2 * 2] =
	{
        235, 81, 145, 41, 16, 210, 170, 106,
        107, 157, 74, 178, 94, 147, 115, 95,

        128, 128,   90, 240,    54, 34,     240, 110,   128,128,    16,146,     166,16,     202,222,
        202,62,     111,25,     221,167,    34,92,      147,231,    52,192,     71,137,     147,71

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
        pack_A2R10G10B10(0x3FF, 0x3FF, 0x3FF),    pack_A2R10G10B10(0x3FF, 0,0),      pack_A2R10G10B10(0, 0x3FF, 0),     pack_A2R10G10B10(0, 0, 0x3FF),
        pack_A2R10G10B10(0x3FF, 0x3FF, 0),        pack_A2R10G10B10(0, 0x3FF, 0x3FF), pack_A2R10G10B10(0x3FF, 0, 0x3FF), pack_A2R10G10B10(0x3FF, 0x3FF, 0x3FF),
        pack_A2R10G10B10(0x3FF, 0x3FF, 0x3FF),    pack_A2R10G10B10(0x3FF, 0,0),      pack_A2R10G10B10(0, 0x3FF, 0),     pack_A2R10G10B10(0, 0, 0x3FF),
        pack_A2R10G10B10(0x3FF, 0x3FF, 0),        pack_A2R10G10B10(0, 0x3FF, 0x3FF), pack_A2R10G10B10(0x3FF, 0, 0x3FF), pack_A2R10G10B10(0x3FF, 0x3FF, 0x3FF),
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


    static uint8_t test_yuv444i_bt601[(8 * 2) * 3] =
	{
		235, 128, 128,      81, 90, 240,     145, 54, 34,    41, 240, 110,   16,128,128,    210,16,146,     170,166,16,      106,202,222,
        107,202,62,         157,111,25,      74,221,167,     178,34,92,      94,147,231,    147,52,192,     115,71,137,      95,147,71
	};

    static uint8_t test_rgb_to_yuv422[(8 * 2) * 3] =
    {
        255,255,255,  255,255,255,  255,0,0,    255,0,0,    0,255,0,   0,255,0,    0,0,255,     0,0,255,
        0,0,0,     0,0,0,     255,255,0,   255,255,0,   0,255,255,  0,255,255,   255,0,255,    255,0,255
    };

    static uint8_t test_rgb_to_yuv420[(8 * 2) * 3] =
	{
        255,255,255,  255,255,255,  255,0,0,    255,0,0,    0,255,0,   0,255,0,    0,0,255,     0,0,255,
        255,255,255,  255,255,255,  255,0,0,    255,0,0,    0,255,0,   0,255,0,    0,0,255,     0,0,255
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

    static uint8_t test_rgbn[(8 * 2) * 3] =
	{
        235,235,235,  235,16,16,  16,235,16,   16,16,235,  16,16,16,    235,235,16,  16,235,235,  235,16,235,
        16,128,235,   16,235,128, 128,16,235,  128,235,16, 235,16,128,  235,128,16,  128,128,16,  16,128,128
    };
    static uint8_t test_rgbf[(8 * 2) * 3] =
    {
        255,255,255,  255,0,0,  0,255,0,   0,0,255,  0,0,0,    255,255,0,  0,255,255,  255,0,255,
        0,128,255,   0,255,128, 128,0,255,  128,255,0, 255,0,128,  255,128,0,  128,128,0,  0,128,128
    };

    static uint8_t test_rgb24_to_A2R10G10B10[(8 * 2) * 3] =
    {
        255,255,255,   255,0,0,   0,255,0,   0,0,255,   255,255,0,   0,255,255,   255,0,255,   255,255,255,
        255,255,255,   255,0,0,   0,255,0,   0,0,255,   255,255,0,   0,255,255,   255,0,255,   255,255,255
    };
    static uint16_t test_P210[ 8 * 2 + 4 * 2 + 4 * 2 ]
    {
        940 << 6, 940 << 6, 324 << 6, 324 << 6, 580 << 6, 580 << 6, 164 << 6, 164 << 6,
        64 << 6, 64 << 6, 840 << 6, 840 << 6, 680 << 6, 680 << 6, 424 << 6, 424 << 6,

        512 << 6, 512 << 6, 360 << 6, 960 << 6, 216 << 6, 136 << 6, 960 << 6, 440 << 6,
        512 << 6, 512 << 6, 64 << 6, 584 << 6, 664 << 6, 64 << 6, 808 << 6, 888 << 6,
    };
    static uint16_t test_P010[ 8 * 2 + 4 + 4 ]
    {
        940 << 6, 940 << 6, 324 << 6, 324 << 6, 580 << 6, 580 << 6, 164 << 6, 164 << 6,
        940 << 6, 940 << 6, 324 << 6, 324 << 6, 580 << 6, 580 << 6, 164 << 6, 164 << 6,

        512 << 6, 512 << 6, 360 << 6, 960 << 6, 216 << 6, 136 << 6, 960 << 6, 440 << 6,
    };


    ConvertMeta info;
    //*****************************************************************

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


//**************************************************************************

    std::cout << "YUV444 to RGB24-------------------------\n";
    set_meta <YUV444, RGB24 >(info, 8, 2, test_yuv444p_bt601, test_yuv444p_bt601 + 8 * 2, test_yuv444p_bt601 + 8 * 2 + 8 * 2 );
    colorspace_convert<YUV444, RGB24, BT_601> (info);
    print_interleaved( info ) ;

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
    std::cout << "P210 and P010 test\n";
    std::cout << "P210 to YUV444-------------------------\n";
    set_meta <P210, YUV444 >(info, 8, 2, (uint8_t*)test_P210, (uint8_t*)test_P210 + 8 * 2 * 2, (uint8_t*)test_P210 + 8 * 2 * 2 + 4 * 2 * 2);
    colorspace_convert<P210, YUV444, BT_601> (info);
    print_planar( info ) ;

    std::cout << "P010 to YUV444-------------------------\n";
    set_meta <P010, YUV444 >(info, 8, 2, (uint8_t*)test_P010, (uint8_t*)test_P010 + 8 * 2 * 2, (uint8_t*)test_P010 + 8 * 2 * 2 + 4 * 2);
    colorspace_convert<P010, YUV444, BT_601> ( info );
    print_planar( info ) ;


    /*
    std::cout << "YUV422 to P210-------------------------\n";
    set_meta <YUV422, P210 >(info, 8, 2, test_yuv422p_bt601, test_yuv422p_bt601 + 8 * 2, test_yuv422p_bt601 + 8 * 2 + 4 * 2);
    colorspace_convert<YUV422, P210, BT_601> (info);
    print_planar( info ) ;


   /*
    std::cout << "YUV444 interleaved to planar FULL_RANGE\n";
    info.src_stride[0] = 8 * 3;
    info.src_stride[1] = 0;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8;
    info.dst_stride[1] = 8;
    info.dst_stride[2] = 8;

	info.src_data[0] = test_yuv444i_bt601;
	info.dst_data[0] = result;
	info.dst_data[1] = result +  8 * 2;
	info.dst_data[2] = result +  8 * 2 + 8 * 2;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<YUV444, Interleaved, NORM_RANGE, YUV444, Planar, FULL_RANGE, BT_601> (info);
	print_yuv(result);

    std::cout << "YUV444 interleaved to interleaved\n";
    info.src_stride[0] = 8 * 3;
    info.src_stride[1] = 0;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8 * 3;
    info.dst_stride[1] = 8;
    info.dst_stride[2] = 8;

	info.src_data[0] = test_yuv444i_bt601;
	info.dst_data[0] = result;
	info.dst_data[1] = result;
	info.dst_data[2] = result;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<YUV444, Interleaved, NORM_RANGE , YUV444, Interleaved, NORM_RANGE, BT_601> (info);
	print_yuv(result);

    */
    //*****************************************************************
    /*
    std::cout << "Packing test\n";

    std::cout << "RGB24 interleaved to YUV444 planar\n";
    info.src_stride[0] = 8 * 3;
    info.src_stride[1] = 0;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8;
    info.dst_stride[1] = 8;
    info.dst_stride[2] = 8;

    info.src_data[0] = test_rgbf;
    info.dst_data[0] = result;
    info.dst_data[1] = result + 8 * 2;
    info.dst_data[2] = result + 8 * 2 + 8 * 2;

    memset(result, 0xff, sizeof(result));
    colorspace_convert<RGB24, YUV444, BT_601> (info);
    print_yuv(result);

    //*****************************************************************
    std::cout << "RGB24 interleaved to YUV422 interleaved\n";
    info.src_stride[0] = 8 * 3;
    info.src_stride[1] = 0;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8;
    info.dst_stride[1] = 4;
    info.dst_stride[2] = 4;

    info.src_data[0] = test_rgb_to_yuv422;
	info.dst_data[0] = result;
    info.dst_data[1] = result + 8 * 2;
    info.dst_data[2] = result + 8 * 2 + 4 * 2;

	memset(result, 0xff, sizeof(result));
    colorspace_convert<RGB24, YUV422, BT_601> (info);
	print_yuv(result);
    //*****************************************************************
    std::cout << "Packing test\n";
    std::cout << "RGB24 interleaved to YUV420 planar\n";
    info.src_stride[0] = 8 * 3;
    info.src_stride[1] = 0;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8;
    info.dst_stride[1] = 4;
    info.dst_stride[2] = 4;

    info.src_data[0] = test_rgb_to_yuv420;
	info.dst_data[0] = result;
	info.dst_data[1] = result + 8 * 2;
	info.dst_data[2] = result + 8 * 2 + 4;

	memset(result, 0xff, sizeof(result));
    colorspace_convert<RGB24, I420, BT_601> (info);
	print_yuv(result);
    //*****************************************************************
    std::cout << "Semiplanar test\n";
    std::cout << "Semiplanar YUV420 to planar yuv444\n";
    //y y
    //y y
    //u v

    info.src_stride[0] = 8;
    info.src_stride[1] = 8;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8;
    info.dst_stride[1] = 8;
    info.dst_stride[2] = 8;

    info.src_data[0] = test_yuv420s_bt601;
    info.src_data[1] = test_yuv420s_bt601 + 8 * 2;

    info.dst_data[0] = result;
    info.dst_data[1] = result + 8 * 2;
    info.dst_data[2] = result + 8 * 2 + 8 * 2;

    memset(result, 0xff, sizeof(result));
    colorspace_convert<NV12, YUV444, BT_601> (info);
    print_yuv(result);
    */
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

