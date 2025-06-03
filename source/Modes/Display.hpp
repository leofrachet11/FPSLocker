class DockedManualGui : public tsl::Gui {
public:
	uint32_t crc = 0;
	DockedModeRefreshRateAllowed rr = {0};
	DockedAdditionalSettings as;
	uint8_t maxRefreshRate = 60;
    DockedManualGui(uint8_t maxRefreshRate_impl) {
		if (maxRefreshRate_impl >= 70) maxRefreshRate = maxRefreshRate_impl;
		LoadDockedModeAllowedSave(rr, as, nullptr);
	}

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "Docked 1080p display manual settings");

		auto list = new tsl::elm::List();

		for (size_t i = 1; i < sizeof(DockedModeRefreshRateAllowedValues); i++) {
			if (maxRefreshRate < DockedModeRefreshRateAllowedValues[i]) break;
			char Hz[] = "120 Hz";
			snprintf(Hz, sizeof(Hz), "%d Hz", DockedModeRefreshRateAllowedValues[i]);
			auto *clickableListItem = new tsl::elm::ToggleListItem(Hz, rr[i]);
			clickableListItem->setClickListener([this, i](u64 keys) { 
				if (keys & HidNpadButton_A) {
					rr[i] = !rr[i];
					return true;
				}
				return false;
			});
			list->addItem(clickableListItem);	
		}
		
		frame->setContent(list);

        return frame;
    }

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		smInitialize();
		if (R_SUCCEEDED(apmInitialize())) {
			ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
			apmGetPerformanceMode(&mode);
			apmExit();
			if (mode != ApmPerformanceMode_Boost) {
				tsl::goBack();
				return true;
			}
		}
		smExit();
		if (keysHeld & HidNpadButton_B) {
			if (R_SUCCEEDED(SaltySD_Connect())) {
				SaveDockedModeAllowedSave(rr, as);
				SaltySD_SetAllowedDockedRefreshRates(rr);
				svcSleepThread(100'000);
				SaltySD_Term();
			}
			tsl::goBack();
			return true;
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DockedAdditionalGui : public tsl::Gui {
public:
	uint32_t crc = 0;
	DockedModeRefreshRateAllowed rr = {0};
	DockedAdditionalSettings as;
    DockedAdditionalGui() {
		LoadDockedModeAllowedSave(rr, as, nullptr);
	}

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "Docked display additional settings");

		auto list = new tsl::elm::List();

		auto *clickableListItem4 = new tsl::elm::ToggleListItem("Allow patches to force 60 Hz", !as.dontForce60InDocked);
		clickableListItem4->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				as.dontForce60InDocked = !as.dontForce60InDocked;
				if (R_SUCCEEDED(SaltySD_Connect())) {
					SaltySD_SetDontForce60InDocked(as.dontForce60InDocked);
					SaltySD_Term();
				}
				SaveDockedModeAllowedSave(rr, as);
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem4);

		auto *clickableListItem5 = new tsl::elm::ToggleListItem("Use lowest refresh rate for unmatched FPS targets", as.fpsTargetWithoutRRMatchLowest);
		clickableListItem5->setClickListener([this](u64 keys) { 
			if (keys & HidNpadButton_A) {
				as.fpsTargetWithoutRRMatchLowest = !as.fpsTargetWithoutRRMatchLowest;
				if (R_SUCCEEDED(SaltySD_Connect())) {
					SaltySD_SetMatchLowestRR(as.fpsTargetWithoutRRMatchLowest);
					SaltySD_Term();
				}
				SaveDockedModeAllowedSave(rr, as);
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem5);
		
		frame->setContent(list);

        return frame;
    }

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		smInitialize();
		if (R_SUCCEEDED(apmInitialize())) {
			ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
			apmGetPerformanceMode(&mode);
			apmExit();
			if (mode != ApmPerformanceMode_Boost) {
				tsl::goBack();
				return true;
			}
		}
		smExit();
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DockedGui : public tsl::Gui {
private:
	char Docked_c[256] = "";
	DockedModeRefreshRateAllowed rr;
	DockedAdditionalSettings as;
	uint8_t highestRefreshRate;
public:
    DockedGui() {
		mkdir("sdmc:/SaltySD/plugins/FPSLocker/", 777);
		mkdir("sdmc:/SaltySD/plugins/FPSLocker/ExtDisplays/", 777);
		int crc32 = 0;
		LoadDockedModeAllowedSave(rr, as, &crc32);
		highestRefreshRate = 60;
		getDockedHighestRefreshRate(&highestRefreshRate);
		snprintf(Docked_c, sizeof(Docked_c), "Reported max refresh rate: %u Hz\nConfig ID: %08X", highestRefreshRate, crc32);
	}

	size_t base_height = 128;
	bool block = false;

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "Docked display settings");

		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {

			renderer->drawString(Docked_c, false, x, y+20, 20, renderer->a(0xFFFF));
			
		}), 65);

		auto *clickableListItem1 = new tsl::elm::ListItem2("Allowed 1080p refresh rates");
		clickableListItem1->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A) && !block) {
				tsl::changeTo<DockedManualGui>(highestRefreshRate);
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem1);

		auto *clickableListItem2 = new tsl::elm::ListItem2("Additional settings");
		clickableListItem2->setClickListener([this](u64 keys) { 
			if ((keys & HidNpadButton_A) && !block) {
				tsl::changeTo<DockedAdditionalGui>();
				return true;
			}
			return false;
		});

		list->addItem(clickableListItem2);
		
		frame->setContent(list);

        return frame;
    }

	virtual void update() override {
		if (!block) tsl::hlp::doWithSmSession([this]{
			if (R_SUCCEEDED(apmInitialize())) {
				ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
				apmGetPerformanceMode(&mode);
				if (mode != ApmPerformanceMode_Boost ) {
					block = true;
					snprintf(Docked_c, sizeof(Docked_c),	"You are not in docked mode.\n"
															"Go back, put your Switch to dock\n"
															"and come back.");
				}
				apmExit();
			}
		});
	}
};

class DockedRefreshRateChangeGui : public tsl::Gui {
public:
	DockedModeRefreshRateAllowed rr;
	DockedAdditionalSettings as;
	DockedRefreshRateChangeGui () {
		LoadDockedModeAllowedSave(rr, as, nullptr);
	}

	// Called when this Gui gets loaded to create the UI
	// Allocate all elements on the heap. libtesla will make sure to clean them up when not needed anymore
	virtual tsl::elm::Element* createUI() override {
		// A OverlayFrame is the base element every overlay consists of. This will draw the default Title and Subtitle.
		// If you need more information in the header or want to change it's look, use a HeaderOverlayFrame.
		auto frame = new tsl::elm::OverlayFrame("FPSLocker", "Change Refresh Rate");

		// A list that can contain sub elements and handles scrolling
		auto list = new tsl::elm::List();

		for (size_t i = 0; i < sizeof(rr); i++) {
			if (rr[i] == false)
				continue;
			char Hz[] = "254 Hz";
			snprintf(Hz, sizeof(Hz), "%d Hz", DockedModeRefreshRateAllowedValues[i]);
			auto *clickableListItem = new tsl::elm::MiniListItem(Hz);
			clickableListItem->setClickListener([this, i](u64 keys) { 
				if (keys & HidNpadButton_A) {
					if (!oldSalty) {
						if (R_SUCCEEDED(SaltySD_Connect())) {
							SaltySD_SetDisplayRefreshRate(DockedModeRefreshRateAllowedValues[i]);
							SaltySD_Term();
							refreshRate_g = DockedModeRefreshRateAllowedValues[i];
						}
					}
					tsl::goBack();
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem);
		}

		frame->setContent(list);

		return frame;
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		smInitialize();
		if (R_SUCCEEDED(apmInitialize())) {
			ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
			apmGetPerformanceMode(&mode);
			apmExit();
			if (mode != ApmPerformanceMode_Boost) {
				smExit();
				tsl::goBack();
				return true;
			}
		}
		smExit();
		return false;   // Return true here to singal the inputs have been consumed
	}
};

class DisplayGui : public tsl::Gui {
private:
	char refreshRate_c[32] = "";
	char oled_c[48] = "Not available for Switch OLED\nin handheld mode.";
	bool isDocked = false;
	ApmPerformanceMode entry_mode = ApmPerformanceMode_Invalid;
public:
    DisplayGui() {
		if (isLite) entry_mode = ApmPerformanceMode_Normal;
		else {
			smInitialize();
			if (R_SUCCEEDED(apmInitialize())) {
				apmGetPerformanceMode(&entry_mode);
				apmExit();
			}
			else entry_mode = ApmPerformanceMode_Normal;
			smExit();
		}
	}
	size_t base_height = 128;

    virtual tsl::elm::Element* createUI() override {
        auto frame = new tsl::elm::OverlayFrame("FPSLocker", "Display settings");

		auto list = new tsl::elm::List();

		list->addItem(new tsl::elm::CustomDrawer([this](tsl::gfx::Renderer *renderer, s32 x, s32 y, s32 w, s32 h) {

			renderer->drawString(refreshRate_c, false, x, y+20, 20, renderer->a(0xFFFF));
			if (isOLED && !isDocked) renderer->drawString(oled_c, false, x, y+50, 20, renderer->a(0xFFFF));
			
		}), 90);

		if (!displaySync) {
			if (entry_mode == ApmPerformanceMode_Normal) {
				auto *clickableListItem = new tsl::elm::ListItem2("Increase Refresh Rate");
				clickableListItem->setClickListener([this](u64 keys) { 
					if ((keys & HidNpadButton_A) && (!isOLED || isDocked)) {
						if ((refreshRate_g >= 40) && (refreshRate_g < 60)) {
							if (R_SUCCEEDED(SaltySD_Connect())) {
								refreshRate_g += 5;
								SaltySD_SetDisplayRefreshRate(refreshRate_g);
								SaltySD_Term();
								if (Shared) (Shared -> displaySync) = refreshRate_g ? true : false;
							}
						}
						return true;
					}
					return false;
				});

				list->addItem(clickableListItem);

				auto *clickableListItem2 = new tsl::elm::ListItem2("Decrease Refresh Rate");
				clickableListItem2->setClickListener([this](u64 keys) { 
					if ((keys & HidNpadButton_A) && (!isOLED || isDocked)) {
						if (refreshRate_g > 40) {
							if (R_SUCCEEDED(SaltySD_Connect())) {
								refreshRate_g -= 5;
								SaltySD_SetDisplayRefreshRate(refreshRate_g);
								if (Shared) (Shared -> displaySync) = refreshRate_g ? true : false;
								SaltySD_Term();
							}
						}
						return true;
					}
					return false;
				});

				list->addItem(clickableListItem2);
			}
			else if (entry_mode == ApmPerformanceMode_Boost) {
				auto *clickableListItem2 = new tsl::elm::ListItem2("Change Refresh Rate");
				clickableListItem2->setClickListener([](u64 keys) { 
					if (keys & HidNpadButton_A) {
						tsl::changeTo<DockedRefreshRateChangeGui>();
						return true;
					}
					return false;
				});	
				list->addItem(clickableListItem2);	
			}
		}

		if (!oldSalty) {
			list->addItem(new tsl::elm::CategoryHeader("Match refresh rate with FPS Target.", true));
			auto *clickableListItem3 = new tsl::elm::ToggleListItem("Display Sync", displaySync);
			clickableListItem3->setClickListener([this](u64 keys) { 
				if (keys & HidNpadButton_A) {
					if (R_SUCCEEDED(SaltySD_Connect())) {
						SaltySD_SetDisplaySync(!displaySync);
						svcSleepThread(100'000);
						if (!isOLED || entry_mode == ApmPerformanceMode_Boost) {
							u64 PID = 0;
							Result rc = pmdmntGetApplicationProcessId(&PID);
							if (R_SUCCEEDED(rc) && Shared) {
								if (!displaySync == true && (Shared -> FPSlocked) < 40) {
									SaltySD_SetDisplayRefreshRate(60);
									(Shared -> displaySync) = false;
								}
								else if (!displaySync == true) {
									SaltySD_SetDisplayRefreshRate((Shared -> FPSlocked));
									(Shared -> displaySync) = (Shared -> FPSlocked) ? true : false;
								}
								else {
									(Shared -> displaySync) = false;
								}
							}
							else if (!displaySync == true && (R_FAILED(rc) || !PluginRunning)) {
								SaltySD_SetDisplayRefreshRate(60);
							}
						}
						SaltySD_Term();
						displaySync = !displaySync;
					}
					tsl::goBack();
					tsl::changeTo<DisplayGui>();
					return true;
				}
				return false;
			});

			list->addItem(clickableListItem3);

			if (!isLite) {
				auto *clickableListItem4 = new tsl::elm::ListItem2("Docked Settings");
				clickableListItem4->setClickListener([this](u64 keys) { 
					if ((keys & HidNpadButton_A)) {
						tsl::changeTo<DockedGui>();
						return true;
					}
					return false;
				});

				list->addItem(clickableListItem4);
			}
		}
		
		frame->setContent(list);

        return frame;
    }

	virtual void update() override {
		refreshRate_g = *refreshRate_shared;
		snprintf(refreshRate_c, sizeof(refreshRate_c), "Display Refresh Rate: %d Hz", refreshRate_g);
	}

	// Called once every frame to handle inputs not handled by other UI elements
	virtual bool handleInput(u64 keysDown, u64 keysHeld, const HidTouchState &touchPos, HidAnalogStickState joyStickPosLeft, HidAnalogStickState joyStickPosRight) override {
		if (!isLite) {
			smInitialize();
			if (R_SUCCEEDED(apmInitialize())) {
				ApmPerformanceMode mode = ApmPerformanceMode_Invalid;
				apmGetPerformanceMode(&mode);
				apmExit();
				if (mode != entry_mode) {
					smExit();
					tsl::goBack();
					tsl::changeTo<DisplayGui>();
					return true;
				}
			}
			smExit();
		}
		return false;   // Return true here to singal the inputs have been consumed
	}
};