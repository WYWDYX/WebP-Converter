#include <iostream>
#include <fstream>
#include <vector>
#include <jpeglib.h>
#include <webp/decode.h>
#include <filesystem>

bool convert_webp_to_jpeg(const std::filesystem::path& input_file, const std::filesystem::path& output_file) {
    std::ifstream file(input_file, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "打开输入文件夹出错: " << input_file << std::endl;
        return false;
    }

    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(file_size);

    if (!file.read(buffer.data(), file_size)) {
        std::cerr << "读取输入文件出错: " << input_file << std::endl;
        return false;
    }

    file.close();

    int width, height;
    uint8_t* rgba_data = WebPDecodeRGBA(reinterpret_cast<const uint8_t*>(buffer.data()), file_size, &width, &height);

    if (rgba_data == nullptr) {
        std::cerr << "WebP解码出错: " << input_file << std::endl;
        return false;
    }

    FILE* output = fopen(output_file.string().c_str(), "wb");
    if (!output) {
        std::cerr << "打开输出文件夹出错: " << output_file << std::endl;
        WebPFree(rgba_data);
        return false;
    }

    jpeg_compress_struct cinfo;
    jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, output);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    JSAMPROW row_pointer[1];

    while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPLE row[cinfo.image_width * cinfo.input_components];
        for (int i = 0; i < cinfo.image_width; i++) {
            row[i * cinfo.input_components + 0] = rgba_data[cinfo.next_scanline * width * 4 + i * 4 + 0];
            row[i * cinfo.input_components + 1] = rgba_data[cinfo.next_scanline * width * 4 + i * 4 + 1];
            row[i * cinfo.input_components + 2] = rgba_data[cinfo.next_scanline * width * 4 + i * 4 + 2];
        }
        row_pointer[0] = row;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(output);
    WebPFree(rgba_data);

    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "用法: " << argv[0] << " <输入WebP文件夹路径> <输出JPEG文件夹路径>" << std::endl;
        return 1;
    }

    std::filesystem::path input_dir(argv[1]);
    std::filesystem::path output_dir(argv[2]);

    if (!std::filesystem::is_directory(input_dir)) {
        std::cerr << "输入路径不是文件夹" << std::endl;
        return 1;
    }

    if (!std::filesystem::exists(output_dir) && !std::filesystem::create_directory(output_dir)) {
        std::cerr << "创建输出文件夹失败" << std::endl;
        return 1;
    }

    for (const auto& entry : std::filesystem::directory_iterator(input_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".webp") {
            std::filesystem::path output_file = output_dir / (entry.path().stem().string() + ".jpg");
            if (convert_webp_to_jpeg(entry.path(), output_file)) {
                std::cout << "转换成功: " << entry.path() << " -> " << output_file << std::endl;
            } else {
                std::cerr << "转换失败: " << entry.path() << " -> " << output_file << std::endl;
            }
        }
    }

    return 0;
}