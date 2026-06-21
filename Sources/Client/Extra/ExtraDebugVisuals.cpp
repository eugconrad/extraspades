/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.
 */

#include "ExtraDebugVisuals.h"

#include "ExtraClientFeatures.h"
#include "../Client.h"
#include "../ClientUI.h"
#include "../IAudioChunk.h"
#include "../IAudioDevice.h"
#include <Core/Settings.h>
#include <Core/Strings.h>

DEFINE_SPADES_SETTING(cg_keyToggleFog, "i");
DEFINE_SPADES_SETTING(cg_disableFogVisual, "0");
DEFINE_SPADES_SETTING(cg_keyToggleNoclip, "u");

namespace spades {
	namespace client {
		namespace {
			const char* const kDebugVisualMenuKey = "Delete";

			bool CheckExtraKey(const std::string& cfg, const std::string& input) {
				if (cfg.empty())
					return false;

				static const std::string space1("space");
				static const std::string space2("spacebar");
				static const std::string space3("spacekey");

				if (EqualsIgnoringCase(cfg, space1) ||
				    EqualsIgnoringCase(cfg, space2) ||
				    EqualsIgnoringCase(cfg, space3)) {
					return input == " ";
				}
				return EqualsIgnoringCase(cfg, input);
			}
		}

		ExtraDebugVisuals::ExtraDebugVisuals(Client& client) : client(client) {}

		bool ExtraDebugVisuals::HandleKeyEvent(const std::string& name, bool down) {
			if (name == kDebugVisualMenuKey && down) {
				client.ClearTransientInputState();
				client.scriptedUI->ToggleDebugVisualMenu();
				return true;
			}

			if (!down || !client.world)
				return false;

			if (CheckExtraKey(cg_keyToggleNoclip, name)) {
				client.GetExtraFeatures().ToggleNoclip();
				return true;
			}

			if (CheckExtraKey(cg_keyToggleFog, name)) {
				cg_disableFogVisual = cg_disableFogVisual ? 0 : 1;
				client.ShowAlert(_Tr("Client", "Fog visual: {0}",
					cg_disableFogVisual ? "OFF" : "ON"), Client::AlertType::Notice);

				Handle<IAudioChunk> c =
				  client.audioDevice->RegisterSound("Sounds/Player/Flashlight.opus");
				client.audioDevice->PlayLocal(c.GetPointerOrNull(), AudioParam());
				return true;
			}

			return false;
		}
	} // namespace client
} // namespace spades
