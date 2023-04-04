#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <png.h>
#include <jpeglib.h>

#include "OctoAI.hpp"

// Port from my mle_map library. WITH FIXES OF LEAKS LMAO.

namespace OAI {
	void TMap2D::Allocate(TU16 Width, TU16 Height, TU32 ChannelsNum) {
		Free();
		this->Width = Width;
		this->Height = Height;
		this->ChannelsNum = ChannelsNum;

		this->Data = new TU8[Width*Height*ChannelsNum];
	}

	TMap2D::TMap2D(const char* Fp) {
		if (!Load(Fp)) {
			puts("FUCK");
			fflush(stdout);
			Free();
		}
	}

	TMap2D::TMap2D(TConfig& C) {
		Allocate(C.Width, C.Height, C.ChannelsNum);
	}
	TMap2D::TMap2D(TConfig& Config, TU8* data) {
		this->Width = Config.Width;
		this->Height = Config.Height;
		this->ChannelsNum = Config.ChannelsNum;
		
		this->Data = data;
	}

	TMap2D::~TMap2D() {
		Free();
	}
	
	void TMap2D::Free() {
		delete [] Data;
		Data = nullptr;
	}

	void TMap2D::SetPixel(TU16 x, TU16 y, const TU8* pixel) {
		for (unsigned c = 0; c < ChannelsNum; c++)
			Data[(Width*y+x)*ChannelsNum+c] = pixel[c];
	}
	void TMap2D::GetPixel(TU16 x, TU16 y, TU8* pixel) {
		for (unsigned c = 0; c < ChannelsNum; c++)
			pixel[c] = Data[(Width*y+x)*ChannelsNum+c];
	}
	TU8* TMap2D::GetPixel(TU16 x, TU16 y) {
		return Data+((Width*y+x)*ChannelsNum);
	}

	void TMap2D::Contrast(float Factor) {
		for (unsigned I = 0; I < Width*Height*ChannelsNum; I++) {
			TI16 New = Factor*(Data[I] - 127) + 127;
			if (New > 255)
				Data[I] = 255;
			else if (New < 0)
				Data[I] = 0;
			else
				Data[I] = New;
		}
	}
	void TMap2D::Lighten(float Factor) {
		for (unsigned I = 0; I < Width*Height*ChannelsNum; I++) {
			TI16 New = Factor*Data[I];
			if (New > 255)
				Data[I] = 255;
			else if (New < 0)
				Data[I] = 0;
			else
				Data[I] = New;
		}
	}
	void TMap2D::Noise(TU8 Strength) {
		for (unsigned I = 0; I < Width*Height*ChannelsNum; I++) {
			TI16 New = ((TI16)(Rng()%Strength))+Data[I];
			if (New > 255)
				Data[I] = 255;
			else if (New < 0)
				Data[I] = 0;
			else
				Data[I] = New;
		}
	}

	void TMap2D::Crop(TU16 l, TU16 t, TU16 r, TU16 b) {
		TU16 cropped_w = r-l, cropped_h = b-t;
		TU8* cropped_data = (TU8*)calloc(sizeof(TU8)*ChannelsNum*cropped_w*cropped_h, 1);

		for (TU16 x = 0; x < cropped_w; x++) {
			for (TU16 y = 0; y < cropped_h; y++) {
				for (unsigned c = 0; c < ChannelsNum; c++)
					if (y+t >= 0 && y+t < Height && x+l >= 0 && x+l < Width)
						cropped_data[(cropped_w*y+x)*ChannelsNum+c]=
						Data[(Width*(y+t)+(x+l))*ChannelsNum+c];
			}
		}
		Width = cropped_w;
		Height = cropped_h;
		free(Data);
		Data = cropped_data;
	}

	// This works using the rotation matrix for points, but reversing the concept.
	// Instead of finding the location of the new pixel, we do the opposite locate the original pixel for the new rotated map. This way we eliminate gaps and make sure every pixel in the new map is accounted for carefully.
	void TMap2D::Rotate(float rad, TU16 around_x, TU16 around_y) {
		TU8* rotated_data = (TU8*)calloc(sizeof(TU8)*ChannelsNum*Width*Height, 1);

		float c = cosf(-rad);
		float s = sinf(-rad);

		for (TU16 x = 0; x < Width; x++) {
			for (TU16 y = 0; y < Height; y++) {
				int ol_x = (x-around_x)*c-(y-around_y)*s+around_x;
				int ol_y = (x-around_x)*s+(y-around_y)*c+around_y;

				// Discarding pixels that come out of bounds
				if (
					ol_x >= 0 && ol_x < Width && 
					ol_y >= 0 && ol_y < Height
				)
					for (unsigned c = 0; c < ChannelsNum; c++) {
						rotated_data[(Width*y+x)*ChannelsNum+c]=
						Data[(Width*ol_y+ol_x)*ChannelsNum+c];
					}
			}
		}
		free(Data);
		Data = rotated_data;
	}

	// This one wasn't ported it did me quite some headaches.
	void TMap2D::Resize(TU16 W, TU16 H) {
		TU8* ResizedData = new TU8[W*H*ChannelsNum];
		
		float HR = (float)Height/H;
		float WR = (float)Width/W;

		TU8* Pixel;
		for (int Y = 0; Y < H; Y++) {
			int PickedY = Y*HR;
			for (int X = 0; X < W; X++) {
				int PickedX = X*WR;
				Pixel = GetPixel(PickedX, PickedY);
				// SetPixel
				for (unsigned c = 0; c < ChannelsNum; c++)
					ResizedData[(W*Y+X)*ChannelsNum+c] = Pixel[c];
			}
		}


		delete [] Data;
		Data = ResizedData;
		Width = W;
		Height = H;
	}

	bool TMap2D::LoadPNG(FILE* f) {
		fseek(f, 8, SEEK_SET); // Skip first 8 bytes of header

		png_structp png_p = png_create_read_struct(
			PNG_LIBPNG_VER_STRING, 
			nullptr,
			nullptr,
			nullptr
		);
		if (!png_p)
			return false;
		
		png_infop info_p = png_create_info_struct(png_p);
		if (!info_p) {
			png_destroy_read_struct(&png_p, nullptr, nullptr);
			return false;
		}
		
		png_init_io(png_p, f);
		png_set_sig_bytes(png_p, 8);
		png_read_info(png_p, info_p);

		int color_type = png_get_color_type(png_p, info_p);
		if (color_type & PNG_COLOR_MASK_PALETTE)
			return false;

		TU16 w = png_get_image_width(png_p, info_p);
		TU16 h = png_get_image_height(png_p, info_p);
		int channels_n = 1;

		if (color_type & PNG_COLOR_MASK_COLOR)
			channels_n += 2;
		if (color_type & PNG_COLOR_MASK_ALPHA)
			channels_n += 1;

		Allocate(w, h, channels_n);

		TU8* row = new TU8[w*channels_n];
		
		for (int y = 0; y < h; y++) {
			png_read_row(png_p, row, nullptr);
			
			for (int i = 0; i < w*channels_n; i++)
				Data[(w*y)*channels_n+i] = row[i];
		}

		png_destroy_info_struct(png_p, &info_p);
		png_destroy_read_struct(&png_p, &info_p, nullptr);

		delete[] row;

		return true;
	}

	bool TMap2D::LoadJPG(FILE* f) {
		struct jpeg_decompress_struct jpeg;
	
		struct jpeg_error_mgr jerr;
		jpeg.err = jpeg_std_error(&jerr);

		jpeg_create_decompress(&jpeg);

		jpeg_stdio_src(&jpeg, f);

		jpeg_read_header(&jpeg, TRUE);

		// Set output color space to RGB if TMap2D is in YCbCr, because fuck YCbCr.
		if (jpeg.jpeg_color_space == JCS_YCbCr)
			jpeg.out_color_space = JCS_RGB;
		jpeg_calc_output_dimensions(&jpeg);

		jpeg_start_decompress(&jpeg);

		TU16 w = jpeg.output_width, h = jpeg.output_height;
		int channels_n = jpeg.num_components;

		Allocate(w, h, channels_n);

		TU8* row = new TU8[w*channels_n];
		while (jpeg.output_scanline < jpeg.output_height) {
			jpeg_read_scanlines(&jpeg, &row, 1);

			int y = jpeg.output_scanline-1; // Because it is incremented
			
			for (int i = 0; i < w*channels_n; i++)
				Data[(w*y)*channels_n+i] = row[i];
		}

		jpeg_finish_decompress(&jpeg);
		jpeg_destroy_decompress(&jpeg);
		delete [] row;

		return 1;
	}

	bool TMap2D::LoadRAW(FILE* f) {
		TU16 Size[2];
		fread(Size, sizeof(Width), 2, f);
		int ChannelsNum;
		fread(&ChannelsNum, sizeof(ChannelsNum), 1, f);

		Allocate(Size[0], Size[1], ChannelsNum);

		fread(Data, Size[0]*Size[1]*ChannelsNum, 1, f);

		return true;
	}

	bool TMap2D::Load(const char* Fp) {
		FILE* F = fopen(Fp, "rb");
		if (!F)
			return 0;
		
		static const char JpegMagic[] = {(char)0xff, (char)0xd8, (char)0xff};
		static const char PngMagic[] = {(char)0x89, 'P', 'N', 'G'};

		char TestMagic[4];
		fread(TestMagic, 4, 1, F);
		rewind(F);
		
		bool Ret;

		if (!memcmp(TestMagic, JpegMagic, 3))
			Ret = LoadJPG(F);
		else if (!memcmp(TestMagic, PngMagic, 4))
			Ret = LoadPNG(F);
		else
			Ret = LoadRAW(F);
		
		fclose(F);
		return Ret;
	}

	bool _LowerCaseStrcmp(const char* Lowered, const char* Other) {
		while (*Lowered && *Other) {
			// Lowercase
			char L = *Lowered;
			if (L >= 'A' && L <= 'Z')
				L += 32;
			// Compare
			if (L != *Other)
				return false;
			
			Lowered++;
			Other++;
		}
		// Both terminated though?
		if (*Lowered == *Other)
			return true;
		return false;
	}

	bool TMap2D::SaveRAW(const char* Fp) {
		FILE* F = fopen(Fp, "wb");
		if (!F)
			return false;
		
		fwrite(&Width, sizeof(Width), 1, F);
		fwrite(&Height, sizeof(Width), 1, F);
		fwrite(&ChannelsNum, sizeof(ChannelsNum), 1, F);

		fwrite(Data, sizeof(*Data), Width*Height*ChannelsNum, F);

		fclose(F);

		return true;
	}

	bool TMap2D::SaveJPG(const char *fp, int quality) {
		FILE* f = fopen(fp, "wb");
		if (!f)
			return 0;
		
		struct jpeg_compress_struct jpeg;
		struct jpeg_error_mgr jerr;

		jpeg.err = jpeg_std_error(&jerr);

		jpeg_create_compress(&jpeg);

		jpeg_stdio_dest(&jpeg, f);

		jpeg.image_width = Width;
		jpeg.image_height = Height;
		jpeg.input_components = ChannelsNum;
		if (ChannelsNum == 3)
			jpeg.in_color_space = JCS_RGB;
		else if (ChannelsNum == 1)
			jpeg.in_color_space = JCS_GRAYSCALE;

		jpeg_set_defaults(&jpeg);

		if (quality != 100)
			jpeg_set_quality(&jpeg, quality, FALSE);
		
		jpeg_start_compress(&jpeg, TRUE);

		// TU8* row = new TU8[ChannelsNum * Width];
		while (jpeg.next_scanline < jpeg.image_height) {
			int y = jpeg.next_scanline;
			// for (unsigned i = 0; i < Width*ChannelsNum; i++) {
			// 	row[i] = Data[(Width*y)*ChannelsNum+i];
			// }
			TU8* Ptr = Data+(Width*y)*ChannelsNum;
			jpeg_write_scanlines(&jpeg, &Ptr, 1);
		}
		
		jpeg_finish_compress(&jpeg);
		jpeg_destroy_compress(&jpeg);
		// delete [] row;
		fclose(f);

		return 1;
	}

	bool TMap2D::SavePNG(const char *fp) {
		FILE* f = fopen(fp, "wb");
		if (!f)
			return false;
		
		png_structp png_p = png_create_write_struct(
			PNG_LIBPNG_VER_STRING, 
			NULL,
			NULL,
			NULL
		);
		if (!png_p) {
			fclose(f);
			return false;
		}
		
		png_infop info_p = png_create_info_struct(png_p);
		if (!info_p) {
			png_destroy_read_struct(&png_p, NULL, NULL);
			fclose(f);
			return false;
		}
		
		png_init_io(png_p, f);

		// Determine color type
		int color_type = 0;
		int c = ChannelsNum;
		if (c == 1)
			color_type = PNG_COLOR_TYPE_GRAY;
		else if (c == 2)
			color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
		else if (c == 3)
			color_type = PNG_COLOR_TYPE_RGB;
		else if (c == 4)
			color_type = PNG_COLOR_TYPE_RGB_ALPHA;
			
		png_set_IHDR(
			png_p,
			info_p,
			Width, Height,
			8, color_type,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE,
			PNG_FILTER_TYPE_BASE
		);
		png_write_info(png_p, info_p);

		// TU8 row[Width*c];
		
		for (int y = 0; y < Height; y++) {
			// for (int i = 0; i < Width*c; i++)
			// 	row[i] = Data[(Width*y)*c+i];
			// png_write_row(png_p, row);
			png_write_row(png_p, Data+(Width*y)*c);
		}

		png_destroy_info_struct(png_p, &info_p);
		png_destroy_write_struct(&png_p, &info_p);
		fclose(f);

		return true;
	}

	bool TMap2D::Save(const char* Fp) {
		const char* Ext;
		for (Ext = Fp; *Ext != '.'; Ext++) {
			if (!*Ext)
				return false;
		}
		Ext++;

		bool Ret;
		if (_LowerCaseStrcmp(Ext, "png"))
			Ret = SavePNG(Fp);
		else if (_LowerCaseStrcmp(Ext, "jpeg") || _LowerCaseStrcmp(Ext, "jpg"))
			Ret = SaveJPG(Fp, 100);
		else
			Ret = SaveRAW(Fp);
		
		return Ret;
	}
}