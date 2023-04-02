#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <png.h>
#include <jpeglib.h>

#include "OctoAI.hpp"

namespace OAI {
	void TMap2D::Allocate(TU16 Width, TU16 Height, TU32 ChannelsNum) {
		this->Width = Width;
		this->Height = Height;
		this->ChannelsNum = ChannelsNum;

		this->Data = new TU8[Width*Height*ChannelsNum];
	}

	TMap2D::TMap2D(const char* Fp) {
		if (!Load(Fp))
			Free();
	}

	TMap2D::~TMap2D() {
		Free();
	}
	
	void TMap2D::Free() {
		if (Data)
			delete [] Data;
		Data = nullptr;
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
		fread(&ChannelsNum, 4, 1, f);

		Allocate(Size[0], Size[1], ChannelsNum);

		fread(Data, Size[0]*Size[1]*ChannelsNum, 1, f);

		return true;
	}

	bool TMap2D::Load(const char* Fp) {
		FILE* F = fopen(Fp, "rb");
		if (!F)
			return 0;
		
		char JpegMagic[] = {0xff, 0xd8, 0xff};
		char PngMagic[] = {0x89, 'P', 'N', 'G'};

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

	bool SaveRAW(const char* Fp) {
		fwrite(Size, 2, 2, f);
		int ChannelsNum;
		fread(&ChannelsNum, 4, 1, f);

		Allocate(Size[0], Size[1], ChannelsNum);

		fread(Data, Size[0]*Size[1]*ChannelsNum, 1, f);

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
			Ret = SaveJPG(Fp);
		else if (_LowerCaseStrcmp(Ext, "jpeg") || _LowerCaseStrcmp(Ext, "jpg"))
			Ret = SavePNG(Fp);
		else
			Ret = SaveRAW(Fp);
		
		return Ret;
	}
}