
// stl
#include <iostream> // cout
#include <algorithm> // min, max, clamp
#include <cmath> // powf

// オレオレライブラリ
#include "tpot_image.h"

namespace tpot
{
	bool color_buffer_uc::Initialize(int width, int height)
	{
		if (width < 1 || height < 1) { std::cout << "Error: buffer size is zero!" << std::endl; return false; }

		if (this->image_) Release();

		this->width_ = width;
		this->height_ = height;

		this->image_ = new color_uc[width * height];

		return true;
	}


	void color_buffer_uc::Release()
	{
		if (this->image_) delete[] this->image_; this->image_ = nullptr;
	}


	bool color_buffer_f::Initialize(int width, int height)
	{
		if (width < 1 || height < 1) { std::cout << "Error: buffer size is zero!" << std::endl; return false; }

		if (this->image_) Release();

		this->width_ = width;
		this->height_ = height;

		this->image_ = new color[width * height];

		return true;
	}


	void color_buffer_f::Release()
	{
		if (this->image_) delete[] this->image_; this->image_ = nullptr;
	}

	bool color_buffer_uc::to_f(color_buffer_f& dst) const
	{
		dst.Initialize(this->width_, this->height_);

		color* a_f = dst.get_raw();

		#pragma omp parallel for
		for (int y = 0; y < this->height_; y++) 
		{
			const color_uc* p_i = this->image_ + this->width_ * y;
			color* p_f = a_f + this->width_ * y;
			for (int x = 0; x < this->width_; x++) 
			{
				p_f[x] = p_i[x].to_f();
				if (1.0f < p_f[x].r || 1.0f < p_f[x].g || 1.0f < p_f[x].b) {
					std::cout << "Error: too bright pixel!" 
						<< " R:" << p_f[x].r 
						<< " G:" << p_f[x].g 
						<< " B:" << p_f[x].b << std::endl;
				}
			}
		}

		return true;
	}

	bool color_buffer_f::to_uc(color_buffer_uc& dst) const
	{
		dst.Initialize(this->width_, this->height_);

		color_uc* a_i = dst.get_raw();

		#pragma omp parallel for
		for (int y = 0; y < this->height_; y++) 
		{
			const color* p_f = this->image_ + this->width_ * y;
			color_uc* p_i = a_i + this->width_ * y;
			for (int x = 0; x < this->width_; x++) 
			{
				p_i[x] = p_f[x].to_i();
				if (1.0f < p_f[x].r || 1.0f < p_f[x].g || 1.0f < p_f[x].b) {
					std::cout << "Error: too bright pixel!"
						<< " R:" << p_f[x].r
						<< " G:" << p_f[x].g
						<< " B:" << p_f[x].b << std::endl;
				}
			}
		}

		return true;
	}

	bool color_buffer_uc::copy(color_buffer_uc& dst) const
	{
		// 同じサイズでないとひとまず受け入れない
		if (width_ != dst.width() || height_ != dst.height()) {
			std::cout << "Error:copy image size is invalid!" << std::endl;
			return false;
		}

		color_uc* a_d = dst.get_raw();

		#pragma omp parallel for
		for (int y = 0; y < this->height_; y++)
		{
			const color_uc* p_s = this->image_ + this->width_ * y;
			color_uc* p_d = a_d + this->width_ * y;
			for (int x = 0; x < this->width_; x++)
			{
				p_d[x] = p_s[x];
			}
		}

		return true;
	}


	bool color_buffer_f::copy(color_buffer_f & dst) const
	{
		// 同じサイズでないとひとまず受け入れない
		if (width_ != dst.width() || height_ != dst.height()) {
			std::cout << "Error:copy image size is invalid!" << std::endl;
			return false;
		}

		color* a_d = dst.get_raw();

		#pragma omp parallel for
		for (int y = 0; y < this->height_; y++)
		{
			const color* p_s = this->image_ + this->width_ * y;
			color* p_d = a_d + this->width_ * y;
			for (int x = 0; x < this->width_; x++)
			{
				p_d[x] = p_s[x];
			}
		}

		return true;
	}

} // namespace tpot
