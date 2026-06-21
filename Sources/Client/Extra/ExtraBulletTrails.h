/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.
 */

#pragma once

#include <deque>
#include <unordered_map>

#include "../Weapon.h"
#include <Core/Math.h>

namespace spades {
	namespace client {
		class Client;
		class Tracer;

		class ExtraBulletTrailLogManager;

		class ExtraBulletTrails {
			Client& client;
			std::unordered_map<int, std::deque<Tracer*>> tracersByPlayer;
			ExtraBulletTrailLogManager* trailLogManager = nullptr;

			ExtraBulletTrailLogManager& EnsureTrailLogManager();

		public:
			explicit ExtraBulletTrails(Client& client);

			void ResetForWorld();
			void ClearLocalEntities();
			void RegisterTracer(int playerId, Tracer* tracer);
			void UnregisterTracer(int playerId, Tracer* tracer);
			void AddTrail(int ownerPlayerId, Vector3 startPos, Vector3 endPos, Vector4 color,
			              float lifeTime, WeaponType weaponType, int maxPerPlayer);
			void AddConfiguredTrail(int ownerPlayerId, Vector3 startPos, Vector3 endPos,
			                        Vector4 color, WeaponType weaponType);
		};
	} // namespace client
} // namespace spades
