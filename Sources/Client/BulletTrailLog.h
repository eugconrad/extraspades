//
//  BulletTrailLog.h
//  OpenSpades
//

#pragma once

#include "Client.h"
#include "GameProperties.h"
#include "IRenderer.h"
#include "ILocalEntity.h"
#include <Core/Math.h>
#include <algorithm>
#include <deque>
#include <unordered_map>
#include <vector>

namespace spades {
	namespace client {
		class BulletTrailLogManager : public ILocalEntity {
			struct Segment {
				Vector3 startPos;
				Vector3 endPos;
			};

			struct TrailLog {
				int ownerPlayerId;
				WeaponType weaponType;
				Vector4 color;
				float age = 0.0F;
				float lifeTime = 5.0F;
				float holdTime = 1.25F;
				std::vector<Segment> segments;
			};

			Client& client;
			std::vector<TrailLog> trails;
			std::unordered_map<int, std::deque<std::size_t>> trailsByPlayer;

			static bool IsShotgun(WeaponType type) { return type == SHOTGUN_WEAPON; }

			void RebuildPlayerIndex() {
				trailsByPlayer.clear();
				for (std::size_t i = 0; i < trails.size(); ++i)
					trailsByPlayer[trails[i].ownerPlayerId].push_back(i);
			}

			void RemoveTrail(std::size_t index) {
				if (index >= trails.size())
					return;
				trails.erase(trails.begin() + static_cast<std::ptrdiff_t>(index));
				RebuildPlayerIndex();
			}

			TrailLog* FindRecentShotgunGroup(int ownerPlayerId) {
				for (auto it = trails.rbegin(); it != trails.rend(); ++it) {
					if (it->ownerPlayerId != ownerPlayerId)
						continue;
					if (!IsShotgun(it->weaponType))
						return nullptr;
					if (it->age <= 0.05F && it->segments.size() < 16)
						return &*it;
					return nullptr;
				}
				return nullptr;
			}

			void CullOldestForPlayer(int ownerPlayerId, int maxPerPlayer) {
				for (;;) {
					auto it = trailsByPlayer.find(ownerPlayerId);
					if (it == trailsByPlayer.end() || (int)it->second.size() <= maxPerPlayer ||
					    it->second.empty())
						break;
					RemoveTrail(it->second.front());
				}
			}

			static float TrailAlpha(const TrailLog& trail) {
				if (trail.age <= trail.holdTime)
					return 1.0F;
				float fadeTime = std::max(0.01F, trail.lifeTime - trail.holdTime);
				float t = (trail.age - trail.holdTime) / fadeTime;
				t = Clamp(t, 0.0F, 1.0F);
				return 1.0F - t;
			}

			static Vector4 TrailColor(const TrailLog& trail, float alpha, float opacity) {
				Vector4 c = trail.color;
				c.w = alpha * opacity;
				return c;
			}

			static void AddImpactCross(IRenderer& renderer, Vector3 startPos, Vector3 endPos,
			                           Vector4 color, float alpha, WeaponType weaponType) {
				Vector3 dir = endPos - startPos;
				if (dir.GetSquaredLength() < 0.0001F)
					return;
				dir = dir.Normalize();

				Vector3 side = Vector3::Cross(dir, MakeVector3(0, 0, 1));
				if (side.GetSquaredLength() < 0.0001F)
					side = Vector3::Cross(dir, MakeVector3(0, 1, 0));
				side = side.Normalize();
				Vector3 up = Vector3::Cross(side, dir).Normalize();

				float size = IsShotgun(weaponType) ? 0.08F : 0.10F;
				Vector4 marker = color;
				marker.w = alpha * 0.75F;

				Vector3 d1 = (side + up).Normalize() * size;
				Vector3 d2 = (side - up).Normalize() * size;
				renderer.AddDebugLine(endPos - d1, endPos + d1, marker);
				renderer.AddDebugLine(endPos - d2, endPos + d2, marker);
			}

		public:
			explicit BulletTrailLogManager(Client& client) : client(client) {}

			void AddTrail(int ownerPlayerId, Vector3 startPos, Vector3 endPos, Vector4 color,
			              float lifeTime, WeaponType weaponType, int maxPerPlayer) {
				if (lifeTime <= 0.0F)
					return;

				if (TrailLog* group = IsShotgun(weaponType) ? FindRecentShotgunGroup(ownerPlayerId) : nullptr) {
					group->segments.push_back({startPos, endPos});
					group->lifeTime = std::max(group->lifeTime, std::max(1.5F, lifeTime));
					group->holdTime = std::min(2.0F, group->lifeTime * 0.5F);
					return;
				}

				TrailLog trail;
				trail.ownerPlayerId = ownerPlayerId;
				trail.weaponType = weaponType;
				trail.color = color;
				trail.lifeTime = std::max(1.5F, lifeTime);
				trail.holdTime = std::min(2.0F, trail.lifeTime * 0.5F);
				trail.segments.push_back({startPos, endPos});
				trails.push_back(std::move(trail));
				RebuildPlayerIndex();

				CullOldestForPlayer(ownerPlayerId, std::max(1, maxPerPlayer));
			}

			bool Update(float dt) override {
				for (auto& trail : trails)
					trail.age += dt;

				for (std::size_t i = 0; i < trails.size();) {
					if (trails[i].age > trails[i].lifeTime)
						RemoveTrail(i);
					else
						++i;
				}
				return true;
			}

			void Render3D() override {
				IRenderer& renderer = client.GetRenderer();
				for (const auto& trail : trails) {
					float alpha = TrailAlpha(trail);
					if (alpha <= 0.0F)
						continue;

					for (const auto& segment : trail.segments) {
						renderer.AddDebugLine(segment.startPos, segment.endPos,
						                      TrailColor(trail, alpha, 0.85F));
						AddImpactCross(renderer, segment.startPos, segment.endPos, trail.color,
						               alpha, trail.weaponType);
					}
				}
			}
		};
	} // namespace client
} // namespace spades
