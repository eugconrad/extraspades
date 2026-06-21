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
		class Tracer;

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
			void ClearLocalEntities();
			void LoadFeedbackSounds();
			void ClearTransientInputState();
			bool HandleInputKey(const std::string& name, bool down);
			void ControllerAxisEvent(float moveX, float moveY, float lookX, float lookY);
			void Update(float dt);

			bool IsNoclipEnabled() const;
			void ToggleNoclip();
			bool UpdateNoclipCamera(float dt);
			bool HandleAdsWheel(const std::string& name);
			float ApplyAdsZoomScale(float zoom) const;

			void MergeGamepadPlayerInput(PlayerInput& input) const;
			void MergeGamepadWeaponInput(WeaponInput& input) const;
			bool IsGamepadReloadKeyPressed() const;

			void RegisterTracer(int playerId, Tracer* tracer);
			void UnregisterTracer(int playerId, Tracer* tracer);
			void AddConfiguredBulletTrail(int ownerPlayerId, Vector3 startPos, Vector3 endPos,
			                              Vector4 color, WeaponType weaponType);

			void DrawGrenadeWorldTrails();
			void DrawGrenadeTrajectory();
			void OnLocalPlayerDied();
			void OnLocalPlayerKilled(Player& killer, Player& victim, KillType killType);
			void DrawKillFlash();
		};
	} // namespace client
} // namespace spades
