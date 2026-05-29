#include "DebugMenuView.h"

#include "Client.h"
#include "Fonts.h"
#include "IAudioChunk.h"
#include "IAudioDevice.h"
#include "IFont.h"
#include "IImage.h"
#include "IRenderer.h"

#include <Core/Settings.h>
#include <Core/Strings.h>

#include <cstdio>

SPADES_SETTING(cg_killFlash);
SPADES_SETTING(cg_killFlashDuration);
SPADES_SETTING(cg_killFlashAlpha);
SPADES_SETTING(cg_tracers);
SPADES_SETTING(cg_tracersFirstPerson);
SPADES_SETTING(cg_tracersPerPlayerMax);
SPADES_SETTING(cg_trailLogs);
SPADES_SETTING(cg_trailLogsPerPlayerMax);
SPADES_SETTING(cg_trailLogLifetime);

namespace spades {
	namespace client {

		namespace {
			struct MenuLayout {
				float left;
				float top;
				float width;
				float height;
				float right;
				float row;
				float firstRowY;
			};

			MenuLayout BuildLayout(IRenderer& renderer) {
				float sw = renderer.ScreenWidth();
				float sh = renderer.ScreenHeight();
				MenuLayout l;
				l.width = std::min(700.0F, sw - 80.0F);
				l.height = 430.0F;
				l.left = (sw - l.width) * 0.5F;
				l.top = std::max(40.0F, (sh - l.height) * 0.5F);
				l.right = l.left + l.width;
				l.row = 34.0F;
				l.firstRowY = l.top + 62.0F;
				return l;
			}

			void PlayClick(Client* client) {
				IAudioDevice& audio = client->GetAudioDevice();
				Handle<IAudioChunk> c = audio.RegisterSound("Sounds/Feedback/Limbo/Select.opus");
				audio.PlayLocal(c.GetPointerOrNull(), AudioParam());
			}

			bool HitTest(const AABB2& box, const Vector2& p) { return box && p; }

			void DrawButton(IRenderer& renderer, const AABB2& rect, bool hovered, bool active) {
				Vector4 fill = MakeVector4(0.2F, 0.2F, 0.2F, 0.82F);
				if (active)
					fill = MakeVector4(0.3F, 0.6F, 0.35F, 0.9F);
				else if (hovered)
					fill = MakeVector4(0.35F, 0.35F, 0.35F, 0.9F);
				renderer.SetColorAlphaPremultiplied(fill);
				renderer.DrawImage(nullptr, rect);
				renderer.SetColorAlphaPremultiplied(MakeVector4(0.8F, 0.8F, 0.8F, 0.5F));
				renderer.DrawOutlinedRect(rect.GetMinX(), rect.GetMinY(), rect.GetMaxX(), rect.GetMaxY());
			}

			int ClampInt(int v, int minV, int maxV) { return std::max(minV, std::min(maxV, v)); }

			std::string F2(float v) {
				char b[32];
				sprintf(b, "%.2f", v);
				return b;
			}
		} // namespace

		DebugMenuView::DebugMenuView(Client* client)
		    : client(client), renderer(client->GetRenderer()),
		      cursorPos(MakeVector2(renderer.ScreenWidth() * 0.5F, renderer.ScreenHeight() * 0.5F)) {}

		DebugMenuView::~DebugMenuView() {}

		void DebugMenuView::MouseEvent(float x, float y) {
			cursorPos.x = Clamp(x, 0.0F, renderer.ScreenWidth());
			cursorPos.y = Clamp(y, 0.0F, renderer.ScreenHeight());
		}

		void DebugMenuView::Update(float) {}

		void DebugMenuView::KeyEvent(const std::string& key, bool down) {
			if (!down)
				return;

			MenuLayout lo = BuildLayout(renderer);
			float y = lo.firstRowY;

			auto applyStep = [&](float& value, float step, float minV, float maxV, bool plus) {
				value = plus ? value + step : value - step;
				value = Clamp(value, minV, maxV);
			};

			auto processToggle = [&](auto& setting, const char* label) {
				AABB2 btn(lo.left + 305.0F, y, 90.0F, 26.0F);
				if (HitTest(btn, cursorPos) && key == "LeftMouseButton") {
					int v = (int)setting;
					setting = v ? 0 : 1;
					PlayClick(client);
				}
				y += lo.row;
			};

			auto processFloat = [&](auto& setting, const char* label, float step, float minV, float maxV) {
				float minusX = lo.right - 170.0F;
				AABB2 minusBtn(minusX, y, 32.0F, 26.0F);
				AABB2 plusBtn(minusX + 136.0F, y, 32.0F, 26.0F);

				float v = setting;
				if (key == "LeftMouseButton" && HitTest(minusBtn, cursorPos)) {
					applyStep(v, step, minV, maxV, false);
					setting = v;
					PlayClick(client);
				}
				if (key == "LeftMouseButton" && HitTest(plusBtn, cursorPos)) {
					applyStep(v, step, minV, maxV, true);
					setting = v;
					PlayClick(client);
				}
				if (key == "WheelUp" || key == "WheelDown") {
					AABB2 wheelZone(lo.left + 12.0F, y - 2.0F, lo.width - 24.0F, 30.0F);
					if (HitTest(wheelZone, cursorPos)) {
						applyStep(v, step, minV, maxV, key == "WheelUp");
						setting = v;
					}
				}
				y += lo.row;
			};

			auto processInt = [&](auto& setting, const char* label, int step, int minV, int maxV) {
				float minusX = lo.right - 170.0F;
				AABB2 minusBtn(minusX, y, 32.0F, 26.0F);
				AABB2 plusBtn(minusX + 136.0F, y, 32.0F, 26.0F);

				int v = ClampInt((int)setting, minV, maxV);
				if (key == "LeftMouseButton" && HitTest(minusBtn, cursorPos)) {
					v = ClampInt(v - step, minV, maxV);
					setting = v;
					PlayClick(client);
				}
				if (key == "LeftMouseButton" && HitTest(plusBtn, cursorPos)) {
					v = ClampInt(v + step, minV, maxV);
					setting = v;
					PlayClick(client);
				}
				if (key == "WheelUp" || key == "WheelDown") {
					AABB2 wheelZone(lo.left + 12.0F, y - 2.0F, lo.width - 24.0F, 30.0F);
					if (HitTest(wheelZone, cursorPos)) {
						v = ClampInt(v + (key == "WheelUp" ? step : -step), minV, maxV);
						setting = v;
					}
				}
				y += lo.row;
			};

			processToggle(cg_killFlash, "Kill flash");
			processFloat(cg_killFlashDuration, "Kill flash duration", 0.02F, 0.04F, 0.7F);
			processFloat(cg_killFlashAlpha, "Kill flash alpha", 0.02F, 0.02F, 0.6F);
			processToggle(cg_tracers, "Bullet tracers");
			processToggle(cg_tracersFirstPerson, "Tracers in first-person");
			processInt(cg_tracersPerPlayerMax, "Tracers per player", 1, 1, 30);
			processToggle(cg_trailLogs, "Static trail logs");
			processInt(cg_trailLogsPerPlayerMax, "Trail logs per player", 1, 1, 30);
			processFloat(cg_trailLogLifetime, "Trail log lifetime", 0.25F, 0.25F, 20.0F);

			AABB2 closeBtn(lo.right - 34.0F, lo.top + 10.0F, 24.0F, 24.0F);
			if (key == "LeftMouseButton" && HitTest(closeBtn, cursorPos)) {
				client->CloseDebugMenu();
				PlayClick(client);
			}
		}

		void DebugMenuView::Draw() {
			IFont& font = client->fontManager->GetGuiFont();
			float sw = renderer.ScreenWidth();
			float sh = renderer.ScreenHeight();
			MenuLayout lo = BuildLayout(renderer);

			renderer.SetColorAlphaPremultiplied(MakeVector4(0.0F, 0.0F, 0.0F, 0.45F));
			renderer.DrawImage(nullptr, AABB2(0, 0, sw, sh));

			renderer.SetColorAlphaPremultiplied(MakeVector4(0.09F, 0.11F, 0.14F, 0.95F));
			renderer.DrawFilledRect(lo.left, lo.top, lo.right, lo.top + lo.height);
			renderer.SetColorAlphaPremultiplied(MakeVector4(0.75F, 0.8F, 0.9F, 0.55F));
			renderer.DrawOutlinedRect(lo.left, lo.top, lo.right, lo.top + lo.height);

			font.DrawShadow("Debug Visual Menu", MakeVector2(lo.left + 14.0F, lo.top + 12.0F), 1.0F,
			                MakeVector4(1, 1, 1, 1), MakeVector4(0, 0, 0, 0.6F));
			font.DrawShadow("[DELETE] close", MakeVector2(lo.right - 150.0F, lo.top + 12.0F), 1.0F,
			                MakeVector4(1, 1, 1, 0.7F), MakeVector4(0, 0, 0, 0.6F));

			auto drawRowLabel = [&](const std::string& label, float y) {
				font.DrawShadow(label, MakeVector2(lo.left + 14.0F, y + 4.0F), 1.0F,
				                MakeVector4(1, 1, 1, 1), MakeVector4(0, 0, 0, 0.6F));
			};

			float y = lo.firstRowY;

			auto drawToggle = [&](const std::string& label, int value) {
				drawRowLabel(label, y);
				AABB2 btn(lo.left + 305.0F, y, 90.0F, 26.0F);
				DrawButton(renderer, btn, HitTest(btn, cursorPos), value != 0);
				font.DrawShadow(value ? "ON" : "OFF", MakeVector2(btn.GetMinX() + 28.0F, y + 4.0F),
				                1.0F, MakeVector4(1, 1, 1, 1), MakeVector4(0, 0, 0, 0.5F));
				y += lo.row;
			};

			auto drawValueButtons = [&](const std::string& label, const std::string& valueStr) {
				drawRowLabel(label, y);
				float minusX = lo.right - 170.0F;
				AABB2 minusBtn(minusX, y, 32.0F, 26.0F);
				AABB2 valueBox(minusX + 34.0F, y, 100.0F, 26.0F);
				AABB2 plusBtn(minusX + 136.0F, y, 32.0F, 26.0F);
				DrawButton(renderer, minusBtn, HitTest(minusBtn, cursorPos), false);
				DrawButton(renderer, plusBtn, HitTest(plusBtn, cursorPos), false);
				renderer.SetColorAlphaPremultiplied(MakeVector4(0.16F, 0.16F, 0.16F, 0.9F));
				renderer.DrawImage(nullptr, valueBox);
				renderer.SetColorAlphaPremultiplied(MakeVector4(0.7F, 0.7F, 0.7F, 0.45F));
				renderer.DrawOutlinedRect(valueBox.GetMinX(), valueBox.GetMinY(), valueBox.GetMaxX(),
				                          valueBox.GetMaxY());
				font.DrawShadow("-", MakeVector2(minusBtn.GetMinX() + 11.0F, y + 4.0F), 1.0F,
				                MakeVector4(1, 1, 1, 1), MakeVector4(0, 0, 0, 0.5F));
				font.DrawShadow("+", MakeVector2(plusBtn.GetMinX() + 10.0F, y + 3.0F), 1.0F,
				                MakeVector4(1, 1, 1, 1), MakeVector4(0, 0, 0, 0.5F));
				Vector2 textSize = font.Measure(valueStr);
				font.DrawShadow(valueStr,
				                MakeVector2(valueBox.GetMinX() + (valueBox.GetWidth() - textSize.x) * 0.5F, y + 4.0F),
				                1.0F, MakeVector4(1, 1, 1, 1), MakeVector4(0, 0, 0, 0.5F));
				y += lo.row;
			};

			drawToggle("Kill fade", (int)cg_killFlash);
			drawValueButtons("Kill fade duration", F2((float)cg_killFlashDuration));
			drawValueButtons("Kill fade intensity", F2((float)cg_killFlashAlpha));
			drawToggle("Flying tracers", (int)cg_tracers);
			drawToggle("Tracers in first person", (int)cg_tracersFirstPerson);
			drawValueButtons("Flying tracers per player", ToString((int)cg_tracersPerPlayerMax));
			drawToggle("Static trail logs", (int)cg_trailLogs);
			drawValueButtons("Static trail logs per player", ToString((int)cg_trailLogsPerPlayerMax));
			drawValueButtons("Static trail lifetime (sec)", F2((float)cg_trailLogLifetime));

			font.DrawShadow("Tip: wheel over a row to change its value",
			                MakeVector2(lo.left + 14.0F, lo.top + lo.height - 30.0F), 1.0F,
			                MakeVector4(1, 1, 1, 0.7F), MakeVector4(0, 0, 0, 0.5F));

			AABB2 closeBtn(lo.right - 34.0F, lo.top + 10.0F, 24.0F, 24.0F);
			DrawButton(renderer, closeBtn, HitTest(closeBtn, cursorPos), false);
			font.DrawShadow("X", MakeVector2(closeBtn.GetMinX() + 7.0F, closeBtn.GetMinY() + 3.0F), 1.0F,
			                MakeVector4(1, 1, 1, 1), MakeVector4(0, 0, 0, 0.5F));

			Handle<IImage> cursor = renderer.RegisterImage("Gfx/UI/Cursor.png");
			renderer.SetColorAlphaPremultiplied(MakeVector4(1, 1, 1, 1));
			renderer.DrawImage(cursor, AABB2(cursorPos.x - 8, cursorPos.y - 8, 32, 32));
		}
	} // namespace client
} // namespace spades
