#include <iostream>
#include <fstream>
#include <string>
#include <jpeglib.h>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <sys/ioctl.h>
#include <unistd.h>

#define STRECH_AMOUNT 3

struct custom_JPEG_data loadJPEG(const char *);
u_char calculate_intensity(u_char r, u_char g, u_char b);

struct custom_JPEG_data {
    std::vector<u_char> * bytes;
    int width;
    int height;
    int component_size;
};

int main(int argc, char ** argv) {

    if (argc != 2 && argc != 4) {
        std::cerr <<  "Usage: ./main path/to/file.jpg [outfile.txt dimensions]" << std::endl;
        return 1;
    }
    char map[] = "@#%xo;:,.";
    
    //char map[] = "`^\",:;Il!i~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"
    std::string output;
    struct custom_JPEG_data data = loadJPEG(argv[1]);

    int patch_width, patch_height;

    if (argc == 2) {
        struct winsize wsize;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &wsize);
        patch_width = (data.width * STRECH_AMOUNT + wsize.ws_col - 1) / wsize.ws_col; // ceiling division so we will always be within bounds
        patch_height = (data.height + wsize.ws_row - 1) / wsize.ws_row;
    }

    else if (argc == 4) {
        patch_width = patch_height = std::stoi(argv[3]);
    }
    
    for (int y = 0; y < data.height; y += patch_height) {
        int patches_row_count = 0;
        for (int x = 0; x < data.width * data.component_size; x += patch_width * data.component_size) {
            int intensity = 0;
            int loop_count = 0;
            for (int j = y; j < y + patch_height; j++) {
                for (int i = x; i < x + (patch_width * data.component_size); i += data.component_size) {
                    try {
                    intensity += calculate_intensity(data.bytes->at(j * data.width * data.component_size + i), 
                                                     data.bytes->at(j * data.width * data.component_size + i+1), 
                                                     data.bytes->at(j * data.width * data.component_size + i+2));
                    loop_count++;
                    }
                    catch (const std::out_of_range& err) {
                        continue;
                    }
                }
            } 
            
            if (intensity && loop_count) {
                intensity /= loop_count; // intensity averaged over all pixels in patch
                char representation = map[intensity/31];
                for (int i = 0; i < STRECH_AMOUNT; i++) {
                    output += representation;
                }
            }
            patches_row_count += 1;
            
        }
        output += '\n';
    }

    if (argc == 3) {
        std::cout << output << std::endl;
    }
    else {
        std::ofstream outfile(argv[2]);
        outfile << output;
        outfile.close();
        std::cout << "Printed to file: " << argv[2] << std::endl;
    }
    delete data.bytes;
    return 0;
}

// Will return an empty buffer on fail
struct custom_JPEG_data loadJPEG(const char * filename) {
    u_long jpg_size;
    struct custom_JPEG_data data_struct = { 0 };
    std::FILE * infile;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    if ((infile = std::fopen(filename, "r")) == nullptr) {
        return data_struct;
    }
    jpeg_stdio_src(&cinfo, infile);

    if (jpeg_read_header(&cinfo, true) != 1) {
        return data_struct;
    }

    jpeg_start_decompress(&cinfo);
    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int pixel_size = cinfo.output_components;
    int row_width = width * pixel_size;

    u_char * raw_buffer = (u_char *) malloc(sizeof(u_char) * row_width * height);

    while (cinfo.output_scanline < cinfo.output_height) {
        u_char * singleArray[1];
        singleArray[0] = raw_buffer + cinfo.output_scanline * row_width;
        jpeg_read_scanlines(&cinfo, singleArray, 1);
    }
     std::vector<u_char> * buffer = new std::vector<u_char>(raw_buffer, raw_buffer + row_width * height);

    
    free(raw_buffer);
    jpeg_destroy_decompress(&cinfo);
    std::fclose(infile);

    data_struct.bytes = buffer;
    data_struct.component_size = pixel_size;
    data_struct.height = height;
    data_struct.width = width;

    return data_struct;

}

u_char calculate_intensity(u_char r, u_char g, u_char b) {
    float intensity = (0.2126*(float)r + 0.7152*(float)g + 0.0722*(float)b);
    return (u_char)intensity;
}