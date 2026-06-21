/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.
 */

#pragma once

#include <string>

namespace spades {
	namespace client {
		class Client;

		class ExtraDebugVisuals {
			Client& client;

		public:
			explicit ExtraDebugVisuals(Client& client);

			bool HandleKeyEvent(const std::string& name, bool down);
		};
	} // namespace client
} // namespace spades
