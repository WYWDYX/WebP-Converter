#include <iostream>
#include <fstream>
#include <vector>
#include <png.h>
#include <webp/decode.h>
#include <filesystem>

bool convert_webp_to_png(const std::filesystem::path& input_file, const std::filesystem::path& output_file) {
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

    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        std::cerr << "创建PNG结构出错" << std::endl;
        fclose(output);
        WebPFree(rgba_data);
        return false;
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        std::cerr << "创建PNG信息结构出错" << std::endl;
        png_destroy_write_struct(&png_ptr, NULL);
        fclose(output);
        WebPFree(rgba_data);
        return false;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        std::cerr << "PNG初始化I/O出错" << std::endl;
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(output);
        WebPFree(rgba_data);
        return false;
    }

    png_init_io(png_ptr, output);
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);

    for (int y = 0; y < height; ++y) {
        png_write_row(png_ptr, &rgba_data[y * width * 4]);
    }

    png_write_end(png_ptr, NULL);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(output);
    WebPFree(rgba_data);

    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "用法: " << argv[0] << " <输入WebP文件夹路径> <输出PNG文件夹路径>" << std::endl;
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
            std::filesystem::path output_file = output_dir / (entry.path().stem().string() + ".png");
            if (convert_webp_to_png(entry.path(), output_file)) {
                std::cout << "转换成功: " << entry.path() << " -> " << output_file << std::endl;
            } else {
                std::cerr << "转换失败: " << entry.path() << " -> " << output_file << std::endl;
            }
        }
    }

    return 0;
}