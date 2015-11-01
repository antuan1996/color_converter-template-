#define ENABLE_LOG


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
		info.dst_stride[i] = stride * 3; // RGB
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
    colorspace_convert<YUV444, Planar, NORM_RANGE, RGB, Interleaved, FULL_RANGE, BT_601> (info);

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

static void print_rgb(const uint8_t *rgb)
{
	printf("RGB:\n");
	for (size_t y = 0; y < 2; y++) {
		for (size_t x = 0; x < 8; x++) {
			printf("%d,%d,%d   ",
				   rgb[0 + (x + y * 8) * 3],
				   rgb[1 + (x + y * 8) * 3],
				   rgb[2 + (x + y * 8) * 3]);
		}
		printf("\n");
	}
	printf("\n");
}


static void print_yuv(const uint8_t *yuv)
{
	printf("YUV:\n");
	for (size_t plane = 0; plane < 3; plane++) {
		for (size_t y = 0; y < 2; y++) {
			for (size_t x = 0; x < 8; x++) {
				printf("%3d, ", *yuv++);
			}
			printf("\n");
		}
		printf("\n");
	}
	printf("\n");
}


int check( uint8_t* a, uint8_t* b, int width, int height ) {
    int res = 0;
    for( size_t y = 0; y < uint(height); ++y )
        for( size_t x = 0 ; x < uint(width) ; ++x ) {
            res += abs( a[y * width + x] - b[y * width + x] );
        }
    return res;
}


static int syntetic_test()
{
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
    static uint8_t test_A2R10G10B10_bt601[(8 * 2) * 4] =
	{
        0xFF,0xFF,0xFF,0xFF,    0xFF, 0xF, 0, 0,     3,0xF0, 0x3F,0,     3, 0, 0xC0, 0xFF,    0xFF,0xFF,0xFF,0xFF,    0xFF, 0xF, 0, 0,     3,0xF0, 0x3F,0,     3, 0, 0xC0, 0xFF,
        0xFF,0xFF,0xFF,0xFF,    0xFF, 0xF, 0, 0,     3,0xF0, 0x3F,0,     3, 0, 0xC0, 0xFF,    0xFF,0xFF,0xFF,0xFF,    0xFF, 0xF, 0, 0,     3,0xF0, 0x3F,0,     3, 0, 0xC0, 0xFF
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


    static uint8_t test_twisergb[(8 * 2) * 3] =
	{
        235,235,235,  235,235,235,  235,16,16,    235,16,16,    16,235,16,   16,235,16,    16,16,235,     16,16,235,
        16,16,16,     16,16,16,     235,235,16,   235,235,16,   16,235,235,  16,235,235,   235,16,235,    235,16,235
    };
    static uint8_t test_rgb_yuv420[(8 * 2) * 3] =
	{
        235,235,235,  235,235,235,  235,16,16,    235,16,16,    16,235,16,   16,235,16,    16,16,235,     16,16,235,
        235,235,235,  235,235,235,  235,16,16,    235,16,16,    16,235,16,   16,235,16,    16,16,235,     16,16,235
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
	static uint8_t test_rgb[(8 * 2) * 3] =
	{
        235,235,235,  235,16,16,  16,235,16,   16,16,235,  16,16,16,    235,235,16,  16,235,235,  235,16,235,
        16,128,235,   16,235,128, 128,16,235,  128,235,16, 235,16,128,  235,128,16,  128,128,16,  16,128,128
    };
    static uint8_t test_rgb24_to_A2R10G10B10[(8 * 2) * 3] =
    {
        234,234,234,   234,16,16,   16,234,16,   16,16,234,   234,234,234,   234,16,16,   16,234,16,   16,16,234,
        234,234,234,   234,16,16,   16,234,16,   16,16,234,   234,234,234,   234,16,16,   16,234,16,   16,16,234
    };
	static uint8_t result[(8 * 2) * 3];
	std::cout << "RGB to YUV tests\n";

	std::cout << " RGB Interleaved to YUV444 planar (different standards) \n";
	ConvertMeta info;
	info.width = 8;
	info.height = 2;
	size_t stride = 8;

    info.src_stride[0] = 8 * 3;
    info.src_stride[1] = 0;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8;
    info.dst_stride[1] = 8;
    info.dst_stride[2] = 8;

	info.src_data[0] = test_rgb;
	info.src_data[1] = nullptr;
	info.src_data[2] = nullptr;
	info.dst_data[0] = result;
	info.dst_data[1] = result + 2 * 8;
	info.dst_data[2] = result + 2 * 8 + 2 * 8;

	std::cout << "BT601-------------------------\n";
	memset(result, 0xff, sizeof(result));
	colorspace_convert< RGB, Interleaved, NORM_RANGE, YUV444, Planar, NORM_RANGE, BT_601> (info);
	print_yuv(result);

	std::cout << "BT709-------------------------\n";
	memset(result, 0xff, sizeof(result));
	colorspace_convert< RGB, Interleaved, NORM_RANGE, YUV444, Planar, NORM_RANGE, BT_709> (info);
    print_yuv(result);

	std::cout << "BT2020-------------------------\n";
	memset(result, 0xff, sizeof(result));
	colorspace_convert< RGB, Interleaved, NORM_RANGE, YUV444, Planar, NORM_RANGE, BT_2020> (info);
    print_yuv(result);

//**************************************************************************
	std::cout << "YUV(444, 422 , 420) planar to RGB interleaved\n";

    std::cout << "YUV444-------------------------\n";
    info.src_stride[0] = 8;
    info.src_stride[1] = 8;
    info.src_stride[2] = 8;

    info.dst_stride[0] = 8 * 3;
    info.dst_stride[1] = 8;
    info.dst_stride[2] = 8;

	info.src_data[0] = test_yuv444p_bt601;
	info.src_data[1] = test_yuv444p_bt601 + (8 * 2);
	info.src_data[2] = test_yuv444p_bt601 + (8 * 2) + (8 * 2);
	info.dst_data[0] = result;
	info.dst_data[1] = nullptr;
	info.dst_data[2] = nullptr;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<YUV444, Planar, NORM_RANGE, RGB, Interleaved, NORM_RANGE, BT_601> (info);
	print_rgb(result);


    std::cout << "YUV422-------------------------\n";
    info.src_stride[0] = 8;
    info.src_stride[1] = 4;
    info.src_stride[2] = 4;

    info.dst_stride[0] = 8 * 3;
    info.dst_stride[1] = 8;
    info.dst_stride[2] = 8;

	info.src_data[0] = test_yuv422p_bt601;
	info.src_data[1] = test_yuv422p_bt601 + (8 * 2);
	info.src_data[2] = test_yuv422p_bt601 + (8 * 2) + (4 * 2);
	info.dst_data[0] = result;
	info.dst_data[1] = result + 2 * 8;
	info.dst_data[2] = result + 2 * 8 + 2 * 8;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<YUV422, Planar, NORM_RANGE, RGB, Interleaved, NORM_RANGE, BT_601> (info);
	print_rgb(result);

    std::cout << "YUV420-------------------------\n";
    info.src_stride[0] = 8;
    info.src_stride[1] = 4;
    info.src_stride[2] = 4;

    info.dst_stride[0] = 8 * 3;
    info.dst_stride[1] = 8;
    info.dst_stride[2] = 8;

	info.src_data[0] = test_yuv420p_bt601;
	info.src_data[1] = test_yuv420p_bt601 + (8 * 2);
	info.src_data[2] = test_yuv420p_bt601 + (8 * 2) + 4;
	info.dst_data[0] = result;
	info.dst_data[1] = result + 2 * 8;
	info.dst_data[2] = result + 2 * 8 + 2 * 8;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<YUV420, Planar, NORM_RANGE, RGB, Interleaved, NORM_RANGE,  BT_601> (info);
	print_rgb(result);

//*****************************************************************
	std::cout << "YUV(444, 422) interleaved to RGB interleaved\n";

    std::cout << "YUV444-------------------------\n";
    info.src_stride[0] = 8 * 3;
    info.src_stride[1] = 0;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8 * 3;
    info.dst_stride[1] = 0;
    info.dst_stride[2] = 0;

	info.src_data[0] = test_yuv444i_bt601;
	info.dst_data[0] = result;
	info.dst_data[1] = nullptr;
	info.dst_data[2] = nullptr;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<YUV444, Interleaved, NORM_RANGE, RGB, Interleaved, NORM_RANGE, BT_601> (info);
	print_rgb(result);


    std::cout << "YUV422-------------------------\n";
    info.src_stride[0] = 8 * 2;
    info.src_stride[1] = 0;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8 * 3;
    info.dst_stride[1] = 0;
    info.dst_stride[2] = 0;

	info.src_data[0] = test_yuv422i_bt601;
	info.src_data[1] = nullptr;
	info.src_data[2] = nullptr;

	info.dst_data[0] = result;
	info.dst_data[1] = nullptr;
	info.dst_data[2] = nullptr;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<YUV422, Interleaved, NORM_RANGE, RGB, Interleaved, NORM_RANGE, BT_601> (info);
	print_rgb(result);

//*****************************************************************
	std::cout << "A2R10G10B10 interleaved to YUV444 planar\n";

    info.src_stride[0] = 8 * 4;
    info.src_stride[1] = 0;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8;
    info.dst_stride[1] = 8;
    info.dst_stride[2] = 8;

	info.src_data[0] = test_A2R10G10B10_bt601;
	info.dst_data[0] = result;
	info.dst_data[1] = result +  8 * 2;
	info.dst_data[2] = result +  8 * 2 + 8 * 2;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<A2R10G10B10, Interleaved, FULL_RANGE, YUV444, Planar, NORM_RANGE, BT_601> (info);
	print_yuv(result);

//*****************************************************************
	std::cout << "A2R10G10B10 interleaved to RGB Ineteleaved\n";

    info.src_stride[0] = 8 * 4;
    info.src_stride[1] = 0;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8 * 3;
    info.dst_stride[1] = 0;
    info.dst_stride[2] = 0;

	info.src_data[0] = test_A2R10G10B10_bt601;
	info.dst_data[0] = result;
	info.dst_data[1] = nullptr;
	info.dst_data[2] = nullptr;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<A2R10G10B10, Interleaved, FULL_RANGE, RGB, Interleaved, NORM_RANGE, BT_601> (info);
	print_rgb(result);

    //*****************************************************************
	std::cout << "A2R10G10B10 interleaved to YUV444 planar\n";

    info.src_stride[0] = 8 * 4;
    info.src_stride[1] = 0;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8;
    info.dst_stride[1] = 8;
    info.dst_stride[2] = 8;

	info.src_data[0] = test_A2R10G10B10_bt601;
	info.dst_data[0] = result;
	info.dst_data[1] = result +  8 * 2;
	info.dst_data[2] = result +  8 * 2 + 8 * 2;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<A2R10G10B10, Interleaved, FULL_RANGE, YUV444, Planar, NORM_RANGE, BT_601> (info);
	print_yuv(result);

    //*****************************************************************

    std::cout << "Same formats test\n";

    std::cout << "YUV444 interleaved to planar";
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
	colorspace_convert<YUV444, Interleaved, NORM_RANGE, YUV444, Planar, NORM_RANGE, BT_601> (info);
	print_yuv(result);

    std::cout << "YUV444 interleaved to interleaved";
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


    //*****************************************************************

    std::cout << "Packing test\n";
    std::cout << "RGB24 interleaved to YUV422 interleaved";
    info.src_stride[0] = 8 * 3;
    info.src_stride[1] = 0;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8 * 2;
    info.dst_stride[1] = 0;
    info.dst_stride[2] = 0;

	info.src_data[0] = test_rgb;
	info.dst_data[0] = result;
	info.dst_data[1] = nullptr;
	info.dst_data[2] = nullptr;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<RGB, Interleaved, NORM_RANGE, YUV422, Interleaved, NORM_RANGE, BT_601> (info);
	print_yuv(result);
    //*****************************************************************
    std::cout << "Packing test\n";
    std::cout << "RGB24 interleaved to YUV420 planar";
    info.src_stride[0] = 8 * 3;
    info.src_stride[1] = 0;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8;
    info.dst_stride[1] = 4;
    info.dst_stride[2] = 4;

	info.src_data[0] = test_rgb_yuv420;
	info.dst_data[0] = result;
	info.dst_data[1] = result + 8 * 2;
	info.dst_data[2] = result + 8 * 2 + 4;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<RGB, Interleaved, NORM_RANGE, YUV420, Planar, NORM_RANGE, BT_601> (info);
	print_yuv(result);
    //*****************************************************************
    std::cout << "Semiplanar test\n";
    std::cout << "YUV444 semiplanar to planar";
    info.src_stride[0] = 8;
    info.src_stride[1] = 8 * 2;
    info.src_stride[2] = 0;

    info.dst_stride[0] = 8;
    info.dst_stride[1] = 8;
    info.dst_stride[2] = 8;

	info.src_data[0] = test_yuv444s_bt601;
	info.src_data[1] = test_yuv444s_bt601 + 8 * 2;


	info.dst_data[0] = result;
	info.dst_data[1] = result + 8 * 2;
	info.dst_data[2] = result + 8 * 2 + 8 * 2;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<YUV444, SemiPlanar, NORM_RANGE, YUV444, Planar, NORM_RANGE, BT_601> (info);
	print_yuv(result);
    //*****************************************************************
    std::cout << "YUV444 planar to semiplanar";
    info.src_stride[0] = 8;
    info.src_stride[1] = 8;
    info.src_stride[2] = 8;

    info.dst_stride[0] = 8;
    info.dst_stride[1] = 8 * 2;

	info.src_data[0] = test_yuv444p_bt601;
	info.src_data[1] = test_yuv444p_bt601 + 8 * 2;
	info.src_data[2] = test_yuv444p_bt601 + 8 * 2 + 8 * 2;

	info.dst_data[0] = result;
	info.dst_data[1] = result + 8 * 2;

	memset(result, 0xff, sizeof(result));
	colorspace_convert<YUV444, Planar, NORM_RANGE, YUV444, SemiPlanar, NORM_RANGE, BT_601> (info);
	print_yuv(result);


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

