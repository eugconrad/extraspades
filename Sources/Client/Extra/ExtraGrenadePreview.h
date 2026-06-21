/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.
 */

#pragma once

#include <unordered_map>
#include <vector>

#include <Core/Math.h>

namespace spades {
	namespace client {
		class Client;
		class Grenade;
		class IRenderer;
		class Player;

		class ExtraGrenadePreview {
			struct PersistedGrenadeTrail {
				std::vector<Vector3> points;
				float expireTime;
			};

			Client& client;
			std::unordered_map<const Grenade*, std::vector<Vector3>> liveGrenadeTrails;
			std::vector<PersistedGrenadeTrail> persistedGrenadeTrails;

			void DrawLiveGrenadeTrails();
			void DrawLocalTrajectory(Player& player);

		public:
			explicit ExtraGrenadePreview(Client& client);

			void ResetForWorld();
			void DrawWorldTrails();
			void DrawLocalTrajectory();
		};
	} // namespace client
} // namespace spades
