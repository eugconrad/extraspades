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

#include "BlockTextureMapper.h"

#include <algorithm>
#include <cctype>
#include <cmath>

#include "GLImage.h"
#include "IGLDevice.h"
#include <Core/Bitmap.h>
#include <Core/BitmapAtlasGenerator.h>
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/FileManager.h>

namespace spades {
	namespace draw {
		namespace {
			const char* BlockTexturePath = "Textures/Blocks";
			const int AtlasPadding = 4;
			const float TextureMatchLightnessBias = 0.16F;
			const float TextureMatchLuminanceWeight = 1.5F;
			const float TextureMatchChromaWeight = 0.5F;

			BlockTextureRef InvalidBlockTextureRef() {
				BlockTextureRef ref = {0, 0, 0, 0, false};
				return ref;
			}

			int LightenForTextureMatch(uint8_t value) {
				float channel = static_cast<float>(value);
				return static_cast<int>(channel + (255.0F - channel) * TextureMatchLightnessBias +
				                        0.5F);
			}

			float Luminance(int r, int g, int b) {
				return 0.2126F * static_cast<float>(r) + 0.7152F * static_cast<float>(g) +
				       0.0722F * static_cast<float>(b);
			}

			int Chroma(int r, int g, int b) {
				return std::max(r, std::max(g, b)) - std::min(r, std::min(g, b));
			}

			float TextureMatchDistance(int targetR, int targetG, int targetB, int textureR,
			                           int textureG, int textureB) {
				const int dr = targetR - textureR;
				const int dg = targetG - textureG;
				const int db = targetB - textureB;
				const float rgbDistance = static_cast<float>(dr * dr + dg * dg + db * db);

				const float luminanceDelta = Luminance(targetR, targetG, targetB) -
				                             Luminance(textureR, textureG, textureB);
				const int chromaDelta = Chroma(targetR, targetG, targetB) -
				                        Chroma(textureR, textureG, textureB);
				return rgbDistance + luminanceDelta * luminanceDelta * TextureMatchLuminanceWeight +
				       static_cast<float>(chromaDelta * chromaDelta) * TextureMatchChromaWeight;
			}
		} // namespace

		BlockTextureMapper::BlockTextureMapper(IGLDevice& device) : device(device) {
			SPADES_MARK_FUNCTION();
			Load();
		}

		uint32_t BlockTextureMapper::ColorKey(uint8_t r, uint8_t g, uint8_t b) {
			return (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
		}

		uint16_t BlockTextureMapper::PackUV(float value) {
			value = std::max(0.0F, std::min(1.0F, value));
			return static_cast<uint16_t>(floorf(value * 65535.0F + 0.5F));
		}

		bool BlockTextureMapper::IsPngFile(const std::string& name) {
			if (name.size() < 4)
				return false;
			std::string ext = name.substr(name.size() - 4);
			std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			return ext == ".png";
		}

		void BlockTextureMapper::ComputeAverageRGB(Bitmap& bitmap, uint8_t& r, uint8_t& g, uint8_t& b) {
			SPADES_MARK_FUNCTION_DEBUG();

			uint64_t sumR = 0;
			uint64_t sumG = 0;
			uint64_t sumB = 0;
			uint64_t sumA = 0;

			const int width = bitmap.GetWidth();
			const int height = bitmap.GetHeight();
			const uint32_t* pixels = bitmap.GetPixels();
			for (int i = 0; i < width * height; i++) {
				uint32_t pixel = pixels[i];
				uint8_t alpha = static_cast<uint8_t>(pixel >> 24);
				sumR += static_cast<uint8_t>(pixel) * uint64_t(alpha);
				sumG += static_cast<uint8_t>(pixel >> 8) * uint64_t(alpha);
				sumB += static_cast<uint8_t>(pixel >> 16) * uint64_t(alpha);
				sumA += alpha;
			}

			if (sumA == 0)
				sumA = uint64_t(width) * uint64_t(height);

			r = static_cast<uint8_t>(sumR / sumA);
			g = static_cast<uint8_t>(sumG / sumA);
			b = static_cast<uint8_t>(sumB / sumA);
		}

		Handle<Bitmap> BlockTextureMapper::CreatePaddedBitmap(Bitmap& bitmap, int padding) {
			SPADES_MARK_FUNCTION_DEBUG();

			const int srcWidth = bitmap.GetWidth();
			const int srcHeight = bitmap.GetHeight();
			Handle<Bitmap> padded = Handle<Bitmap>::New(srcWidth + padding * 2,
			                                             srcHeight + padding * 2);

			uint32_t* outPixels = padded->GetPixels();
			const uint32_t* inPixels = bitmap.GetPixels();
			const int outWidth = padded->GetWidth();
			const int outHeight = padded->GetHeight();

			for (int y = 0; y < outHeight; y++) {
				int sy = std::max(0, std::min(srcHeight - 1, y - padding));
				for (int x = 0; x < outWidth; x++) {
					int sx = std::max(0, std::min(srcWidth - 1, x - padding));
					outPixels[x + y * outWidth] = inPixels[sx + sy * srcWidth];
				}
			}

			return padded;
		}

		void BlockTextureMapper::Load() {
			SPADES_MARK_FUNCTION();

			auto files = FileManager::EnumFiles(BlockTexturePath);
			size_t pngCount = 0;
			for (const auto& file : files) {
				if (IsPngFile(file))
					pngCount++;
			}

			SPLog("Textured blocks: found %d PNG file(s) in Resources/%s",
			      static_cast<int>(pngCount), BlockTexturePath);

			for (const auto& file : files) {
				if (!IsPngFile(file))
					continue;

				const std::string path = std::string(BlockTexturePath) + "/" + file;
				try {
					Handle<Bitmap> bitmap = Bitmap::Load(path);
					uint8_t avgR = 0, avgG = 0, avgB = 0;
					ComputeAverageRGB(*bitmap, avgR, avgG, avgB);

					TextureEntry entry = {};
					entry.avgR = avgR;
					entry.avgG = avgG;
					entry.avgB = avgB;
					entry.ref = InvalidBlockTextureRef();
					entry.name = file;
					entry.width = bitmap->GetWidth();
					entry.height = bitmap->GetHeight();

					bitmaps.push_back(CreatePaddedBitmap(*bitmap, AtlasPadding));
					textures.push_back(entry);

					SPLog("Textured blocks: %s average RGB = %d, %d, %d",
					      path.c_str(), avgR, avgG, avgB);
				} catch (const std::exception& ex) {
					SPLog("Textured blocks: failed to load %s: %s", path.c_str(), ex.what());
				} catch (...) {
					SPLog("Textured blocks: failed to load %s", path.c_str());
				}
			}

			SPLog("Textured blocks: loaded %d PNG texture(s)", static_cast<int>(textures.size()));

			if (textures.empty()) {
				SPLog("Textured blocks: fallback to flat-color map renderer because no textures are loaded");
				return;
			}

			BitmapAtlasGenerator generator;
			for (auto& bitmap : bitmaps)
				generator.AddBitmap(bitmap.GetPointerOrNull());

			BitmapAtlasGenerator::Result result = generator.Pack();
			Handle<Bitmap> atlasBitmap(result.bitmap, false);
			const float atlasWidth = static_cast<float>(atlasBitmap->GetWidth());
			const float atlasHeight = static_cast<float>(atlasBitmap->GetHeight());

			for (size_t i = 0; i < result.items.size() && i < textures.size(); i++) {
				const auto& item = result.items[i];
				BlockTextureRef ref;
				ref.u0 = PackUV(static_cast<float>(item.x + AtlasPadding) / atlasWidth);
				ref.v0 = PackUV(static_cast<float>(item.y + AtlasPadding) / atlasHeight);
				ref.u1 = PackUV(static_cast<float>(item.x + AtlasPadding + textures[i].width) /
				                atlasWidth);
				ref.v1 = PackUV(static_cast<float>(item.y + AtlasPadding + textures[i].height) /
				                atlasHeight);
				ref.valid = true;
				textures[i].ref = ref;
			}

			atlasImage = GLImage::FromBitmap(*atlasBitmap, &device);
			SPLog("Textured blocks: built in-memory atlas %dx%d with %d tile(s)",
			      atlasBitmap->GetWidth(), atlasBitmap->GetHeight(), static_cast<int>(textures.size()));
		}

		void BlockTextureMapper::BindAtlas() {
			SPADES_MARK_FUNCTION_DEBUG();
			if (atlasImage)
				atlasImage->Bind(IGLDevice::Texture2D);
		}

		BlockTextureRef BlockTextureMapper::GetTextureForColor(uint8_t r, uint8_t g, uint8_t b) {
			SPADES_MARK_FUNCTION_DEBUG();

			if (!IsReady())
				return InvalidBlockTextureRef();

			uint32_t key = ColorKey(r, g, b);
			auto it = colorCache.find(key);
			if (it != colorCache.end())
				return it->second;

			const int targetR = LightenForTextureMatch(r);
			const int targetG = LightenForTextureMatch(g);
			const int targetB = LightenForTextureMatch(b);

			float bestDistance = 1.0e30F;
			BlockTextureRef best = InvalidBlockTextureRef();
			for (const auto& texture : textures) {
				const float dist = TextureMatchDistance(targetR, targetG, targetB, int(texture.avgR),
				                                        int(texture.avgG), int(texture.avgB));
				if (dist < bestDistance) {
					bestDistance = dist;
					best = texture.ref;
				}
			}

			colorCache[key] = best;
			return best;
		}
	} // namespace draw
} // namespace spades
