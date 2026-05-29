//
//  BulletTrailLog.h
//  OpenSpades
//

#pragma once

#include "ILocalEntity.h"
#include "Client.h"
#include <Core/Math.h>

namespace spades {
	namespace client {
		class BulletTrailLog : public ILocalEntity {
			Client& client;
			int ownerPlayerId;
			Vector3 startPos;
			Vector3 endPos;
			Vector4 color;
			float age = 0.0F;
			float lifeTime = 5.0F;
			bool forceExpire = false;

		public:
			BulletTrailLog(Client& client, int ownerPlayerId, Vector3 startPos, Vector3 endPos,
			               Vector4 color, float lifeTime);
			~BulletTrailLog() override;

			void ExpireNow() { forceExpire = true; }
			bool Update(float dt) override;
			void Render3D() override;
		};

		inline BulletTrailLog::BulletTrailLog(Client& client, int ownerPlayerId, Vector3 startPos,
		                                      Vector3 endPos, Vector4 color, float lifeTime)
		    : client(client), ownerPlayerId(ownerPlayerId), startPos(startPos), endPos(endPos),
		      color(color), lifeTime(lifeTime) {}

		inline BulletTrailLog::~BulletTrailLog() { client.UnregisterTrailLog(ownerPlayerId, this); }

		inline bool BulletTrailLog::Update(float dt) {
			age += dt;
			return !(forceExpire || age > lifeTime);
		}

		inline void BulletTrailLog::Render3D() {
			float alpha = Clamp(1.0F - (age / lifeTime), 0.0F, 1.0F);
			if (alpha <= 0.0F)
				return;

			Vector4 drawColor = color;
			drawColor.w = 0.55F * alpha;
			client.GetRenderer().AddDebugLine(startPos, endPos, drawColor);
		}
	} // namespace client
} // namespace spades
