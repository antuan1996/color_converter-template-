#include "color_convert.hpp"

int main(){
    freopen("report.txt","w",stdout);
    system("pwd");
    Convert_meta test;
    test.width = 176;
    test.height = 144;
    int frame_size = test.width * test.height * 3;
    std::cout << "Getting memory" << std::endl;
    test.src_data = (uint8_t* )malloc(frame_size);
    if (test.src_data == nullptr) return 1;
    test.dst_data = (uint8_t* )malloc(frame_size);
    if (test.dst_data == nullptr) return 1;

    std::cout << "Reading data" << std::endl;
    FILE* f_in = fopen("line.rgb", "rb");
    fread(test.src_data, frame_size, 1, f_in);
    fclose(f_in);
    std::cout << "I'm ready" << std::endl;
    convert <RGB, Interleaved, YUV444, Planar, BT_601>(test);

    std::cout << "Writng results" << std::endl;
    FILE* f_out = fopen("result.yuv", "wb");
    fwrite(test.dst_data, frame_size, 1, f_out);
    fclose(f_out);

    std::cout << "Exit" << std::endl;
    free(test.src_data);
    free(test.dst_data);
    return 0;
}
