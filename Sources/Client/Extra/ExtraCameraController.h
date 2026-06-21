/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.
 */

#pragma once

namespace spades {
	namespace client {
		class Client;

		class ExtraCameraController {
			Client& client;
			bool noclipEnabled = false;
			float adsZoomScale = 1.0F;
			bool adsZoomActiveLastFrame = false;

		public:
			explicit ExtraCameraController(Client& client);

			void Reset();
			bool IsNoclipEnabled() const { return noclipEnabled; }
			void ToggleNoclip();
			bool UpdateNoclipCamera(float dt);
			void UpdateAdsZoomState();
			bool HandleAdsWheel(const char* name);
			float ApplyAdsZoomScale(float zoom) const;
		};
	} // namespace client
} // namespace spades
