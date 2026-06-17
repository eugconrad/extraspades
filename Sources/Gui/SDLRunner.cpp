/*
 Copyright (c) 2013 yvt

 This file is part of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.	 If not, see <http://www.gnu.org/licenses/>.

 */

#include <cctype>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <unordered_map>

#include "SDLRunner.h"

#include "Icon.h"
#include "SDLGLDevice.h"
#include <Audio/ALDevice.h>
#include <Audio/NullDevice.h>
#include <Client/Client.h>
#include <Core/ConcurrentDispatch.h>
#include <Core/Debug.h>
#include <Core/Disposable.h>
#include <Core/FileManager.h>
#include <Core/IStream.h>
#include <Core/Math.h>
#include <Core/Settings.h>
#include <Draw/OpenGL/GLRenderer.h>
#include <Draw/SW/SWPort.h>
#include <Draw/SW/SWRenderer.h>
#include <ZeroSpades.h>

SPADES_SETTING(r_videoWidth);
SPADES_SETTING(r_videoHeight);
DEFINE_SPADES_SETTING(r_fullscreen, "0");
DEFINE_SPADES_SETTING(r_vsync, "1");
DEFINE_SPADES_SETTING(r_allowSoftwareRendering, "0");
DEFINE_SPADES_SETTING(r_renderer, "gl");
DEFINE_SPADES_SETTING(s_audioDriver, "openal");
DEFINE_SPADES_SETTING(cl_fps, "0");

DEFINE_SPADES_SETTING(cg_gamepadEnabled, "1");
DEFINE_SPADES_SETTING(cg_gamepadInvertY, "0");
DEFINE_SPADES_SETTING(cg_gamepadSensitivity, "1");
DEFINE_SPADES_SETTING(cg_gamepadSensitivityX, "1");
DEFINE_SPADES_SETTING(cg_gamepadSensitivityY, "1");
DEFINE_SPADES_SETTING(cg_gamepadDeadzone, "0.18");
DEFINE_SPADES_SETTING(cg_gamepadTriggerThreshold, "0.45");
DEFINE_SPADES_SETTING(cg_gamepadResponseCurve, "1");
DEFINE_SPADES_SETTING(cg_gamepadVibration, "1");
DEFINE_SPADES_SETTING(cg_gamepadAxisMoveX, "LeftX");
DEFINE_SPADES_SETTING(cg_gamepadAxisMoveY, "LeftY");
DEFINE_SPADES_SETTING(cg_gamepadAxisLookX, "RightX");
DEFINE_SPADES_SETTING(cg_gamepadAxisLookY, "RightY");
DEFINE_SPADES_SETTING(cg_gamepadButtonFire, "GamepadRightTrigger");
DEFINE_SPADES_SETTING(cg_gamepadButtonAim, "GamepadLeftTrigger");
DEFINE_SPADES_SETTING(cg_gamepadButtonJump, "GamepadA");
DEFINE_SPADES_SETTING(cg_gamepadButtonCrouch, "GamepadB");
DEFINE_SPADES_SETTING(cg_gamepadButtonReload, "GamepadX");
DEFINE_SPADES_SETTING(cg_gamepadButtonSwitchTool, "GamepadY");
DEFINE_SPADES_SETTING(cg_gamepadButtonPrevTool, "GamepadLeftShoulder");
DEFINE_SPADES_SETTING(cg_gamepadButtonNextTool, "GamepadRightShoulder");
DEFINE_SPADES_SETTING(cg_gamepadButtonToolSpade, "GamepadDPadLeft");
DEFINE_SPADES_SETTING(cg_gamepadButtonToolBlock, "GamepadDPadDown");
DEFINE_SPADES_SETTING(cg_gamepadButtonToolWeapon, "GamepadDPadUp");
DEFINE_SPADES_SETTING(cg_gamepadButtonToolGrenade, "GamepadDPadRight");
DEFINE_SPADES_SETTING(cg_gamepadButtonMenu, "GamepadStart");
DEFINE_SPADES_SETTING(cg_gamepadButtonScoreboard, "GamepadBack");

static int lastMouseX = 0, lastMouseY = 0;

namespace spades {
	namespace gui {
		struct SDLRunner::GamepadState {
			SDL_GameController* controller = nullptr;
			SDL_JoystickID instanceId = -1;
			std::array<Sint16, SDL_CONTROLLER_AXIS_MAX> axes;
			std::unordered_map<std::string, bool> emittedActions;
			std::unordered_map<std::string, bool> virtualButtons;
			std::unordered_map<std::string, bool> uiVirtualButtons;

			GamepadState() { axes.fill(0); }
		};

		SDLRunner::SDLRunner() : m_hasSystemMenu(false), gamepadState(new GamepadState()) {}

		SDLRunner::~SDLRunner() { CloseGameController(); }

		client::IAudioDevice* SDLRunner::CreateAudioDevice() {
			if (EqualsIgnoringCase(s_audioDriver, "openal")) {
				return new audio::ALDevice();
			} else if (EqualsIgnoringCase(s_audioDriver, "null")) {
				return new audio::NullDevice();
			} else {
				SPLog("Unknown audio driver: %s, falling back to OpenAL", s_audioDriver.CString());
				s_audioDriver = "openal";
				return new audio::ALDevice();
			}
		}

		std::string SDLRunner::TranslateButton(Uint8 b) {
			SPADES_MARK_FUNCTION();
			switch (b) {
				case SDL_BUTTON_LEFT: return "LeftMouseButton";
				case SDL_BUTTON_RIGHT: return "RightMouseButton";
				case SDL_BUTTON_MIDDLE: return "MiddleMouseButton";
				case SDL_BUTTON_X1: return "MouseButton4";
				case SDL_BUTTON_X2: return "MouseButton5";
				default: return std::string();
			}
		}

		std::string SDLRunner::TranslateControllerButton(Uint8 b) {
			SPADES_MARK_FUNCTION();
			switch (static_cast<SDL_GameControllerButton>(b)) {
				case SDL_CONTROLLER_BUTTON_A: return "GamepadA";
				case SDL_CONTROLLER_BUTTON_B: return "GamepadB";
				case SDL_CONTROLLER_BUTTON_X: return "GamepadX";
				case SDL_CONTROLLER_BUTTON_Y: return "GamepadY";
				case SDL_CONTROLLER_BUTTON_BACK: return "GamepadBack";
				case SDL_CONTROLLER_BUTTON_GUIDE: return "GamepadGuide";
				case SDL_CONTROLLER_BUTTON_START: return "GamepadStart";
				case SDL_CONTROLLER_BUTTON_LEFTSTICK: return "GamepadLeftStick";
				case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return "GamepadRightStick";
				case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return "GamepadLeftShoulder";
				case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return "GamepadRightShoulder";
				case SDL_CONTROLLER_BUTTON_DPAD_UP: return "GamepadDPadUp";
				case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return "GamepadDPadDown";
				case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return "GamepadDPadLeft";
				case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return "GamepadDPadRight";
				default: return std::string();
			}
		}

		static bool MatchesGamepadControl(Settings::ItemHandle& setting, const std::string& control) {
			return EqualsIgnoringCase((std::string)setting, control);
		}

		static float NormalizeControllerAxis(Sint16 value) {
			float v = value < 0 ? (float)value / 32768.0F : (float)value / 32767.0F;
			return Clamp(v, -1.0F, 1.0F);
		}

		static SDL_GameControllerAxis ParseControllerAxisName(std::string name, bool& invert) {
			invert = false;
			if (!name.empty() && name[0] == '-') {
				invert = true;
				name.erase(name.begin());
			}

			if (EqualsIgnoringCase(name, "LeftX"))
				return SDL_CONTROLLER_AXIS_LEFTX;
			if (EqualsIgnoringCase(name, "LeftY"))
				return SDL_CONTROLLER_AXIS_LEFTY;
			if (EqualsIgnoringCase(name, "RightX"))
				return SDL_CONTROLLER_AXIS_RIGHTX;
			if (EqualsIgnoringCase(name, "RightY"))
				return SDL_CONTROLLER_AXIS_RIGHTY;
			if (EqualsIgnoringCase(name, "LeftTrigger"))
				return SDL_CONTROLLER_AXIS_TRIGGERLEFT;
			if (EqualsIgnoringCase(name, "RightTrigger"))
				return SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
			return SDL_CONTROLLER_AXIS_INVALID;
		}

		static float ApplyGamepadCurve(float value) {
			float sign = value < 0.0F ? -1.0F : 1.0F;
			float mag = fabsf(value);
			if ((int)cg_gamepadResponseCurve != 0)
				mag *= mag;
			return sign * mag;
		}

		static float ApplyGamepadDeadzone(float value) {
			float deadzone = Clamp((float)cg_gamepadDeadzone, 0.0F, 0.95F);
			float mag = fabsf(value);
			if (mag <= deadzone)
				return 0.0F;
			float sign = value < 0.0F ? -1.0F : 1.0F;
			return sign * ((mag - deadzone) / (1.0F - deadzone));
		}

		static float ReadMappedAxis(const SDLRunner::GamepadState& state,
		                            Settings::ItemHandle& setting) {
			bool invert = false;
			SDL_GameControllerAxis axis = ParseControllerAxisName((std::string)setting, invert);
			if (axis == SDL_CONTROLLER_AXIS_INVALID)
				return 0.0F;
			float value = NormalizeControllerAxis(state.axes[axis]);
			return invert ? -value : value;
		}

		static void SetGamepadAction(SDLRunner::GamepadState& state, View& view,
		                             const std::string& action, bool down) {
			bool& old = state.emittedActions[action];
			if (old == down)
				return;
			old = down;
			view.KeyEvent(action, down);
		}

		static void ReleaseGamepadActions(SDLRunner::GamepadState& state, View& view) {
			for (auto& item : state.emittedActions) {
				if (item.second) {
					item.second = false;
					view.KeyEvent(item.first, false);
				}
			}
			view.ControllerAxisEvent(0.0F, 0.0F, 0.0F, 0.0F);
		}

		static void DispatchGamepadControl(SDLRunner::GamepadState& state, View& view,
		                                   const std::string& control, bool down) {
			if (MatchesGamepadControl(cg_gamepadButtonFire, control))
				SetGamepadAction(state, view, "GamepadActionFire", down);
			if (MatchesGamepadControl(cg_gamepadButtonAim, control))
				SetGamepadAction(state, view, "GamepadActionAim", down);
			if (MatchesGamepadControl(cg_gamepadButtonJump, control))
				SetGamepadAction(state, view, "GamepadActionJump", down);
			if (MatchesGamepadControl(cg_gamepadButtonCrouch, control))
				SetGamepadAction(state, view, "GamepadActionCrouch", down);
			if (MatchesGamepadControl(cg_gamepadButtonReload, control))
				SetGamepadAction(state, view, "GamepadActionReload", down);
			if (MatchesGamepadControl(cg_gamepadButtonSwitchTool, control))
				SetGamepadAction(state, view, "GamepadActionSwitchTool", down);
			if (MatchesGamepadControl(cg_gamepadButtonPrevTool, control))
				SetGamepadAction(state, view, "GamepadActionPrevTool", down);
			if (MatchesGamepadControl(cg_gamepadButtonNextTool, control))
				SetGamepadAction(state, view, "GamepadActionNextTool", down);
			if (MatchesGamepadControl(cg_gamepadButtonToolSpade, control))
				SetGamepadAction(state, view, "GamepadActionToolSpade", down);
			if (MatchesGamepadControl(cg_gamepadButtonToolBlock, control))
				SetGamepadAction(state, view, "GamepadActionToolBlock", down);
			if (MatchesGamepadControl(cg_gamepadButtonToolWeapon, control))
				SetGamepadAction(state, view, "GamepadActionToolWeapon", down);
			if (MatchesGamepadControl(cg_gamepadButtonToolGrenade, control))
				SetGamepadAction(state, view, "GamepadActionToolGrenade", down);
			if (MatchesGamepadControl(cg_gamepadButtonMenu, control))
				SetGamepadAction(state, view, "GamepadActionMenu", down);
			if (MatchesGamepadControl(cg_gamepadButtonScoreboard, control))
				SetGamepadAction(state, view, "GamepadActionScoreboard", down);
		}

		void SDLRunner::CloseGameController(View* view) {
			if (!gamepadState)
				return;
			if (view)
				ReleaseGamepadActions(*gamepadState, *view);
			if (gamepadState->controller) {
				SPLog("Gamepad disconnected");
				SDL_GameControllerClose(gamepadState->controller);
				gamepadState->controller = nullptr;
				gamepadState->instanceId = -1;
			}
			gamepadState->axes.fill(0);
			gamepadState->virtualButtons.clear();
			gamepadState->uiVirtualButtons.clear();
		}

		void SDLRunner::OpenFirstGameController() {
			if (!cg_gamepadEnabled || !gamepadState || gamepadState->controller)
				return;

			int count = SDL_NumJoysticks();
			for (int i = 0; i < count; ++i) {
				if (!SDL_IsGameController(i))
					continue;
				SDL_GameController* controller = SDL_GameControllerOpen(i);
				if (!controller) {
					SPLog("Failed to open gamepad %d: %s", i, SDL_GetError());
					continue;
				}

				SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);
				gamepadState->controller = controller;
				gamepadState->instanceId = joystick ? SDL_JoystickInstanceID(joystick) : -1;
				SPLog("Gamepad connected: %s", SDL_GameControllerName(controller));
				break;
			}
		}

		void SDLRunner::UpdateGameController(View& view, float dt) {
			if (!gamepadState)
				return;

			if (!cg_gamepadEnabled) {
				ReleaseGamepadActions(*gamepadState, view);
				CloseGameController();
				return;
			}

			OpenFirstGameController();
			if (!gamepadState->controller) {
				view.ControllerAxisEvent(0.0F, 0.0F, 0.0F, 0.0F);
				return;
			}

			for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i) {
				gamepadState->axes[i] = SDL_GameControllerGetAxis(
				  gamepadState->controller, static_cast<SDL_GameControllerAxis>(i));
			}

			float triggerThreshold = Clamp((float)cg_gamepadTriggerThreshold, 0.0F, 1.0F);
			float leftTrigger =
			  Clamp((float)gamepadState->axes[SDL_CONTROLLER_AXIS_TRIGGERLEFT] / 32767.0F, 0.0F, 1.0F);
			float rightTrigger =
			  Clamp((float)gamepadState->axes[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] / 32767.0F, 0.0F, 1.0F);

			if (view.NeedsAbsoluteMouseCoordinate()) {
				ReleaseGamepadActions(*gamepadState, view);

				auto updateUiVirtualButton = [&](const std::string& name, bool down) {
					bool& old = gamepadState->uiVirtualButtons[name];
					if (old == down)
						return;
					old = down;
					view.KeyEvent(name, down);
				};
				updateUiVirtualButton("GamepadLeftTrigger", leftTrigger >= triggerThreshold);
				updateUiVirtualButton("GamepadRightTrigger", rightTrigger >= triggerThreshold);
				return;
			}

			for (auto& item : gamepadState->uiVirtualButtons) {
				if (item.second) {
					item.second = false;
					view.KeyEvent(item.first, false);
				}
			}

			auto updateVirtualButton = [&](const std::string& name, bool down) {
				bool& old = gamepadState->virtualButtons[name];
				if (old == down)
					return;
				old = down;
				DispatchGamepadControl(*gamepadState, view, name, down);
			};
			updateVirtualButton("GamepadLeftTrigger", leftTrigger >= triggerThreshold);
			updateVirtualButton("GamepadRightTrigger", rightTrigger >= triggerThreshold);

			float moveX = ApplyGamepadCurve(ApplyGamepadDeadzone(
			  ReadMappedAxis(*gamepadState, cg_gamepadAxisMoveX)));
			float moveY = ApplyGamepadCurve(ApplyGamepadDeadzone(
			  ReadMappedAxis(*gamepadState, cg_gamepadAxisMoveY)));
			float lookX = ApplyGamepadCurve(ApplyGamepadDeadzone(
			  ReadMappedAxis(*gamepadState, cg_gamepadAxisLookX)));
			float lookY = ApplyGamepadCurve(ApplyGamepadDeadzone(
			  ReadMappedAxis(*gamepadState, cg_gamepadAxisLookY)));

			if ((int)cg_gamepadInvertY)
				lookY = -lookY;

			float sensitivity = std::max(0.0F, (float)cg_gamepadSensitivity);
			float sensitivityX = std::max(0.0F, (float)cg_gamepadSensitivityX);
			float sensitivityY = std::max(0.0F, (float)cg_gamepadSensitivityY);
			float lookScale = 800.0F * dt * sensitivity;
			view.ControllerAxisEvent(moveX, moveY, lookX * lookScale * sensitivityX,
			                         lookY * lookScale * sensitivityY);
		}

		std::string SDLRunner::TranslateKey(const SDL_Keysym& k) {
			SPADES_MARK_FUNCTION();

			switch (k.sym) {
				case SDLK_ESCAPE: return "Escape";
				case SDLK_LEFT: return "Left";
				case SDLK_RIGHT: return "Right";
				case SDLK_UP: return "Up";
				case SDLK_DOWN: return "Down";
				case SDLK_SPACE: return " ";
				case SDLK_TAB: return "Tab";
				case SDLK_BACKSPACE: return "BackSpace";
				case SDLK_DELETE: return "Delete";
				case SDLK_RETURN: return "Enter";
				case SDLK_SLASH: return "/";
				case SDLK_KP_MINUS: return "-";
				case SDLK_KP_PLUS: return "+";
				case SDLK_KP_1: return "num1";
				case SDLK_KP_2: return "num2";
				case SDLK_KP_3: return "num3";
				case SDLK_KP_4: return "num4";
				case SDLK_KP_5: return "num5";
				case SDLK_KP_6: return "num6";
				case SDLK_KP_7: return "num7";
				case SDLK_KP_8: return "num8";
				case SDLK_KP_9: return "num9";
				case SDLK_KP_0: return "num0";
				default: return std::string(SDL_GetScancodeName(k.scancode));
			}
		}

		int SDLRunner::GetModState() { return SDL_GetModState(); }

		void SDLRunner::ProcessEvent(SDL_Event& event, View& view) {
			switch (event.type) {
				case SDL_QUIT:
					view.Closing();
					// running = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
					view.KeyEvent(TranslateButton(event.button.button), true);
					break;
				case SDL_MOUSEBUTTONUP:
					view.KeyEvent(TranslateButton(event.button.button), false);
					break;
				case SDL_MOUSEMOTION:
					if (m_active) {
						if (view.NeedsAbsoluteMouseCoordinate())
							view.MouseEvent(event.motion.x, event.motion.y);
						else
							view.MouseEvent(event.motion.xrel, event.motion.yrel);
					}
					break;
				case SDL_MOUSEWHEEL:
					view.WheelEvent(-event.wheel.x, -event.wheel.y);
					break;
				case SDL_KEYDOWN:
					if (!event.key.repeat) {
						// Toggle fullscreen mode
						if (event.key.keysym.mod & KMOD_ALT &&
							event.key.keysym.sym == SDLK_RETURN) {
							SDL_Window* window = SDL_GetWindowFromID(event.key.windowID);

							if (r_fullscreen) {
								if (!SDL_SetWindowFullscreen(window, 0))
									r_fullscreen = 0;
								else
									SPLog("Couldn't exit fullscreen mode: %s", SDL_GetError());
							} else {
								if (!SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN))
									r_fullscreen = 1;
								else
									SPLog("Couldn't enter fullscreen mode: %s", SDL_GetError());
							}
							return;
						}

						view.KeyEvent(TranslateKey(event.key.keysym), true);
					}
					break;
				case SDL_KEYUP: view.KeyEvent(TranslateKey(event.key.keysym), false); break;
				case SDL_TEXTINPUT: view.TextInputEvent(event.text.text); break;
				case SDL_TEXTEDITING:
					view.TextEditingEvent(event.edit.text, event.edit.start, event.edit.length);
					break;
				case SDL_WINDOWEVENT:
					if (event.window.type == SDL_WINDOWEVENT_FOCUS_GAINED) {
						SDL_SetRelativeMouseMode(SDL_TRUE);
						SDL_ShowCursor(SDL_DISABLE);
						m_active = true;
					} else if (event.window.type == SDL_WINDOWEVENT_FOCUS_LOST) {
						SDL_SetRelativeMouseMode(SDL_FALSE);
						SDL_ShowCursor(SDL_ENABLE);
						m_active = false;
					}
					break;
				case SDL_CONTROLLERDEVICEADDED:
					if (cg_gamepadEnabled)
						OpenFirstGameController();
					break;
				case SDL_CONTROLLERDEVICEREMOVED:
					if (gamepadState && event.cdevice.which == gamepadState->instanceId) {
						CloseGameController(&view);
						OpenFirstGameController();
					}
					break;
				case SDL_CONTROLLERBUTTONDOWN:
				case SDL_CONTROLLERBUTTONUP:
					if (gamepadState && gamepadState->controller &&
					    event.cbutton.which == gamepadState->instanceId) {
						std::string button = TranslateControllerButton(event.cbutton.button);
						if (!button.empty()) {
							bool down = event.type == SDL_CONTROLLERBUTTONDOWN;
							if (view.NeedsAbsoluteMouseCoordinate())
								view.KeyEvent(button, down);
							else
								DispatchGamepadControl(*gamepadState, view, button, down);

							if (down && cg_gamepadVibration &&
							    MatchesGamepadControl(cg_gamepadButtonFire, button)) {
#if SDL_VERSION_ATLEAST(2, 0, 9)
								SDL_GameControllerRumble(gamepadState->controller, 0x2000, 0x6000, 80);
#endif
							}
						}
					}
					break;
				case SDL_CONTROLLERAXISMOTION:
					if (gamepadState && gamepadState->controller &&
					    event.caxis.which == gamepadState->instanceId &&
					    event.caxis.axis < SDL_CONTROLLER_AXIS_MAX) {
						gamepadState->axes[event.caxis.axis] = event.caxis.value;
					}
					break;
				default: break;
			}
		}

		void SDLRunner::RunClientLoop(SDL_Window* wnd, spades::client::IRenderer* renderer,
									  spades::client::IAudioDevice* audio) {
			{
				Handle<View> view(CreateView(renderer, audio), false);
				Uint32 ot = SDL_GetTicks();

				bool running = true;
				bool lastShift = false;
				bool lastCtrl = false;
				bool editing = false;
				bool lastGui = false;
				bool lastAlt = false;
				bool absoluteMouseCoord = true;

				SPLog("Starting Client Loop");

				while (running) {
					SDL_Event event;

					DispatchQueue::GetThreadQueue()->ProcessQueue();

					Uint32 dt = SDL_GetTicks() - ot;

					if ((float)cl_fps != 0) {
						// Limit the frame rate
						Uint32 desiredDelay = static_cast<Uint32>(1000.0F / (float)cl_fps);
						desiredDelay = std::max<Uint32>(std::min<Uint32>(desiredDelay, 200), 1);
						if (dt < desiredDelay) {
							SDL_Delay(desiredDelay - dt);

							// Remeasure the time delta
							dt = SDL_GetTicks() - ot;
						}
					}

					ot += dt;
					if ((int32_t)dt > 0) {
						UpdateGameController(*view, (float)dt / 1000.0F);
						view->RunFrame((float)dt / 1000.0F);
						view->RunFrameLate((float)dt / 1000.0F);
					}

					if (view->WantsToBeClosed()) {
						view->Closing();
						SPLog("Close requested by Client");
						break;
					}

					int modState = GetModState();
					if (modState & KMOD_CTRL) {
						if (!lastCtrl) {
							view->KeyEvent("Control", true);
							lastCtrl = true;
						}
					} else {
						if (lastCtrl) {
							view->KeyEvent("Control", false);
							lastCtrl = false;
						}
					}

					if (modState & KMOD_SHIFT) {
						if (!lastShift) {
							view->KeyEvent("Shift", true);
							lastShift = true;
						}
					} else {
						if (lastShift) {
							view->KeyEvent("Shift", false);
							lastShift = false;
						}
					}

					if (modState & KMOD_GUI) {
						if (!lastGui) {
							view->KeyEvent("Meta", true);
							lastGui = true;
						}
					} else {
						if (lastGui) {
							view->KeyEvent("Meta", false);
							lastGui = false;
						}
					}

					if (modState & KMOD_ALT) {
						if (!lastAlt) {
							view->KeyEvent("Alt", true);
							lastAlt = true;
						}
					} else {
						if (lastAlt) {
							view->KeyEvent("Alt", false);
							lastAlt = false;
						}
					}

					bool ed = view->AcceptsTextInput();
					if (ed && !editing)
						SDL_StartTextInput();
					else if (!ed && editing)
						SDL_StopTextInput();
					editing = ed;

					if (editing) {
						AABB2 rt = view->GetTextInputRect();
						SDL_Rect srt;
						srt.x = (int)rt.GetMinX();
						srt.y = (int)rt.GetMinY();
						srt.w = (int)rt.GetWidth();
						srt.h = (int)rt.GetHeight();
						SDL_SetTextInputRect(&srt);
					}

					bool ab = view->NeedsAbsoluteMouseCoordinate();
					if (ab != absoluteMouseCoord) {
						if (ab) {
							SDL_SetRelativeMouseMode(SDL_FALSE);
							SDL_WarpMouseInWindow(wnd, lastMouseX, lastMouseY);
						} else {
							SDL_GetMouseState(&lastMouseX, &lastMouseY);

							// re-center if needed...
							if (lastMouseX == 0 && lastMouseY == 0) {
								int sw, sh;
								SDL_GetWindowSize(wnd, &sw, &sh);
								lastMouseX = sw / 2;
								lastMouseY = sh / 2;
							}

							SDL_SetRelativeMouseMode(SDL_TRUE);
						}

						absoluteMouseCoord = ab;
					}

					// Process events
					while (SDL_PollEvent(&event))
						ProcessEvent(event, *view);
				}

				CloseGameController(&*view);
				SPLog("Leaving Client Loop");
			}
		}

		auto SDLRunner::GetRendererType() -> RendererType {
			if (EqualsIgnoringCase(r_renderer, "gl"))
				return RendererType::GL;
			else if (EqualsIgnoringCase(r_renderer, "sw"))
				return RendererType::SW;
			else
				SPRaise("Unknown renderer name: %s", r_renderer.CString());
		}

		class SDLSWPort : public draw::SWPort, public Disposable {
			SDL_Window* wnd;
			SDL_Surface* surface;
			bool adjusted;
			int actualW, actualH;

			Handle<Bitmap> framebuffer;

			void SetFramebufferBitmap() {
				if (adjusted) {
					framebuffer = Handle<Bitmap>::New(actualW, actualH);
				} else {
					framebuffer = Handle<Bitmap>::New(reinterpret_cast<uint32_t*>(surface->pixels),
													  surface->w, surface->h);
				}
			}

			void EnsureSurfaceIsValid() {
				if (!surface)
					SPRaise("The SDL surface associated with this SDLSWPart has already been"
							"destroyed.");
			}

		protected:
			~SDLSWPort() {
				if (surface && SDL_MUSTLOCK(surface))
					SDL_UnlockSurface(surface);
			}

		public:
			SDLSWPort(SDL_Window* wnd) : wnd(wnd), surface(nullptr) {
				surface = SDL_GetWindowSurface(wnd);
				// FIXME: check pixel format
				if (SDL_MUSTLOCK(surface))
					SDL_LockSurface(surface);
				actualW = surface->w & ~7;
				actualH = surface->h & ~7;
				if (actualW != surface->w || actualH != surface->h) {
					SPLog("Surface size %dx%d doesn't match the software renderer's"
						  " requirements. Rounded to %dx%d using an intermediate surface.",
						  surface->w, surface->h, actualW, actualH);
					adjusted = true;
					memset(surface->pixels, 0, surface->w * surface->h * 4);
				} else {
					adjusted = false;
				}
				SetFramebufferBitmap();
			}

			void Dispose() override { surface = nullptr; }

			Bitmap& GetFramebuffer() override {
				EnsureSurfaceIsValid();

				return *framebuffer;
			}
			void Swap() override {
				EnsureSurfaceIsValid();

				if (adjusted) {
					int sy = (surface->h - actualH) >> 1;
					int sx = (surface->w - actualW) >> 1;
					uint32_t* outPixels = reinterpret_cast<uint32_t*>(surface->pixels);
					outPixels += sx + sy * (surface->pitch >> 2);

					uint32_t* inPixels = framebuffer->GetPixels();
					for (int y = 0; y < actualH; y++) {
						std::memcpy(outPixels, inPixels, actualW * 4);

						outPixels += surface->pitch >> 2;
						inPixels += actualW;
					}
				}

				if (SDL_MUSTLOCK(surface))
					SDL_UnlockSurface(surface);
				SDL_UpdateWindowSurface(wnd);
				if (SDL_MUSTLOCK(surface)) {
					SDL_LockSurface(surface);
					if (!adjusted)
						SetFramebufferBitmap();
				}
			}
		};

		std::tuple<Handle<client::IRenderer>, Handle<Disposable>>
		SDLRunner::CreateRenderer(SDL_Window* wnd, RendererType type) {
			switch (type) {
				case RendererType::GL: {
					auto glDevice = Handle<SDLGLDevice>::New(wnd).Cast<draw::IGLDevice>();
					auto dummy = Handle<Disposable>::New(); // FIXME
					return std::make_tuple(
					  Handle<draw::GLRenderer>::New(std::move(glDevice)).Cast<client::IRenderer>(),
					  std::move(dummy));
				}
				case RendererType::SW: {
					auto port = Handle<SDLSWPort>::New(wnd).Cast<draw::SWPort>();
					return std::make_tuple(
					  Handle<draw::SWRenderer>::New(port).Cast<client::IRenderer>(),
					  port.Cast<Disposable>());
				}
				default: SPRaise("Invalid renderer type");
			}
		}

		void SDLRunner::Run(int width, int height) {
			SPADES_MARK_FUNCTION();

			SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
			SDL_GameControllerEventState(SDL_ENABLE);

			try {
				std::string caption;
				{
					caption = PACKAGE_STRING;
#if !NDEBUG
					caption.append(" DEBUG build");
#endif
#ifndef GIT_COMMIT_HASH
#ifdef ZEROSPADES_COMPILER_STR
					caption.append(" " ZEROSPADES_COMPILER_STR); // add compiler to window title when git hash unavailable
#endif
#endif
				}

				auto rtype = GetRendererType();

				Uint32 sdlFlags = 0;
				switch (rtype) {
					case RendererType::GL:
						sdlFlags = SDL_WINDOW_OPENGL;
						SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
						SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
						if (!r_allowSoftwareRendering)
							SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
						break;
					case RendererType::SW: sdlFlags = 0; break;
				}

				if (!m_hasSystemMenu) {
					if (r_fullscreen)
						sdlFlags |= SDL_WINDOW_FULLSCREEN;
#ifdef __MACOSX__
					if (!r_fullscreen)
						sdlFlags |= SDL_WINDOW_BORDERLESS;
#endif
				}

				SDL_Window* window = SDL_CreateWindow(caption.c_str(),
					SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
					width, height, sdlFlags);

				if (!window) {
					std::string msg = SDL_GetError();
					SPRaise("Failed to create graphics window: %s", msg.c_str());
				}

#ifdef __APPLE__
#elif __unix
				SDL_Surface* icon = nullptr;
				SDL_RWops* icon_rw = nullptr;
				icon_rw = SDL_RWFromConstMem(g_appIconData, GetAppIconDataSize());
				if (icon_rw != nullptr) {
					icon = IMG_LoadPNG_RW(icon_rw);
					SDL_FreeRW(icon_rw);
				}
				if (icon == nullptr) {
					std::string msg = SDL_GetError();
					SPLog("Failed to load icon: %s", msg.c_str());
				} else {
					SDL_SetWindowIcon(window, icon);
					SDL_FreeSurface(icon);
				}
#endif

				SDL_SetRelativeMouseMode(SDL_FALSE);
				SDL_ShowCursor(SDL_DISABLE);
				m_active = true;

				{
					Handle<client::IRenderer> renderer;
					Handle<Disposable> windowReference;
					std::tie(renderer, windowReference) = CreateRenderer(window, rtype);

					Handle<client::IAudioDevice> audio(CreateAudioDevice(), false);

					if (rtype == RendererType::GL) {
						int vsync = r_vsync;
						if (SDL_GL_SetSwapInterval(vsync) != 0)
							SPRaise("SDL_GL_SetSwapInterval failed: %s", SDL_GetError());
					}

					RunClientLoop(window, renderer.GetPointerOrNull(), audio.GetPointerOrNull());

					// `SDL_Window` and its associated resources will be inaccessible
					// past this point. Some referencing objects might be still alive due to
					// the indeterministic nature of AngelScript's tracing GC, so we explicitly
					// break such references right now.
					windowReference->Dispose();
				}
			} catch (...) {
				SDL_Quit();
				throw;
			}

			SDL_Quit();
		}
	} // namespace gui
} // namespace spades
