/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.
 */

#pragma once

#include "../IAudioChunk.h"
#include "../IWorldListener.h"

namespace spades {
	namespace client {
		class Client;
		class Player;

		class ExtraFeedbackEffects {
			Client& client;
			float lastKillFlashTime = -100.0F;
			bool lastKillFlashHeadshot = false;
			int customMultiKillCount = 0;
			float customMultiKillLastTime = -100.0F;

			Handle<IAudioChunk> customHeadshotSound;
			Handle<IAudioChunk> customKnifeSound;
			Handle<IAudioChunk> customGrenadeSound;
			Handle<IAudioChunk> customDoubleKillSound;
			Handle<IAudioChunk> customTripleKillSound;
			Handle<IAudioChunk> customMultiKillSound;
			Handle<IAudioChunk> customUltraKillSound;
			Handle<IAudioChunk> customGodlikeSound;

		public:
			explicit ExtraFeedbackEffects(Client& client);

			void ResetForWorld();
			void LoadSounds();
			void OnLocalPlayerDied();
			void OnLocalPlayerKilled(Player& killer, Player& victim, KillType killType);
			void DrawKillFlash();
		};
	} // namespace client
} // namespace spades
