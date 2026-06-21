/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.
 */

#include "ExtraCameraController.h"

#include "../Client.h"
#include "../IAudioChunk.h"
#include "../IAudioDevice.h"
#include "../World.h"
#include <Core/Settings.h>
#include <algorithm>
#include <cmath>
#include <string>

DEFINE_SPADES_SETTING(cg_adsZoomMin, "1.0");
DEFINE_SPADES_SETTING(cg_adsZoomMax, "5.0");
DEFINE_SPADES_SETTING(cg_adsZoomStep, "0.25");

namespace spades {
	namespace client {
		namespace {
			constexpr float kNoclipBaseSpeed = 32.0F;
			constexpr float kNoclipSprintScale = 3.0F;
			constexpr float kNoclipSneakScale = 0.5F;
		}

		ExtraCameraController::ExtraCameraController(Client& client) : client(client) {}

		void ExtraCameraController::Reset() {
			noclipEnabled = false;
			adsZoomScale = 1.0F;
			adsZoomActiveLastFrame = false;
		}

		void ExtraCameraController::ToggleNoclip() {
			noclipEnabled = !noclipEnabled;
			client.ClearTransientInputState();

			if (noclipEnabled) {
				client.freeCameraState.position = client.lastSceneDef.viewOrigin;
				client.freeCameraState.velocity = MakeVector3(0.0F, 0.0F, 0.0F);

				Vector3 o = -client.lastSceneDef.viewAxis[2];
				client.followAndFreeCameraState.yaw = atan2f(o.y, o.x);
				client.followAndFreeCameraState.pitch = -atan2f(o.z, o.GetLength2D());
				client.followCameraState.enabled = false;
			}

			client.ShowAlert(noclipEnabled ? "Noclip enabled" : "Noclip disabled",
			                 Client::AlertType::Notice);
			Handle<IAudioChunk> c = client.audioDevice->RegisterSound("Sounds/Player/Flashlight.opus");
			client.audioDevice->PlayLocal(c.GetPointerOrNull(), AudioParam());
		}

		bool ExtraCameraController::UpdateNoclipCamera(float dt) {
			if (!noclipEnabled)
				return false;

			auto& freeState = client.freeCameraState;
			auto& sharedState = client.followAndFreeCameraState;

			Vector3 up = {0.0F, 0.0F, -1.0F};
			Vector3 front = {-cosf(sharedState.yaw), -sinf(sharedState.yaw), 0.0F};
			Vector3 right = -Vector3::Cross(up, front).Normalize();

			Vector3 move = {0.0F, 0.0F, 0.0F};
			if (client.playerInput.moveForward)
				move += front;
			if (client.playerInput.moveBackward)
				move -= front;
			if (client.playerInput.moveLeft)
				move -= right;
			if (client.playerInput.moveRight)
				move += right;
			if (client.playerInput.jump)
				move += up;
			if (client.playerInput.crouch)
				move -= up;

			float speed = kNoclipBaseSpeed;
			if (client.playerInput.sprint)
				speed *= kNoclipSprintScale;
			else if (client.playerInput.sneak)
				speed *= kNoclipSneakScale;

			if (move.GetSquaredLength() > 0.0F)
				move = move.Normalize() * speed;

			freeState.velocity = move;
			freeState.position += move * dt;
			return true;
		}

		void ExtraCameraController::UpdateAdsZoomState() {
			float minScale = cg_adsZoomMin;
			if (!(minScale > 0.01F))
				minScale = 1.0F;

			bool adsActive = false;
			if (client.world) {
				if (stmp::optional<Player&> maybePlayer = client.world->GetLocalPlayer()) {
					Player& player = maybePlayer.value();
					adsActive = player.IsAlive() && !player.IsSpectator() &&
					            player.IsToolWeapon() && player.GetWeaponInput().secondary;
				}
			}

			if (adsZoomActiveLastFrame && !adsActive)
				adsZoomScale = minScale;

			adsZoomActiveLastFrame = adsActive;
		}

		bool ExtraCameraController::HandleAdsWheel(const char* name) {
			if (!client.world)
				return false;

			stmp::optional<Player&> maybePlayer = client.world->GetLocalPlayer();
			if (!maybePlayer)
				return false;

			Player& p = maybePlayer.value();
			if (p.IsSpectator() || !p.IsAlive() || !p.IsToolWeapon() ||
			    !p.GetWeaponInput().secondary)
				return false;

			float minScale = cg_adsZoomMin;
			float maxScale = cg_adsZoomMax;
			float step = cg_adsZoomStep;

			if (!(minScale > 0.01F))
				minScale = 1.0F;
			if (!(maxScale >= minScale))
				maxScale = minScale;
			if (!(step > 0.0F))
				step = 0.05F;

			if (std::string(name) == "WheelUp")
				adsZoomScale = std::min(adsZoomScale + step, maxScale);
			else
				adsZoomScale = std::max(adsZoomScale - step, minScale);
			return true;
		}

		float ExtraCameraController::ApplyAdsZoomScale(float zoom) const {
			float minScale = cg_adsZoomMin;
			float maxScale = cg_adsZoomMax;
			if (!(minScale > 0.01F))
				minScale = 1.0F;
			if (!(maxScale >= minScale))
				maxScale = minScale;
			return zoom * Clamp(adsZoomScale, minScale, maxScale);
		}
	} // namespace client
} // namespace spades
