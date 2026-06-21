/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.
 */

#include "ExtraGamepadInput.h"

#include "../Client.h"
#include "../IAudioChunk.h"
#include "../IAudioDevice.h"
#include "../World.h"
#include <Core/Strings.h>

namespace spades {
	namespace client {
		namespace {
			constexpr float kMoveThreshold = 0.35F;
			constexpr std::size_t kActionPrefixLength = 13;
		}

		ExtraGamepadInput::ExtraGamepadInput(Client& client) : client(client) {}

		void ExtraGamepadInput::Clear() {
			playerInput = PlayerInput();
			weaponInput = WeaponInput();
			reloadKeyPressed = false;
		}

		void ExtraGamepadInput::ControllerAxisEvent(float moveX, float moveY,
		                                           float lookX, float lookY) {
			SPADES_MARK_FUNCTION();

			if (client.NeedsAbsoluteMouseCoordinate()) {
				playerInput.moveForward = false;
				playerInput.moveBackward = false;
				playerInput.moveLeft = false;
				playerInput.moveRight = false;
				return;
			}

			playerInput.moveLeft = moveX < -kMoveThreshold;
			playerInput.moveRight = moveX > kMoveThreshold;
			playerInput.moveForward = moveY < -kMoveThreshold;
			playerInput.moveBackward = moveY > kMoveThreshold;

			if (lookX != 0.0F || lookY != 0.0F) {
				// Compatibility bridge: SDLRunner still sends controller look as view deltas.
				client.MouseEvent(lookX, lookY);
			}
		}

		bool ExtraGamepadInput::HandleAction(const std::string& name, bool down) {
			if (name.compare(0, kActionPrefixLength, "GamepadAction") != 0)
				return false;

			if (name == "GamepadActionFire") {
				weaponInput.primary = down;
				if (down && client.world) {
					if (stmp::optional<Player&> maybePlayer = client.world->GetLocalPlayer()) {
						Player& p = maybePlayer.value();
						if (p.IsToolWeapon() && !client.CanLocalPlayerUseWeapon())
							client.PlayerDryFiredWeapon(p);
					}
				}
				return true;
			}

			if (name == "GamepadActionAim") {
				bool lastVal = weaponInput.secondary || client.weapInput.secondary;
				weaponInput.secondary = down;
				if (down && client.world) {
					if (stmp::optional<Player&> maybePlayer = client.world->GetLocalPlayer()) {
						Player& p = maybePlayer.value();
						if (p.IsToolWeapon() && !lastVal && client.CanLocalPlayerUseWeapon()) {
							AudioParam param;
							param.volume = 0.08F;
							Handle<IAudioChunk> c =
							  client.audioDevice->RegisterSound("Sounds/Weapons/AimDownSightLocal.opus");
							client.audioDevice->PlayLocal(c.GetPointerOrNull(),
								MakeVector3(0.4F, -0.3F, 0.5F), param);
						}
					}
				}
				return true;
			}

			if (name == "GamepadActionJump") {
				playerInput.jump = down;
				return true;
			}
			if (name == "GamepadActionCrouch") {
				playerInput.crouch = down;
				return true;
			}
			if (name == "GamepadActionReload") {
				reloadKeyPressed = down;
				return true;
			}
			if (name == "GamepadActionScoreboard") {
				client.scoreboardVisible = down;
				return true;
			}
			if (name == "GamepadActionMenu") {
				if (down)
					client.KeyEvent("Escape", true);
				return true;
			}

			if (!down || !client.world)
				return true;

			stmp::optional<Player&> maybePlayer = client.world->GetLocalPlayer();
			if (!maybePlayer)
				return true;

			Player& p = maybePlayer.value();
			if (p.IsSpectator() || !p.IsAlive())
				return true;

			auto selectTool = [&](Player::ToolType tool, const char* unavailable) {
				if (p.IsToolSelectable(tool))
					client.SetSelectedTool(tool);
				else
					client.ShowAlert(_Tr("Client", unavailable), Client::AlertType::Error);
			};

			if (name == "GamepadActionToolSpade") {
				client.SetSelectedTool(Player::ToolSpade);
			} else if (name == "GamepadActionToolBlock") {
				selectTool(Player::ToolBlock, "Out of Blocks");
			} else if (name == "GamepadActionToolWeapon") {
				selectTool(Player::ToolWeapon, "Out of Ammo");
			} else if (name == "GamepadActionToolGrenade") {
				selectTool(Player::ToolGrenade, "Out of Grenades");
			} else if (name == "GamepadActionPrevTool" || name == "GamepadActionNextTool" ||
			           name == "GamepadActionSwitchTool") {
				Player::ToolType t = p.GetTool();
				bool reverse = name == "GamepadActionPrevTool";
				do {
					if (reverse) {
						switch (t) {
							case Player::ToolSpade: t = Player::ToolGrenade; break;
							case Player::ToolBlock: t = Player::ToolSpade; break;
							case Player::ToolWeapon: t = Player::ToolBlock; break;
							case Player::ToolGrenade: t = Player::ToolWeapon; break;
						}
					} else {
						switch (t) {
							case Player::ToolSpade: t = Player::ToolBlock; break;
							case Player::ToolBlock: t = Player::ToolWeapon; break;
							case Player::ToolWeapon: t = Player::ToolGrenade; break;
							case Player::ToolGrenade: t = Player::ToolSpade; break;
						}
					}
				} while (!p.IsToolSelectable(t));
				client.SetSelectedTool(t);
			}

			return true;
		}

		void ExtraGamepadInput::MergePlayerInput(PlayerInput& input) const {
			input.moveForward = input.moveForward || playerInput.moveForward;
			input.moveBackward = input.moveBackward || playerInput.moveBackward;
			input.moveLeft = input.moveLeft || playerInput.moveLeft;
			input.moveRight = input.moveRight || playerInput.moveRight;
			input.jump = input.jump || playerInput.jump;
			input.crouch = input.crouch || playerInput.crouch;
			input.sneak = input.sneak || playerInput.sneak;
			input.sprint = input.sprint || playerInput.sprint;
		}

		void ExtraGamepadInput::MergeWeaponInput(WeaponInput& input) const {
			input.primary = input.primary || weaponInput.primary;
			input.secondary = input.secondary || weaponInput.secondary;
		}
	} // namespace client
} // namespace spades
