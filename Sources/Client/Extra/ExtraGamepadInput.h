/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.
 */

#pragma once

#include <string>

#include "../Player.h"

namespace spades {
	namespace client {
		class Client;

		class ExtraGamepadInput {
			Client& client;
			PlayerInput playerInput;
			WeaponInput weaponInput;
			bool reloadKeyPressed = false;

		public:
			explicit ExtraGamepadInput(Client& client);

			void Clear();
			void ControllerAxisEvent(float moveX, float moveY, float lookX, float lookY);
			bool HandleAction(const std::string& name, bool down);

			void MergePlayerInput(PlayerInput& input) const;
			void MergeWeaponInput(WeaponInput& input) const;
			bool IsReloadKeyPressed() const { return reloadKeyPressed; }
		};
	} // namespace client
} // namespace spades
