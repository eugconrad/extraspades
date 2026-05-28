#pragma once

#include <string>

#include <Core/Math.h>

namespace spades {
	namespace client {
		class Client;
		class IRenderer;

		class DebugMenuView {
			Client* client;
			IRenderer& renderer;
			Vector2 cursorPos;

		public:
			explicit DebugMenuView(Client* client);
			~DebugMenuView();

			void MouseEvent(float x, float y);
			void KeyEvent(const std::string& key, bool down);
			void Update(float dt);
			void Draw();
		};
	} // namespace client
} // namespace spades
