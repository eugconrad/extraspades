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

		void ExtraClientFeatures::ClearLocalEntities() {
			bulletTrails.ClearLocalEntities();
		}

		void ExtraClientFeatures::LoadFeedbackSounds() {
			feedbackEffects.LoadSounds();
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

		bool ExtraClientFeatures::IsNoclipEnabled() const {
			return cameraController.IsNoclipEnabled();
		}

		void ExtraClientFeatures::ToggleNoclip() {
			cameraController.ToggleNoclip();
		}

		bool ExtraClientFeatures::UpdateNoclipCamera(float dt) {
			return cameraController.UpdateNoclipCamera(dt);
		}

		bool ExtraClientFeatures::HandleAdsWheel(const std::string& name) {
			return cameraController.HandleAdsWheel(name.c_str());
		}

		float ExtraClientFeatures::ApplyAdsZoomScale(float zoom) const {
			return cameraController.ApplyAdsZoomScale(zoom);
		}

		void ExtraClientFeatures::MergeGamepadPlayerInput(PlayerInput& input) const {
			gamepadInput.MergePlayerInput(input);
		}

		void ExtraClientFeatures::MergeGamepadWeaponInput(WeaponInput& input) const {
			gamepadInput.MergeWeaponInput(input);
		}

		bool ExtraClientFeatures::IsGamepadReloadKeyPressed() const {
			return gamepadInput.IsReloadKeyPressed();
		}

		void ExtraClientFeatures::RegisterTracer(int playerId, Tracer* tracer) {
			bulletTrails.RegisterTracer(playerId, tracer);
		}

		void ExtraClientFeatures::UnregisterTracer(int playerId, Tracer* tracer) {
			bulletTrails.UnregisterTracer(playerId, tracer);
		}

		void ExtraClientFeatures::AddConfiguredBulletTrail(int ownerPlayerId,
		                                                   Vector3 startPos,
		                                                   Vector3 endPos,
		                                                   Vector4 color,
		                                                   WeaponType weaponType) {
			bulletTrails.AddConfiguredTrail(ownerPlayerId, startPos, endPos, color, weaponType);
		}

		void ExtraClientFeatures::DrawGrenadeWorldTrails() {
			grenadePreview.DrawWorldTrails();
		}

		void ExtraClientFeatures::DrawGrenadeTrajectory() {
			grenadePreview.DrawLocalTrajectory();
		}

		void ExtraClientFeatures::OnLocalPlayerDied() {
			feedbackEffects.OnLocalPlayerDied();
		}

		void ExtraClientFeatures::OnLocalPlayerKilled(Player& killer, Player& victim,
		                                              KillType killType) {
			feedbackEffects.OnLocalPlayerKilled(killer, victim, killType);
		}

		void ExtraClientFeatures::DrawKillFlash() {
			feedbackEffects.DrawKillFlash();
		}
	} // namespace client
} // namespace spades
