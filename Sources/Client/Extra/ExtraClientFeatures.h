/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.
 */

#pragma once

#include "ExtraCameraController.h"
#include "ExtraBulletTrails.h"
#include "ExtraDebugVisuals.h"
#include "ExtraFeedbackEffects.h"
#include "ExtraGamepadInput.h"
#include "ExtraGrenadePreview.h"

namespace spades {
	namespace client {
		class Client;

		class ExtraBulletTrails;
		class ExtraFeedbackEffects;
		class ExtraGrenadePreview;

		class ExtraClientFeatures {
			Client& client;
			ExtraGamepadInput gamepadInput;
			ExtraCameraController cameraController;
			ExtraBulletTrails bulletTrails;
			ExtraDebugVisuals debugVisuals;
			ExtraFeedbackEffects feedbackEffects;
			ExtraGrenadePreview grenadePreview;

		public:
			explicit ExtraClientFeatures(Client& client);

			ExtraGamepadInput& Gamepad() { return gamepadInput; }
			const ExtraGamepadInput& Gamepad() const { return gamepadInput; }
			ExtraCameraController& Camera() { return cameraController; }
			const ExtraCameraController& Camera() const { return cameraController; }
			ExtraBulletTrails& BulletTrails() { return bulletTrails; }
			const ExtraBulletTrails& BulletTrails() const { return bulletTrails; }
			ExtraFeedbackEffects& Feedback() { return feedbackEffects; }
			const ExtraFeedbackEffects& Feedback() const { return feedbackEffects; }
			ExtraGrenadePreview& GrenadePreview() { return grenadePreview; }
			const ExtraGrenadePreview& GrenadePreview() const { return grenadePreview; }

			void ResetForWorld();
			void ClearTransientInputState();
			bool HandleInputKey(const std::string& name, bool down);
			void ControllerAxisEvent(float moveX, float moveY, float lookX, float lookY);
			void Update(float dt);
		};
	} // namespace client
} // namespace spades
