/*
 Copyright (c) 2026 Eugene Conrad

 This file is part of ExtraSpades.
 ExtraSpades is a gameplay/client-tweaks fork built on OpenSpades/ZeroSpades.
 */

namespace spades {

	class DebugVisualMenu : spades::ui::UIElement {
		private ClientUI@ ui;
		private PreferenceTab @[] tabs;

		private float contentsLeft, contentsRight, contentsWidth, contentsHeight;
		private float contentsTop, tabLeft, tabRight, tabWidth, tabTop, tabRowHeight;
		private float panelTop, panelBottom;
		private int selectedTabIndex = 0;

		DebugVisualMenu(ClientUI@ ui) {
			super(ui.manager);
			@this.ui = ui;

			float sw = Manager.ScreenWidth;
			float sh = Manager.ScreenHeight;

			contentsWidth = sw - 16.0F;
			if (contentsWidth > 860.0F)
				contentsWidth = 860.0F;

			contentsHeight = sh - 8.0F;
			if (contentsHeight > 550.0F)
				contentsHeight = 550.0F;

			contentsTop = (sh - contentsHeight) * 0.5F;
			contentsLeft = (sw - contentsWidth) * 0.5F;
			contentsRight = contentsLeft + contentsWidth;

			tabWidth = 170.0F;
			tabRowHeight = 30.0F;
			tabTop = contentsTop + 2.0F;
			tabLeft = contentsLeft + 2.0F;
			tabRight = tabLeft + tabWidth;

			float panelBorderOffset = 14.0F;
			panelTop = contentsTop - panelBorderOffset;
			panelBottom = contentsTop + contentsHeight + panelBorderOffset;

			{
				spades::ui::Label shade(Manager);
				shade.BackgroundColor = Vector4(0.0F, 0.0F, 0.0F, 0.72F);
				shade.Bounds = AABB2(0.0F, panelTop + 1.0F, sw, (panelBottom - panelTop) - 1.0F);
				AddChild(shade);
			}

			HeadingNavIndex cameraNav();
			HeadingNavIndex feedbackNav();
			HeadingNavIndex bulletNav();
			HeadingNavIndex grenadeNav();

			AddTab(ExtraCameraPanel(Manager, ui.fontManager, cameraNav), "Camera", cameraNav);
			AddTab(ExtraFeedbackPanel(Manager, ui.fontManager, feedbackNav), "Combat Feedback", feedbackNav);
			AddTab(ExtraBulletVisualPanel(Manager, ui.fontManager, bulletNav), "Bullet Visuals", bulletNav);
			AddTab(ExtraGrenadePanel(Manager, ui.fontManager, grenadeNav), "Grenades", grenadeNav);

			float closeButtonTop = tabTop + float(tabs.length) * tabRowHeight + 5.0F;
			{
				PreferenceTabButton button(Manager);
				button.Caption = "Close";
				button.HotKeyText = "[Esc]";
				button.Bounds = AABB2(tabLeft, closeButtonTop, tabWidth, tabRowHeight);
				@button.Activated = spades::ui::EventHandler(this.OnClosePressed);
				AddChild(button);
			}

			{
				spades::ui::Label title(Manager);
				title.Text = "ExtraSpades Visual Tweaks";
				@title.Font = ui.fontManager.HeadingFont;
				title.Alignment = Vector2(0.5F, 0.5F);
				title.Bounds = AABB2(tabRight, contentsTop - 34.0F, contentsRight - tabRight, 28.0F);
				AddChild(title);
			}

			BuildHeadingNavigation(closeButtonTop + tabRowHeight + 24.0F);
			UpdateTabs();
		}

		private void AddTab(spades::ui::UIElement@ view, string caption, HeadingNavIndex@ nav) {
			PreferenceTab tab(this, view);
			tab.TabButton.Caption = caption;
			tab.TabButton.Bounds = AABB2(tabLeft, tabTop + float(tabs.length) * tabRowHeight,
			                             tabWidth, tabRowHeight);
			@tab.TabButton.Activated = spades::ui::EventHandler(this.OnTabButtonActivated);
			tab.View.Bounds = AABB2(tabRight, tabTop, contentsRight - tabRight, contentsHeight - 6.0F);
			tab.View.Visible = false;
			@tab.HeadingNav = nav;
			AddChild(tab.View);
			AddChild(tab.TabButton);
			tabs.insertLast(tab);
		}

		private void BuildHeadingNavigation(float top) {
			float btnH = 24.0F;
			float btnGap = 2.0F;

			for (uint i = 0; i < tabs.length; i++) {
				PreferenceTab@ tab = tabs[i];
				HeadingNavIndex@ nav = tab.HeadingNav;
				if (nav is null or nav.entries.length < 2)
					continue;

				spades::ui::UIElement container(Manager);
				container.Bounds = AABB2(tabLeft, top, tabWidth,
				                         float(nav.entries.length) * (btnH + btnGap) - btnGap);
				container.Visible = false;

				for (uint j = 0; j < nav.entries.length; j++) {
					HeadingNavEntry ent = nav.entries[j];
					HeadingNavButton btn(Manager);
					btn.Caption = ent.caption;
					btn.SetTarget(nav.list, ent.row);
					btn.Bounds = AABB2(0.0F, float(j) * (btnH + btnGap), tabWidth, btnH);
					container.AddChild(btn);
				}

				AddChild(container);
				@tab.NavPanel = container;
			}
		}

		private void OnTabButtonActivated(spades::ui::UIElement@ sender) {
			for (uint i = 0; i < tabs.length; i++) {
				if (cast<spades::ui::UIElement>(tabs[i].TabButton) is sender) {
					selectedTabIndex = int(i);
					UpdateTabs();
				}
			}
		}

		private void UpdateTabs() {
			for (uint i = 0; i < tabs.length; i++) {
				bool selected = selectedTabIndex == int(i);
				tabs[i].TabButton.Toggled = selected;
				tabs[i].View.Visible = selected;
				if (tabs[i].NavPanel !is null)
					tabs[i].NavPanel.Visible = selected;
			}
		}

		private void OnClosePressed(spades::ui::UIElement@ sender) { Close(); }

		void HotKey(string key) {
			if (key == "Escape") {
				Close();
			} else {
				UIElement::HotKey(key);
			}
		}

		void Close() {
			@ui.ActiveUI = null;
			ui.helper.AlertNotice("Debug visual menu closed");
		}

		void Render() {
			Renderer@ r = Manager.Renderer;
			Vector2 pos = ScreenPosition;
			Vector2 size = Size;

			r.ColorNP = Vector4(1.0F, 1.0F, 1.0F, 0.07F);
			r.DrawImage(null, AABB2(pos.x, pos.y + panelTop, size.x, 1.0F));
			r.DrawImage(null, AABB2(pos.x, pos.y + panelBottom, size.x, 1.0F));

			UIElement::Render();
		}
	}

	class ExtraCameraPanel : spades::ui::UIElement {
		ExtraCameraPanel(spades::ui::UIManager@ manager, FontManager@ fontManager,
		                 HeadingNavIndex@ nav) {
			super(manager);

			StandardPreferenceLayouter layouter(this, fontManager);
			@layouter.HeadingNav = nav;

			layouter.AddHeading("Fog");
			layouter.AddToggleField("Disable visual fog", "cg_disableFogVisual");
			layouter.AddControl("Toggle fog hotkey", "cg_keyToggleFog");

			layouter.AddHeading("Noclip Camera");
			layouter.AddControl("Toggle noclip camera", "cg_keyToggleNoclip");
			layouter.AddToggleField("Spectator camera noclip", "cg_spectatorNoclip");

			layouter.AddHeading("ADS Wheel Zoom");
			layouter.AddSliderField("Minimum ADS zoom", "cg_adsZoomMin",
			                         0.25F, 2.5F, 0.05F, ConfigNumberFormatter(2, "x"));
			layouter.AddSliderField("Maximum ADS zoom", "cg_adsZoomMax",
			                         1.0F, 5.0F, 0.05F, ConfigNumberFormatter(2, "x"));
			layouter.AddSliderField("Mouse wheel zoom step", "cg_adsZoomStep",
			                         0.05F, 1.0F, 0.05F, ConfigNumberFormatter(2, "x"));

			layouter.FinishLayout();
		}
	}

	class ExtraFeedbackPanel : spades::ui::UIElement {
		ExtraFeedbackPanel(spades::ui::UIManager@ manager, FontManager@ fontManager,
		                   HeadingNavIndex@ nav) {
			super(manager);

			StandardPreferenceLayouter layouter(this, fontManager);
			@layouter.HeadingNav = nav;

			layouter.AddHeading("Kill Fade");
			layouter.AddToggleField("Kill fade flash", "cg_killFlash");
			layouter.AddSliderField("Fade duration", "cg_killFlashDuration",
			                         0.04F, 0.7F, 0.02F, ConfigNumberFormatter(2, "s"));
			layouter.AddSliderField("Fade intensity", "cg_killFlashAlpha",
			                         0.02F, 1.0F, 0.02F, ConfigNumberFormatter(2, ""));

			layouter.AddHeading("Custom Kill Sounds");
			layouter.AddToggleField("CS-style custom sounds", "cg_customKillSounds");
			layouter.AddVolumeSlider("Custom sound volume", "cg_customKillSoundsGain");
			layouter.AddSliderField("Multikill window", "cg_customMultiKillWindow",
			                         1.0F, 15.0F, 0.5F, ConfigNumberFormatter(1, "s"));

			layouter.AddHeading("Classic Streak Sounds");
			layouter.AddToggleField("Built-in streak sounds", "cg_killSounds");
			layouter.AddVolumeSlider("Built-in streak volume", "cg_killSoundsGain");
			layouter.AddSliderField("Built-in streak pitch", "cg_killSoundsPitch",
			                         0.5F, 2.0F, 0.05F, ConfigNumberFormatter(2, "x"));

			layouter.FinishLayout();
		}
	}

	class ExtraBulletVisualPanel : spades::ui::UIElement {
		ExtraBulletVisualPanel(spades::ui::UIManager@ manager, FontManager@ fontManager,
		                       HeadingNavIndex@ nav) {
			super(manager);

			StandardPreferenceLayouter layouter(this, fontManager);
			@layouter.HeadingNav = nav;

			layouter.AddHeading("Flying Tracers");
			layouter.AddChoiceField("Bullet tracers", "cg_tracers",
			                        array<string> = {"ON", "3rd Person", "OFF"},
			                        array<int> = {1, 2, 0});
			layouter.AddToggleField("Draw in first person", "cg_tracersFirstPerson");
			layouter.AddSliderField("Flying tracers per player", "cg_tracersPerPlayerMax",
			                         1, 30, 1, ConfigNumberFormatter(0, ""));
			layouter.AddToggleField("Tracer lights", "cg_tracerLights");

			layouter.AddHeading("Static Trail Logs");
			layouter.AddToggleField("Static bullet trails", "cg_trailLogs");
			layouter.AddSliderField("Static trails per player", "cg_trailLogsPerPlayerMax",
			                         1, 30, 1, ConfigNumberFormatter(0, ""));
			layouter.AddSliderField("Static trail lifetime", "cg_trailLogLifetime",
			                         0.25F, 20.0F, 0.25F, ConfigNumberFormatter(2, "s"));

			layouter.FinishLayout();
		}
	}

	class ExtraGrenadePanel : spades::ui::UIElement {
		ExtraGrenadePanel(spades::ui::UIManager@ manager, FontManager@ fontManager,
		                  HeadingNavIndex@ nav) {
			super(manager);

			StandardPreferenceLayouter layouter(this, fontManager);
			@layouter.HeadingNav = nav;

			layouter.AddHeading("Throw Prediction");
			layouter.AddToggleField("Grenade trajectory preview", "cg_grenadeTrajectory");
			layouter.AddSliderField("Prediction time step", "cg_grenadeTrajectoryStep",
			                         0.01F, 0.1F, 0.005F, ConfigNumberFormatter(3, "s"));
			layouter.AddSliderField("Prediction max points", "cg_grenadeTrajectoryMaxPoints",
			                         8, 192, 4, ConfigNumberFormatter(0, ""));

			layouter.AddHeading("Grenade Trails");
			layouter.AddToggleField("Live grenade trail", "cg_grenadeTrail");
			layouter.AddSliderField("Trail persist after explosion", "cg_grenadeTrailPersist",
			                         0.0F, 15.0F, 0.5F, ConfigNumberFormatter(1, "s"));
			layouter.AddSliderField("Trail max points", "cg_grenadeTrailMaxPoints",
			                         8, 192, 4, ConfigNumberFormatter(0, ""));

			layouter.FinishLayout();
		}
	}
}
