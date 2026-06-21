/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.
 */

#include "ExtraClientFeatures.h"

#include "../Client.h"

namespace spades {
	namespace client {
		ExtraClientFeatures::ExtraClientFeatures(Client& client)
		    : client(client), gamepadInput(client), cameraController(client),
		      bulletTrails(client), debugVisuals(client), feedbackEffects(client),
		      grenadePreview(client) {}

		void ExtraClientFeatures::ResetForWorld() {
			gamepadInput.Clear();
			cameraController.Reset();
			bulletTrails.ResetForWorld();
			feedbackEffects.ResetForWorld();
			grenadePreview.ResetForWorld();
		}

		void ExtraClientFeatures::ClearTransientInputState() {
			gamepadInput.Clear();
		}

		bool ExtraClientFeatures::HandleInputKey(const std::string& name, bool down) {
			if (gamepadInput.HandleAction(name, down))
				return true;
			return debugVisuals.HandleKeyEvent(name, down);
		}

		void ExtraClientFeatures::ControllerAxisEvent(float moveX, float moveY,
		                                             float lookX, float lookY) {
			gamepadInput.ControllerAxisEvent(moveX, moveY, lookX, lookY);
		}

		void ExtraClientFeatures::Update(float) {
			cameraController.UpdateAdsZoomState();
		}
	} // namespace client
} // namespace spades
