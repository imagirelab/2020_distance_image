#pragma once
#pragma warning( disable : 4091 ) // 前方宣言のクラス定義がwarningになるがわざとなので無効

// #include <algorithm> // min, max, clamp
// #include <cmath> // powf

namespace tpot
{
    extern class color;

    class color_uc
    {
    public:
        unsigned char r, g, b;

        color to_f() const;
    };

    class color
    {
    public:
        float r = 0.0f, g = 0.0f, b = 0.0f;

        color operator * (float f) const { color r = *this; r.r *= f; r.g *= f; r.b *= f; return r; }
        color operator + (const color& s) const { color r; r.r = this->r + s.r; r.g = this->g + s.g; r.b = this->b + s.b; return r; }
        color operator - (const color& s) const { color r; r.r = this->r - s.r; r.g = this->g - s.g; r.b = this->b - s.b; return r; }
        color operator += (const color& s) { this->r += s.r; this->g += s.g; this->b += s.b; return *this; }
        color operator *= (float f) { this->r *= f; this->g *= f; this->b *= f; return *this; }
        color operator /= (float f) { this->r /= f; this->g /= f; this->b /= f; return *this; }

        color_uc to_i() const {
            color_uc i;

            i.r = (int)(r * 255.99999f);
            i.g = (int)(g * 255.99999f);
            i.b = (int)(b * 255.99999f);

            return i;
        }

        void clamp() {
            this->r = std::clamp(this->r, 0.0f, 1.0f);
            this->g = std::clamp(this->g, 0.0f, 1.0f);
            this->b = std::clamp(this->b, 0.0f, 1.0f);
        }

        static color xyz2rgb(const color& xyz) // sRGB (D65)
        {
            color c;
            c.r = +3.240479f * xyz.r - 1.537150f * xyz.g - 0.498535f * xyz.b;
            c.g = -0.969256f * xyz.r + 1.875892f * xyz.g + 0.041556f * xyz.b;
            c.b = +0.055648f * xyz.r - 0.204043f * xyz.g + 1.057311f * xyz.b;

            return c;
        }

        static color rgb2xyz(const color& rgb)
        {
            color c;
            c.r = 0.412453f * rgb.r + 0.357580f * rgb.g + 0.180423f * rgb.b;
            c.g = 0.212671f * rgb.r + 0.715160f * rgb.g + 0.072169f * rgb.b;
            c.b = 0.019334f * rgb.r + 0.119193f * rgb.g + 0.950227f * rgb.b;

            return c;
        }

        static float f(float t)
        {
            if (0.008856 < t) {
                return std::powf(t, 1.0f / 3.0f);
            }
            else {
                return 7.787f * t + 16.0f / 116.0f;
            }
        }

        static float f_inv(float s)
        {
            float t = (s - 16.0f / 116.0f) / 7.787f;

            if (0.008856 < t) {
                return std::powf(s, 3.0f);
            }
            else {
                return t;
            }
        }

        // XYZ(D65) to L*a*b*
        static color xyz2lab(const color& xyz)
        {
            color c;

            float X = xyz.r;
            float Y = xyz.g;
            float Z = xyz.b;

            // D65
            float Xn = 0.9504f;
            float Yn = 1.0000f;
            float Zn = 1.0888f;

            float Xd = X / Xn;
            float Yd = Y / Yn;
            float Zd = Z / Zn;

            // L*
            if (0.008856f < Yd) {
                c.r = 116.0f * std::powf(Yd, 1.0f / 3.0f) - 16.0f;
            }
            else {
                c.r = 903.3f * Yd;
            }
            // a*
            c.g = 500.0f * (f(Xd) - f(Yd));
            // b*
            c.b = 200.0f * (f(Yd) - f(Zd));

            return c;
        }

        // L*a*b* to XYZ(D65)
        static color lab2xyz(const color& lab)
        {
            color c;

            // D65
            float Xn = 0.9504f;
            float Yn = 1.0000f;
            float Zn = 1.0888f;

            // Y
            float Yd = lab.r / 903.3f;
            if (0.008856f < Yd) {
                Yd = std::powf((lab.r + 16.0f) / 116.0f, 3.0f);
            }
            c.g = Yn * Yd;

            c.r = Xn * f_inv(+lab.g / 500.0f + f(Yd)); // X
            c.b = Zn * f_inv(-lab.b / 200.0f + f(Yd)); // Z

            return c;
        }

        static color rgb2lab(const color& rgb) { return xyz2lab(rgb2xyz(rgb)); }
        static color lab2rgb(const color& rgb) { return xyz2rgb(lab2xyz(rgb)); }
    };

    inline color color_uc::to_f() const {
        color f;
        f.r = (1.0f / 255.0f) * ((float)r);
        f.g = (1.0f / 255.0f) * ((float)g);
        f.b = (1.0f / 255.0f) * ((float)b);
        return f;
    }

    struct HSV
    {
        unsigned int h, s, v;
        HSV(const color_uc& c) {
            v = std::max(std::max(c.r, c.g), c.b);
            int b = std::min(std::min(c.r, c.g), c.b);
            s = 255 * (v - b) / v;
            h = ((c.r < c.g) ? 120 : 0) + ((c.g < c.b) ? 120 : 0) +
                60 * ((c.g < c.r&& c.b < c.r) ? (c.g - c.b) : ((c.b < c.g) ? (c.b - c.r) : (c.r - c.g))) / (v - b);
        }
    };

    // これらクラスをテンプレート化したかったが、コピーをうまく作れなかったので、pending
    extern class color_buffer_f;

    class color_buffer_uc
    {
    private:
        int width_ = 0;
        int height_ = 0;
        color_uc* image_ = nullptr;
    public:
        color_buffer_uc() {}
        ~color_buffer_uc() { Release(); }

        bool Initialize(int width, int height);
        void Release();

        color_uc& get(int x, int y) const { return image_[y * width_ + x]; }
        void set(int x, int y, const color_uc& c) { image_[y * width_ + x] = c; }

        int width() const { return width_; }
        int height() const { return height_; }
        color_uc* get_raw() { return image_; }
        color_uc* see_raw() const { return image_; }

        bool copy(color_buffer_uc& dst) const;
        bool to_f(color_buffer_f& dst) const;
    };

    class color_buffer_f
    {
    private:
        int width_ = 0;
        int height_ = 0;
        color* image_ = nullptr;
    public:
        color_buffer_f() {}
        ~color_buffer_f() { Release(); }

        bool Initialize(int width, int height);
        void Release();

        color& get(int x, int y) const { return image_[y * width_ + x]; }
        void set(int x, int y, const color& c) { image_[y * width_ + x] = c; }

        int width() const { return width_; }
        int height() const { return height_; }
        color* get_raw() { return image_; }
        color* see_raw() const { return image_; }

        bool copy(color_buffer_f& dst) const;
        bool to_uc(color_buffer_uc& dst) const;
    };

} // namespace tpot


