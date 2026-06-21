/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.
 */

#include "ExtraGrenadePreview.h"

#include "../Client.h"
#include "../GameMap.h"
#include "../Grenade.h"
#include "../IRenderer.h"
#include "../Player.h"
#include "../World.h"
#include <Core/Settings.h>
#include <algorithm>

DEFINE_SPADES_SETTING(cg_grenadeTrajectory, "1");
DEFINE_SPADES_SETTING(cg_grenadeTrajectoryStep, "0.033");
DEFINE_SPADES_SETTING(cg_grenadeTrajectoryMaxPoints, "96");
DEFINE_SPADES_SETTING(cg_grenadeTrail, "1");
DEFINE_SPADES_SETTING(cg_grenadeTrailPersist, "5");
DEFINE_SPADES_SETTING(cg_grenadeTrailMaxPoints, "96");

namespace spades {
	namespace client {
		namespace {
			constexpr float kGrenadeFuseSeconds = 3.0F;
			constexpr float kGrenadePhysicsScale = 32.0F;
			constexpr float kGrenadeBounceDamping = 0.36F;
			constexpr float kPreviewLineHalfWidth = 0.035F;
			constexpr float kPreviewLineLift = 0.025F;

			bool SimulateGrenadeStep(const Handle<GameMap>& map, Vector3& pos, Vector3& vel,
			                         float dt) {
				Vector3 oldPos = pos;
				float f = dt * kGrenadePhysicsScale;
				vel.z += dt;
				pos += vel * f;

				IntVector3 lp = pos.Floor();
				IntVector3 lp2 = oldPos.Floor();

				if (map->ClipWorld(lp.x, lp.y, lp.z)) {
					if (lp.z != lp2.z &&
					    ((lp.x == lp2.x && lp.y == lp2.y) || !map->ClipWorld(lp.x, lp.y, lp2.z)))
						vel.z = -vel.z;
					else if (lp.x != lp2.x &&
					         ((lp.y == lp2.y && lp.z == lp2.z) || !map->ClipWorld(lp2.x, lp.y, lp.z)))
						vel.x = -vel.x;
					else if (lp.y != lp2.y &&
					         ((lp.x == lp2.x && lp.z == lp2.z) || !map->ClipWorld(lp.x, lp2.y, lp.z)))
						vel.y = -vel.y;

					pos = oldPos;
					vel *= kGrenadeBounceDamping;
					return true;
				}

				return false;
			}

			Vector4 MixTrajectoryColor(Vector4 a, Vector4 b, float t) {
				t = Clamp(t, 0.0F, 1.0F);
				return MakeVector4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
				                   a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
			}

			Vector4 GrenadeTrajectoryColor(bool impactPreview, float t, float alpha) {
				Vector4 start = impactPreview ? MakeVector4(0.15F, 0.85F, 1.0F, alpha)
				                              : MakeVector4(0.35F, 1.0F, 0.35F, alpha);
				Vector4 mid = impactPreview ? MakeVector4(1.0F, 0.65F, 0.15F, alpha)
				                            : MakeVector4(1.0F, 0.95F, 0.25F, alpha);
				Vector4 end = MakeVector4(1.0F, 0.15F, 0.08F, alpha);
				return t < 0.55F ? MixTrajectoryColor(start, mid, t / 0.55F)
				                 : MixTrajectoryColor(mid, end, (t - 0.55F) / 0.45F);
			}

			void AddThickDebugLine(IRenderer& renderer, Vector3 a, Vector3 b, Vector4 color) {
				Vector3 dir = b - a;
				if (dir.GetSquaredLength() < 0.0001F)
					return;

				dir = dir.Normalize();
				Vector3 side = Vector3::Cross(dir, MakeVector3(0, 0, 1));
				if (side.GetSquaredLength() < 0.0001F)
					side = Vector3::Cross(dir, MakeVector3(0, 1, 0));
				side = side.Normalize() * kPreviewLineHalfWidth;

				Vector3 lift = MakeVector3(0, 0, kPreviewLineLift);
				Vector4 soft = MakeVector4(color.x, color.y, color.z, color.w * 0.55F);
				renderer.AddDebugLine(a, b, color);
				renderer.AddDebugLine(a + side, b + side, soft);
				renderer.AddDebugLine(a - side, b - side, soft);
				renderer.AddDebugLine(a + lift, b + lift, soft);
				renderer.AddDebugLine(a - lift, b - lift, soft);
			}

			void AddGrenadeEndpointMarker(IRenderer& renderer, Vector3 end, bool impactPreview) {
				float m = impactPreview ? 0.34F : 0.26F;
				Vector4 core = impactPreview ? MakeVector4(1.0F, 0.18F, 0.05F, 1.0F)
				                             : MakeVector4(1.0F, 0.45F, 0.08F, 0.95F);
				Vector4 halo = impactPreview ? MakeVector4(1.0F, 0.75F, 0.15F, 0.65F)
				                             : MakeVector4(1.0F, 0.95F, 0.25F, 0.55F);

				AddThickDebugLine(renderer, end + MakeVector3(-m, 0, 0),
				                  end + MakeVector3(m, 0, 0), core);
				AddThickDebugLine(renderer, end + MakeVector3(0, -m, 0),
				                  end + MakeVector3(0, m, 0), core);
				AddThickDebugLine(renderer, end + MakeVector3(0, 0, -m),
				                  end + MakeVector3(0, 0, m), halo);
				AddThickDebugLine(renderer, end + MakeVector3(-m, -m, 0) * 0.7F,
				                  end + MakeVector3(m, m, 0) * 0.7F, halo);
				AddThickDebugLine(renderer, end + MakeVector3(-m, m, 0) * 0.7F,
				                  end + MakeVector3(m, -m, 0) * 0.7F, halo);
			}
		}

		ExtraGrenadePreview::ExtraGrenadePreview(Client& client) : client(client) {}

		void ExtraGrenadePreview::ResetForWorld() {
			liveGrenadeTrails.clear();
			persistedGrenadeTrails.clear();
		}

		void ExtraGrenadePreview::DrawLiveGrenadeTrails() {
			if (!cg_grenadeTrail || !client.world)
				return;

			IRenderer& renderer = client.GetRenderer();
			std::vector<const Grenade*> seen;
			int maxTrailPoints = std::max(8, (int)cg_grenadeTrailMaxPoints);
			for (const auto& nade : client.world->GetAllGrenades()) {
				const Grenade* key = nade.get();
				seen.push_back(key);

				auto& points = liveGrenadeTrails[key];
				points.push_back(nade->GetPosition());
				if ((int)points.size() > maxTrailPoints)
					points.erase(points.begin(), points.begin() + ((int)points.size() - maxTrailPoints));

				for (size_t i = 1; i < points.size(); ++i) {
					float a = (float)i / (float)points.size();
					renderer.AddDebugLine(points[i - 1], points[i], MakeVector4(1.0F, 0.6F, 0.2F, a));
				}
			}

			float persistSecs = std::max(0.0F, (float)cg_grenadeTrailPersist);
			for (auto it = liveGrenadeTrails.begin(); it != liveGrenadeTrails.end();) {
				if (std::find(seen.begin(), seen.end(), it->first) == seen.end()) {
					if (!it->second.empty() && persistSecs > 0.0F)
						persistedGrenadeTrails.push_back({it->second, client.time + persistSecs});
					it = liveGrenadeTrails.erase(it);
				} else {
					++it;
				}
			}

			for (auto it = persistedGrenadeTrails.begin(); it != persistedGrenadeTrails.end();) {
				if (client.time >= it->expireTime || it->points.size() < 2) {
					it = persistedGrenadeTrails.erase(it);
					continue;
				}
				float lifeAlpha = Clamp((it->expireTime - client.time) /
				                        std::max(0.01F, persistSecs), 0.0F, 1.0F);
				for (size_t i = 1; i < it->points.size(); ++i) {
					float a = lifeAlpha * ((float)i / (float)it->points.size());
					renderer.AddDebugLine(it->points[i - 1], it->points[i],
					                       MakeVector4(1.0F, 0.4F, 0.15F, a));
				}
				++it;
			}
		}

		void ExtraGrenadePreview::DrawLocalTrajectory(Player& player) {
			if (!cg_grenadeTrajectory || !player.IsAlive() || !player.IsToolGrenade() ||
			    !player.IsCookingGrenade())
				return;

			float dt = Clamp((float)cg_grenadeTrajectoryStep, 0.01F, 0.1F);
			int maxPoints = std::max(8, (int)cg_grenadeTrajectoryMaxPoints);
			bool impactPreview = player.GetWeaponInput().secondary;

			Vector3 dir = player.GetFront();
			Vector3 pos = player.GetEye() + (dir * 0.1F);
			Vector3 vel = dir + player.GetVelocity();
			std::vector<Vector3> pts;
			pts.reserve((size_t)maxPoints);
			pts.push_back(pos);

			for (float t = 0.0F; t < kGrenadeFuseSeconds && (int)pts.size() < maxPoints; t += dt) {
				bool hit = SimulateGrenadeStep(client.map, pos, vel, dt);
				pts.push_back(pos);
				if (impactPreview && hit)
					break;
			}

			IRenderer& renderer = client.GetRenderer();
			for (size_t i = 1; i < pts.size(); ++i) {
				if ((i & 1) == 0)
					continue;
				float t = (float)i / (float)pts.size();
				float alpha = 0.45F + 0.5F * t;
				AddThickDebugLine(renderer, pts[i - 1], pts[i],
				                  GrenadeTrajectoryColor(impactPreview, t, alpha));
			}
			if (!pts.empty())
				AddGrenadeEndpointMarker(renderer, pts.back(), impactPreview);
		}

		void ExtraGrenadePreview::DrawWorldTrails() {
			DrawLiveGrenadeTrails();
		}

		void ExtraGrenadePreview::DrawLocalTrajectory() {
			if (!client.world)
				return;
			if (stmp::optional<Player&> maybePlayer = client.world->GetLocalPlayer())
				DrawLocalTrajectory(maybePlayer.value());
		}
	} // namespace client
} // namespace spades
