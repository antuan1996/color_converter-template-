#include "color_convert.hpp"
int main(){
    freopen("report.txt","w",stdout);
    system("pwd");
    Convert_meta test;
    test.width = 176;
    test.height = 144;
    int frame_size = test.width * test.height;
    std::cout << "Getting memory" << std::endl;

    // interleaved
    test.src_data[0] = (uint8_t* )malloc(frame_size * 3);
    if (test.src_data == nullptr) return 1;
    test.src_data[1] = test.src_data[0] + frame_size;
    test.src_data[2] = test.src_data[0] + frame_size * 2;

    test.dst_data[0] = (uint8_t* )malloc(frame_size * 3);
    if (test.dst_data == nullptr) return 1;
    test.dst_data[1] = test.dst_data[0] + frame_size;
    test.dst_data[2] = test.dst_data[0] + 2 * frame_size;

    for(int i = 0; i < 3; ++i){
        test.src_stride[i] = 0;
        test.dst_stride[i] = 0;
    }

    std::cout << "Reading data" << std::endl;
    FILE* f_in = fopen("result.yuv", "rb");
    fread(test.src_data[0], frame_size * 3, 1, f_in);
    fclose(f_in);

    std::cout << "I'm ready" << std::endl;
    convert <YUV444, Planar, RGB, Planar, BT_601>(test);

    std::cout << "Writng results" << std::endl;
    FILE* f_out = fopen("result.rgb", "wb");
    fwrite(test.dst_data[0], frame_size * 3, 1, f_out);
    fclose(f_out);

    std::cout << "Exit" << std::endl;
    free(test.src_data[0]);
    free(test.dst_data[0]);
    return 0;
}
