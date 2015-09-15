//#define ENABLE_LOG

#include "color_convert.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <memory>

using namespace ColorspaceConverter;

static int fifo_test(int argc, char *argv[])
{
#ifdef ENABLE_LOG
	system("pwd"); // non-portable
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
	
    std::cout << "Getting memory" << std::endl;
	std::shared_ptr <uint8_t> input_buffer  = std::shared_ptr<uint8_t> (new uint8_t[frame_size * 3], std::default_delete<uint8_t> ());
	std::shared_ptr <uint8_t> output_buffer = std::shared_ptr<uint8_t> (new uint8_t[frame_size * 3], std::default_delete<uint8_t> ());
	
	// Planar YUV444
	info.src_data[0] = input_buffer.get();
    info.src_data[1] = info.src_data[0] + frame_size;
    info.src_data[2] = info.src_data[1] + frame_size;

	// Interleaved RGB24
    info.dst_data[0] = output_buffer.get();
    info.dst_data[1] = NULL;
    info.dst_data[2] = NULL;
	
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
    colorspace_convert<YUV444, Planar, RGB, Interleaved, BT_601> (info);

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

static int syntetic_test()
{
	static uint8_t test_YUV444[(8 * 2) * 3] =
	{
		// Y
		235, 210, 169, 144, 106,  81,  40,  16,
		235, 210, 169, 144, 106,  81,  40,  16,

		// Cb
		127,  16, 165,  53, 202,  90, 239, 128,
		127,  16, 165,  53, 202,  90, 239, 128,

		// Cr
		127, 146,  16,  34, 221, 239, 109, 128,
		127, 146,  16,  34, 221, 239, 109, 128,
	};
	
	static uint8_t test_rgb[(8 * 2) * 3] =
	{
		255,255,255,   255,255,0,   0,255,255, 0,255,0,   255,0,255, 255,0,0,   0,0,255, 0,0,0,
		255,255,255,   255,255,0,   0,255,255, 0,255,0,   255,0,255, 255,0,0,   0,0,255, 0,0,0
	};
	
	static uint8_t result[(8 * 2) * 3];
	
	ConvertMeta info;
	info.width = 8;
	info.height = 2;
	size_t stride = 8;
	
	for (size_t plane = 0; plane < 3; plane++) {
		info.src_stride[plane] = stride;
		info.dst_stride[plane] = stride * 3;
	}
	info.src_data[0] = test_YUV444;
	info.src_data[1] = test_YUV444 + 8 * 2;
	info.src_data[2] = test_YUV444 + 8 * 2 + 8 * 2;
	info.dst_data[0] = result;
	info.dst_data[1] = NULL;
	info.dst_data[2] = NULL;
	memset(result, 0xff, sizeof(result));
	colorspace_convert<YUV444, Planar, RGB, Interleaved, BT_601> (info);
	print_rgb(result);
	
	for (size_t plane = 0; plane < 3; plane++) {
		info.src_stride[plane] = stride * 3;
		info.dst_stride[plane] = stride;
	}
	info.src_data[0] = test_rgb;
	info.src_data[1] = NULL;
	info.src_data[2] = NULL;
	info.dst_data[0] = result;
	info.dst_data[1] = result + 8 * 2;
	info.dst_data[2] = result + 8 * 2 + 8 * 2;
	memset(result, 0xff, sizeof(result));
	colorspace_convert<RGB, Interleaved, YUV444, Planar, BT_601> (info);
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

