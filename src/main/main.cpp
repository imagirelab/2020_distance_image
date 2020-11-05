//#define NOMINMAX
//#include <windows.h>
#include <assert.h>

// leak chack!
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

// stl ライブラリ
#include <string>
#include <iostream>
#include <filesystem> // filesystem
#include <algorithm>  // clamp

// stb ライブラリ
#define STB_IMAGE_IMPLEMENTATION
#include "../imports/stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../imports/stb/stb_image_write.h"

// プロジェクトヘッダ
#include "tpot_image.h"
#include "convert.h"


// ----------------------------------------
// 画像管理


class image
{
private:
    int width_ = 0, height_ = 0;
    tpot::color_buffer_uc buf_i_;
    tpot::color_buffer_f buf_f_;

    void generate_float_buffer()
    {
        if (width_ < 1 || height_ < 1) { std::cout << "Error:image size is zero!" << std::endl; return; }

        if (!buf_f_.Initialize(width_, height_)) { std::cout << "Error: cannot allocate an inner buffer!" << std::endl; return; }

        buf_i_.to_f(buf_f_);
    }

public:
    image() {}
    ~image() {}

    bool load_image(const std::string filename)
    {
        unsigned char* data;
        int bpp;
        data = stbi_load(filename.c_str(), &width_, &height_, &bpp, 0);

        if (!buf_i_.Initialize(width_, height_)) {
            std::cout << "Error: cannot allocate an inner buffer!" << std::endl; return false; 
        }

        #pragma omp parallel for
        for (int i = 0; i < height_; i++) {
            int y = i;// 上下反転
//            int y = height_ - 1 - i;// 上下反転
            unsigned char* p = data + width_ * i * bpp;
            tpot::color_uc* dst = buf_i_.get_raw() + width_ * y;
            for (int x = 0; x < width_; x++) {
                dst->r = p[0];
                dst->g = p[1];
                dst->b = p[2];

                dst++;
                p += bpp;
            }
        }

        stbi_image_free(data);

        // 浮動小数点バッファに変更
        generate_float_buffer();

        return true;
    }

    void save_image(const std::string filename)
    {
        stbi_write_png(
            filename.c_str(),                        // filename
            this->width_, this->height_,             // size
            STBI_rgb,                                // format
            this->buf_i_.see_raw(),                  // image
            3 * this->width_);                       // stride
    }

    bool update_buffer_f(const tpot::color_buffer_f& src) 
    {
        if (!src.copy(buf_f_)) return false;

        buf_f_.to_uc(buf_i_);

        return true;
    }

    int width() const { return width_; }
    int height() const { return height_; }
    const tpot::color_buffer_f& see_buffer() const { return buf_f_; }

//    const RGB& get_color(int x, int y) const { return  pixels_[width_ * y + x]; }
//    const fRGB& get_color_float(int x, int y) const { return  float_pixels_[width_ * y + x]; }
//    void set_color_float(int x, int y, fRGB c) const { float_pixels_[width_ * y + x] = c; }
//    unsigned char get_intensity(int x, int y) const { const RGB& c = get_color(x, y); return (unsigned char)((255.99999f / 255.0f) * (0.298839f * (float)c.r + 0.586811f * (float)c.g + 0.114350f * (float)c.b)); }
};

// ----------------------------------------
// 引数
const float AVERAGE_RADIUS_MAX = 10000;

struct ARGS {
    bool is_show_help = false;                     // -h
//    std::string  input = "monalisa120x180.png";    // -i
//    std::string  input = "suiren_112x112.png";
//    std::string  input = "himawari_145x190.png";
//    std::string  input = "higasa_189x299.png";
//    std::string  input = "ochibohiroi_125x96.png";
    std::string  input = "kanagawaokinamiura_200x133.png";
    std::string output = "out.png";                // -o
    float radius = 3.0f;                           // -radius
};

void ShowHelp()
{
    std::cout << std::endl;
    std::cout << "  -h: ヘルプ表示" << std::endl;
    std::cout << "  -i [ファイル名]: 入力pngファイル" << std::endl;
    std::cout << "  -o [ファイル名]: 出力pngファイル" << std::endl;
    std::cout << "  -radius [値]: 平均を見る半径 (0-" << AVERAGE_RADIUS_MAX << ")" << std::endl;
}

void analyze_args(ARGS& dest, int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-') {
            const char* p = &argv[i][1];
            if (strcmp(p, "h") == 0) {
                dest.is_show_help = true;
            }
            else
            if (strcmp(p, "i") == 0) {
                dest.input = (++i < argc) ? argv[i] : "";
            }
            else
            if (strcmp(p, "o") == 0) {
                dest.output = (++i < argc) ? argv[i] : "";
            }
            else
            if (strcmp(p, "radius") == 0) {
                dest.radius = (++i < argc) ? (float)std::atof(argv[i]) : 0.0f;
                dest.radius = std::clamp(dest.radius, 0.0f, AVERAGE_RADIUS_MAX);
            }
        }
    }
}

// ----------------------------------------
// エントリーポイント
int main(int argc, char* argv[])
{
	// メモリリークのチェック
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // _DEBUG

	ARGS args;
	analyze_args(args, argc, argv);

    // タイトル表示
    std::cout << std::endl;
    std::cout << " distance_art:" << std::endl;
    std::cout << "    using stb libraries." << std::endl;
    std::cout << std::endl;

    if (args.is_show_help)
	{
		ShowHelp();
		return 0;
	}

    // 入力ファイルの存在check
    std::error_code ec;
    bool result = std::filesystem::exists(args.input, ec);
    assert(!ec);
    if (!result) {
        std::cout << "Error:input file(" << args.input << ") does not exist!" << std::endl;
        std::cout << "error code: (" << ec.value() << ") " << ec.message() << std::filesystem::current_path() << std::endl;
        return -1;
    }

    // ファイルロード
    image img;
    if(!img.load_image(args.input)) return -1;

    // データ表示
    std::cout << "      input: " << args.input << " (" << img.width() << "x" << img.height() << ")" << std::endl;
    std::cout << "     output: " << args.output << std::endl;

    // 結果バッファの用意
    tpot::color_buffer_f buf;
    buf.Initialize(img.width(), img.height());

    // 変換
    const CONVERT_OPTIONS options = {
        &img.see_buffer(),      // target
        args.radius,            //  radius
    };
    Analize::convert(buf, options);

    // 出力
    img.update_buffer_f(buf);
    img.save_image(args.output);

    buf.Release();

    std::cout << "DONE!!" << std::endl;

    return 0;
}

