/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.
 */

#include "ExtraFeedbackEffects.h"

#include "../Client.h"
#include "../IAudioDevice.h"
#include "../IRenderer.h"
#include "../Player.h"
#include <Core/FileManager.h>
#include <Core/Settings.h>
#include <algorithm>
#include <string>

DEFINE_SPADES_SETTING(cg_customKillSounds, "0");
DEFINE_SPADES_SETTING(cg_customKillSoundsGain, "1.0");
DEFINE_SPADES_SETTING(cg_customMultiKillWindow, "8.0");
DEFINE_SPADES_SETTING(cg_killFlash, "1");
DEFINE_SPADES_SETTING(cg_killFlashDuration, "0.18");
DEFINE_SPADES_SETTING(cg_killFlashAlpha, "1.0");

namespace spades {
	namespace client {
		namespace {
			constexpr float kDefaultMultiKillWindow = 8.0F;
			constexpr float kMinimumKillFlashDuration = 0.02F;
			constexpr float kDefaultKillFlashDuration = 0.18F;

			Handle<IAudioChunk> RegisterOptionalCustomSound(IAudioDevice& audioDevice,
			                                                const char* baseName) {
				std::string wavPath = std::string("Sounds/Feedback/CustomKill/") + baseName + ".wav";
				if (FileManager::FileExists(wavPath.c_str()))
					return audioDevice.RegisterSound(wavPath.c_str());

				std::string opusPath = std::string("Sounds/Feedback/CustomKill/") + baseName + ".opus";
				if (FileManager::FileExists(opusPath.c_str()))
					return audioDevice.RegisterSound(opusPath.c_str());

				SPLog("CustomKill sound not found: %s(.wav/.opus)", baseName);
				return Handle<IAudioChunk>{};
			}
		}

		ExtraFeedbackEffects::ExtraFeedbackEffects(Client& client) : client(client) {}

		void ExtraFeedbackEffects::ResetForWorld() {
			lastKillFlashTime = -100.0F;
			lastKillFlashHeadshot = false;
			customMultiKillCount = 0;
			customMultiKillLastTime = -100.0F;
		}

		void ExtraFeedbackEffects::LoadSounds() {
			customHeadshotSound = RegisterOptionalCustomSound(*client.audioDevice, "headshot");
			customKnifeSound = RegisterOptionalCustomSound(*client.audioDevice, "knife");
			customGrenadeSound = RegisterOptionalCustomSound(*client.audioDevice, "grenade");
			customDoubleKillSound = RegisterOptionalCustomSound(*client.audioDevice, "doublekill");
			customTripleKillSound = RegisterOptionalCustomSound(*client.audioDevice, "triplekill");
			customMultiKillSound = RegisterOptionalCustomSound(*client.audioDevice, "multikill");
			customUltraKillSound = RegisterOptionalCustomSound(*client.audioDevice, "ultrakill");
			customGodlikeSound = RegisterOptionalCustomSound(*client.audioDevice, "godlike");
		}

		void ExtraFeedbackEffects::OnLocalPlayerDied() {
			customMultiKillCount = 0;
			customMultiKillLastTime = -100.0F;
		}

		void ExtraFeedbackEffects::OnLocalPlayerKilled(Player& killer, Player& victim,
		                                               KillType killType) {
			lastKillFlashTime = client.time;
			lastKillFlashHeadshot = (killType == KillTypeHeadshot);

			if (!cg_customKillSounds || client.IsMuted() || killer.GetId() == victim.GetId() ||
			    killer.IsTeammate(victim))
				return;

			AudioParam param;
			param.volume = std::max(0.0F, (float)cg_customKillSoundsGain);

			Handle<IAudioChunk> actionSound;
			switch (killType) {
				case KillTypeHeadshot: actionSound = customHeadshotSound; break;
				case KillTypeMelee: actionSound = customKnifeSound; break;
				case KillTypeGrenade: actionSound = customGrenadeSound; break;
				default: break;
			}
			if (actionSound)
				client.audioDevice->PlayLocal(actionSound.GetPointerOrNull(), param);

			float window = (float)cg_customMultiKillWindow;
			if (!(window > 0.0F))
				window = kDefaultMultiKillWindow;
			if (client.time - customMultiKillLastTime <= window)
				customMultiKillCount++;
			else
				customMultiKillCount = 1;
			customMultiKillLastTime = client.time;

			Handle<IAudioChunk> multiSound;
			switch (customMultiKillCount) {
				case 2: multiSound = customDoubleKillSound; break;
				case 3: multiSound = customTripleKillSound; break;
				case 4: multiSound = customMultiKillSound; break;
				case 5: multiSound = customUltraKillSound; break;
				default: break;
			}
			if (multiSound)
				client.audioDevice->PlayLocal(multiSound.GetPointerOrNull(), param);

			if (client.curStreak == 10 && customGodlikeSound)
				client.audioDevice->PlayLocal(customGodlikeSound.GetPointerOrNull(), param);
		}

		void ExtraFeedbackEffects::DrawKillFlash() {
			if (!cg_killFlash)
				return;

			float elapsed = client.time - lastKillFlashTime;
			float duration = cg_killFlashDuration;
			float baseAlpha = cg_killFlashAlpha;
			if (!(duration > kMinimumKillFlashDuration))
				duration = kDefaultKillFlashDuration;
			baseAlpha = Clamp(baseAlpha, 0.0F, 1.0F);
			if (elapsed < 0.0F || elapsed >= duration)
				return;

			float t = 1.0F - (elapsed / duration);
			t = Clamp(t, 0.0F, 1.0F);

			Vector4 flashColor = lastKillFlashHeadshot
				? MakeVector4(1.0F, 0.95F, 0.75F, 1.0F)
				: MakeVector4(1.0F, 1.0F, 1.0F, 1.0F);
			float alpha = baseAlpha * t;
			IRenderer& renderer = client.GetRenderer();
			renderer.SetColorAlphaPremultiplied(flashColor * alpha);
			renderer.DrawImage(nullptr, AABB2(0, 0, renderer.ScreenWidth(), renderer.ScreenHeight()));
		}
	} // namespace client
} // namespace spades
