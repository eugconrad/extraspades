/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "GLImage.h"
#include <Core/Bitmap.h>
#include <Core/RefCountedObject.h>

namespace spades {
	namespace draw {
		class IGLDevice;

		struct BlockTextureRef {
			uint16_t u0, v0, u1, v1;
			bool valid;
		};

		class BlockTextureMapper {
			struct TextureEntry {
				uint8_t avgR, avgG, avgB;
				BlockTextureRef ref;
				std::string name;
				int width, height;
			};

			IGLDevice& device;
			std::vector<Handle<Bitmap>> bitmaps;
			std::vector<TextureEntry> textures;
			std::unordered_map<uint32_t, BlockTextureRef> colorCache;
			Handle<GLImage> atlasImage;

			static uint32_t ColorKey(uint8_t r, uint8_t g, uint8_t b);
			static uint16_t PackUV(float value);
			static bool IsPngFile(const std::string& name);
			static void ComputeAverageRGB(Bitmap& bitmap, uint8_t& r, uint8_t& g, uint8_t& b);
			static Handle<Bitmap> CreatePaddedBitmap(Bitmap& bitmap, int padding);

			void Load();

		public:
			explicit BlockTextureMapper(IGLDevice& device);

			bool IsReady() const { return atlasImage && !textures.empty(); }
			void BindAtlas();
			BlockTextureRef GetTextureForColor(uint8_t r, uint8_t g, uint8_t b);
		};
	} // namespace draw
} // namespace spades
