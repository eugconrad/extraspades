//
//  Tracer.h
//  OpenSpades
//
//  Created by Tomoaki Kawada on 8/30/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "ILocalEntity.h"
#include <Core/Math.h>
#include <Core/RefCountedObject.h>

namespace spades {
	namespace client {
		class Client;
		class IImage;
		class Tracer : public ILocalEntity {
			Client& client;
			Handle<IImage> image;
			Vector3 startPos, dir;
			float length;
			float curDistance = 0.0F;
			float visibleLength = 0.0F;
			float velocity = 0.0F;
			float age = 0.0F;
			float holdTime = 3.0F;
			float fadeTime = 1.0F;
			bool firstUpdate = true;
			bool shotgun;
			bool forceExpire = false;
			int ownerPlayerId;
			Vector4 tracerColor;

		public:
			Tracer(Client&, int ownerPlayerId, Vector3 p1, Vector3 p2, float bulletVel,
			       bool shotgun, Vector4 color);
			~Tracer();
			void ExpireNow() { forceExpire = true; }

			bool Update(float dt) override;
			void Render3D() override;
		};
	} // namespace client
} // namespace spades
