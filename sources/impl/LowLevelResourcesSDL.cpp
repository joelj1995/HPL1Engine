/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef WIN32
#pragma comment(lib, "SDL2_image.lib")
#endif

#include "impl/LowLevelResourcesSDL.h"
#include "impl/SDLBitmap2D.h"
#include "impl/MeshLoaderMSH.h"
#include "impl/MeshLoaderCollada.h"
#include "impl/MeshLoaderMap.h"
#ifdef INCLUDE_THEORA
#include "impl/VideoStreamTheora.h"
#endif
#include "impl/Platform.h"
#include "system/String.h"

#include "SDL/SDL.h"
#include "SDL/SDL_image.h"
#include "resources/MeshLoaderHandler.h"
#include "resources/VideoManager.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cLowLevelResourcesSDL::cLowLevelResourcesSDL(cLowLevelGraphicsSDL *apLowLevelGraphics)
	{
		mvImageFormats[0] = "BMP";mvImageFormats[1] = "LBM";mvImageFormats[2] = "PCX";
		mvImageFormats[3] = "GIF";mvImageFormats[4] = "JPEG";mvImageFormats[5] = "PNG";
		mvImageFormats[6] = "JPG";
		mvImageFormats[7] = "TGA";mvImageFormats[8] = "TIFF";mvImageFormats[9] = "TIF";
		mvImageFormats[10] =  "";

		mpLowLevelGraphics = apLowLevelGraphics;
	}

	//-----------------------------------------------------------------------

	cLowLevelResourcesSDL::~cLowLevelResourcesSDL()
	{

	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHOD
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	iBitmap2D* cLowLevelResourcesSDL::LoadBitmap2D(tString asFilePath, tString asType)
	{
		tString tType;
		if(asType != "")
			asFilePath = cString::SetFileExt(asFilePath,asType);

		tType = cString::GetFileExt(asFilePath);
		SDL_Surface* pSurface = NULL;

		if (tType=="bmp") {
			#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			SDL_PixelFormat RGBAFormat;
			RGBAFormat.palette = 0; RGBAFormat.colorkey = 0; RGBAFormat.alpha = 0;
			RGBAFormat.BitsPerPixel = 32; RGBAFormat.BytesPerPixel = 4;

			RGBAFormat.Rmask = 0xFF000000; RGBAFormat.Rshift = 0; RGBAFormat.Rloss = 0;
			RGBAFormat.Gmask = 0x00FF0000; RGBAFormat.Gshift = 8; RGBAFormat.Gloss = 0;
			RGBAFormat.Bmask = 0x0000FF00; RGBAFormat.Bshift = 16; RGBAFormat.Bloss = 0;
			RGBAFormat.Amask = 0x000000FF; RGBAFormat.Ashift = 24; RGBAFormat.Aloss = 0;

			SDL_Surface* orig = NULL;
			orig = IMG_Load(asFilePath.c_str());

			if(orig==NULL){
				//Error handling stuff?
				return NULL;
			}
			pSurface = SDL_ConvertSurface(orig, &RGBAFormat, SDL_SWSURFACE);
			SDL_FreeSurface(orig);
			#else
			pSurface = IMG_Load(asFilePath.c_str());
			#endif
		}
		else if (tType == "dds") {
			pSurface = DDS_Load(asFilePath.c_str());
		}
		else {
			pSurface= IMG_Load(asFilePath.c_str());
		}
		if(pSurface==NULL){
			//Error handling stuff?
			return NULL;
		}

		iBitmap2D* pBmp = mpLowLevelGraphics->CreateBitmap2DFromSurface(pSurface,
													cString::GetFileExt(asFilePath));
		pBmp->SetPath(asFilePath);

		return pBmp;
	}

	//-----------------------------------------------------------------------

	void cLowLevelResourcesSDL::GetSupportedImageFormats(tStringList &alstFormats)
	{
		int lPos = 0;

		while(mvImageFormats[lPos]!="")
		{
			alstFormats.push_back(mvImageFormats[lPos]);
			lPos++;
		}
	}
	//-----------------------------------------------------------------------

	void cLowLevelResourcesSDL::AddMeshLoaders(cMeshLoaderHandler* apHandler)
	{
		//apHandler->AddLoader(hplNew( cMeshLoaderFBX,(mpLowLevelGraphics)));
		apHandler->AddLoader(hplNew( cMeshLoaderMSH,(mpLowLevelGraphics)));
		apHandler->AddLoader(hplNew( cMeshLoaderCollada,(mpLowLevelGraphics)));
		apHandler->AddLoader(hplNew( cMeshLoaderMap, (mpLowLevelGraphics)));
	}

	//-----------------------------------------------------------------------

	void cLowLevelResourcesSDL::AddVideoLoaders(cVideoManager* apManager)
	{
		#ifdef INCLUDE_THORA
		apManager->AddVideoLoader(hplNew( cVideoStreamTheora_Loader,()));
		#endif
	}

	SDL_Surface* cLowLevelResourcesSDL::DDS_Load(const char* file)
	{
		// courtesy of https://gist.github.com/tilkinsc/13191c0c1e5d6b25fbe79bbd2288a673
		SDL_Surface* image = 0;

		unsigned char* header = 0;
		unsigned int width;
		unsigned int height;
		unsigned int mipMapCount;

		unsigned int blockSize;
		unsigned int format;

		unsigned int w;
		unsigned int h;

		unsigned char* buffer = 0;

		GLuint tid = 0;

		SDL_RWops* src = SDL_RWFromFile(file, "rb");
		if (src == 0)
		{
			Error("Could not open DDS image %s", file);
			return nullptr;
		}

		long file_size = SDL_RWsize(src);

		header = (unsigned char*) malloc(128);
		if (header == 0)
			FatalError("Could not allocate memory for DDS header");
		if (SDL_RWseek(src, 0, RW_SEEK_CUR) < 0)
			FatalError("Could not seek in DDS file");
		if (!SDL_RWread(src, header, 128, 1))
			FatalError("Could not read DDS file");

		if (memcmp(header, "DDS ", 4) != 0)
		{
			free(header);
			FatalError("%s", header);
		}

		height = (header[12]) | (header[13] << 8) | (header[14] << 16) | (header[15] << 24);
		width = (header[16]) | (header[17] << 8) | (header[18] << 16) | (header[19] << 24);
		mipMapCount = (header[28]) | (header[29] << 8) | (header[30] << 16) | (header[31] << 24);

		if (header[84] == 'D') {
			switch (header[87]) {
			case '1': // DXT1
				format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
				blockSize = 8;
				break;
			case '3': // DXT3
			case '5': // DXT5
			case '0': // DX10
				FatalError("DDS format not supported");
			default: FatalError("Unknown DDS format.");
			}
		}
		else // BC4U/BC4S/ATI2/BC55/R8G8_B8G8/G8R8_G8B8/UYVY-packed/YUY2-packed unsupported
			FatalError("DDS format not recognized.");

		buffer = (unsigned char*) malloc(file_size - 128);
		if (buffer == 0)
			FatalError("Could not allocate memory for DDS buffer");

		SDL_RWread(src, buffer, file_size, 1);

		unsigned int offset = 0;
		unsigned int size = 0;
		w = width;
		h = height;

		Uint32 Rmask, Gmask, Bmask, Amask;
		Rmask = 0x000000FF;
		Gmask = 0x0000FF00;
		Bmask = 0x00FF0000;
		Amask = 0xFF000000;
		image = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, Rmask, Gmask, Bmask, Amask);
		if (!image)
			FatalError("Could not create surface for DDS texture");

		int8_t* pixels = (int8_t*) image->pixels;
		/*for (int h = 0; h < height / 4; h++)
		{
			for (int w = 0; w < width / 4; w++)
			{
				uint8_t* colorValues = 0;
				int colorIndices = 0;

				for (int y = 0; y < 4; y++)
				{
					for (int x = 0; x < 4; x++)
					{
						int pixelIndex = (3 - x) + (y * 4);
						int imageIndex = (h * 4 + 3 - y) * width * 4 + (w * 4 + x) * 4;
						int colorIndex = (colorIndices >> (2 * (15 - pixelIndex))) & 0x03;
						pixels[imageIndex] = colorValues[colorIndex * 4];
						pixels[imageIndex + 1] = colorValues[colorIndex * 4 + 1];
						pixels[imageIndex + 2] = colorValues[colorIndex * 4 + 2];
						pixels[imageIndex + 3] = colorValues[colorIndex * 4 + 3];
					}
				}

				offset += 8;
			}
		}*/

		SDL_RWclose(src);

		free(buffer);
		free(header);

		return image;
	}

	//-----------------------------------------------------------------------

	//This is a windows implementation of this...I think.
	void cLowLevelResourcesSDL::FindFilesInDir(tWStringList &alstStrings,tWString asDir, tWString asMask)
	{
		Platform::FindFileInDir(alstStrings, asDir,asMask);
	}

	void cLowLevelResourcesSDL::FindFilesInDirRecursive(tFilePathMap& alstStrings, tWString asDir)
	{
		Platform::FindFilesInDirRecursive(alstStrings, asDir);
	}

	//-----------------------------------------------------------------------

}
