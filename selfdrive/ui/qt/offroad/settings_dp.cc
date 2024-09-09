#include "selfdrive/ui/qt/offroad/settings_dp.h"

#include "common/params.h"
#include "selfdrive/ui/ui.h"
#include "selfdrive/ui/qt/util.h"

void DPCtrlPanel::add_generic_toggles(std::vector<std::tuple<QString, QString, QString>> &toggle_defs) {
  for (auto &[param, title, desc] : toggle_defs) {
    if (param == "") {
      auto label = new LabelControl(title, "");
      addItem(label);
      continue;
    }
    auto toggle = new ParamControl(param, title, desc, "", this);

    bool locked = params.getBool((param + "Lock").toStdString());
    toggle->setEnabled(!locked);

    addItem(toggle);
    toggles[param.toStdString()] = toggle;
  }
}

void DPCtrlPanel::add_overall_toggles() {
  std::vector<std::tuple<QString, QString, QString>> toggle_defs{
    {
      "",
      QString::fromUtf8("🐉 ") + tr("Ctrl - Overall") + QString::fromUtf8(" 🐉"),
      "",
    },
    {
      "dp_car_dashcam_mode_removal",
      tr("Dashcam Mode Removal"),
      tr("If you see 'Dashcam Mode' while onroad, enable this will force enable openpilot control.\nDashcam Mode usually means your vehicle is not fully supported.\nUSE AT YOUR OWN RISK!\nReboot required."),
    },
  };
  for (auto &[param, title, desc] : toggle_defs) {
    if (param == "") {
      auto label = new LabelControl(title, "");
      addItem(label);
      continue;
    }
    auto toggle = new ParamControl(param, title, desc, "", this);

    bool locked = params.getBool((param + "Lock").toStdString());
    toggle->setEnabled(!locked);

    addItem(toggle);
    toggles[param.toStdString()] = toggle;
  }
}

void DPCtrlPanel::add_lateral_toggles() {
  std::vector<std::tuple<QString, QString, QString>> toggle_defs{
    {
      "",
      QString::fromUtf8("🐉 ") + tr("Ctrl - Lateral") + QString::fromUtf8(" 🐉"),
      "",
    },
    {
      "dp_alka",
      tr("Enable ALKA"),
      tr("When enabled, openpilot lateral Control will be always on when ACC MAIN is ON.\nReboot required."),
    },
    {
      "dp_lat_lane_priority_mode",
      tr("Enable Lane Priority Mode"),
      tr("When enabled, openpilot will use lane lines for lateral control, fallback to laneless mode automatically when lane lines probabilities are low."),
    },
  };
  auto lane_change_assist_speed_toggle = new ParamSpinBoxControl("dp_lat_lane_change_assist_speed", tr("Lane Change Assist Activate Speed"), tr("Adjust the lane change assist activate speed.\nOff = Disable lane change assistance.\n1 mph = 1.6 kph"), "", 0, 80, 1, tr(" mph"), tr("Off"));
  speed_based_lane_priority_toggle = new ParamSpinBoxControl("dp_lat_lane_priority_mode_speed_based", tr("Only When Drive Above"),
                                                  tr("All Speed - Use Lane Line when available.\n*Number* - Use Lane Line when available and drive speed is above the *number*."),
                                                  "", 0, 120, 1, tr(" kph"), tr("All Speed"));

  std::vector<QString> dp_lat_controller_texts{tr("DEFAULT"), tr("INDI"), tr("LQR")};
  ButtonParamControl* dp_lat_controller_setting = new ButtonParamControl("dp_lat_controller", tr("Lateral Controller"),
                                          tr("Change your lateral controller.\nUSE AT YOUR OWN RISK!\nReboot required."),
                                          "",
                                          dp_lat_controller_texts);

  for (auto &[param, title, desc] : toggle_defs) {
    if (param == "") {
      auto label = new LabelControl(title, "");
      addItem(label);
      continue;
    }
    auto toggle = new ParamControl(param, title, desc, "", this);

    bool locked = params.getBool((param + "Lock").toStdString());
    toggle->setEnabled(!locked);

    addItem(toggle);
    toggles[param.toStdString()] = toggle;
    if (param == "dp_alka") {
      addItem(dp_lat_controller_setting);
    }
    else if (param == "dp_lat_lane_priority_mode") {
      connect(toggle, &ToggleControl::toggleFlipped, [=]() {
        updateToggles();
      });
      addItem(speed_based_lane_priority_toggle);
    }
  }
  addItem(lane_change_assist_speed_toggle);
}

void DPCtrlPanel::add_longitudinal_toggles() {
  std::vector<std::tuple<QString, QString, QString>> toggle_defs{
    {
      "",
      QString::fromUtf8("🐉 ") + tr("Ctrl - Longitudinal") + QString::fromUtf8(" 🐉"),
      "",
    },
    {
      "dp_long_use_df_tune",
      tr("Enable Dynamic Following"),
      tr("When enabled, openpilot will dynamically adjust following distance based on your \"Driving Personality\" setting.")
    },
    {
      "dp_long_use_krkeegen_tune",
      tr("Enable krkeegan Tune (SNG Boost)"),
      tr("When enabled, it will apply an offset to get_stopped_equivalence_factor in longitudinal MPC resulting in an early demand for acceleration.\nCredit to krkeegen: https://github.com/krkeegan/")
    },
    {
      "dp_long_de2e",
      tr("Enable Dynamic End-to-End Longitudinal Control"),
      tr("When enabled, openpilot will dynamically change between End-to-End or ACC mode.\nYour vehicle must support openpilot longitudinal control.")
    },
    {
      "dp_mapd_vision_turn_control",
      tr("Enable Turn Speed Control - Vision"),
      tr("When enabled, openpilot will use path predictions from camera vision to estimate the appropriate speed to drive through turns ahead."),
    },
    {
      "dp_mapd",
      tr("Enable MapD"),
      tr("When enabled, openpilot will display current road name and speed limit on the screen.\nNote: YOUR DRIVING ROUTE DATA WILL BE UPLOADED to openstreetmap server to help to improve map accuracy.\nReboot required."),
    },
//    {
//      "SpeedLimitControl",
//      QString::fromUtf8("₯ －　") + tr("MapD - Enable Speed Limit Control"),
//      tr("When enabled, openpilot will use speed limit signs information from map data to automatically adapt cruise speed to road limits.\nReboot required."),
//    },
    {
      "dp_long_personality_btn",
      tr("On-Screen Personality Button"),
      tr("When enabled, openpilot will display an on-screen personality button."),
    },
    {
      "dp_long_accel_btn",
      tr("On-Screen Accel Button"),
      tr("When enabled, openpilot will display an on-screen accel profile button."),
    },
  };
  std::vector<QString> dp_long_accel_profile_texts{tr("OP"), tr("ECO"), tr("NOR"), tr("SPT")};
  ButtonParamControl* dp_long_accel_profile_setting = new ButtonParamControl("dp_long_accel_profile", tr("Acceleration Profile"),
                                          tr("OP - Stock tune.\nECO - Eco tune.\nNOR - Normal tune.\nSPT - Sport tune."),
                                          "",
                                          dp_long_accel_profile_texts);
  for (auto &[param, title, desc] : toggle_defs) {
    if (param == "") {
      auto label = new LabelControl(title, "");
      addItem(label);
      continue;
    }
    auto toggle = new ParamControl(param, title, desc, "", this);

    bool locked = params.getBool((param + "Lock").toStdString());
    toggle->setEnabled(!locked);

    addItem(toggle);
    toggles[param.toStdString()] = toggle;
    if (param == "dp_mapd_vision_turn_control") {
      // add accel profile after it
      addItem(dp_long_accel_profile_setting);
    }
  }
}

void DPCtrlPanel::add_device_toggles() {
  std::vector<std::tuple<QString, QString, QString>> toggle_defs{
    {
      "",
      QString::fromUtf8("🐉 ") + tr("Device") + QString::fromUtf8(" 🐉"),
      "",
    },
    {
      "dp_device_disable_temp_check",
      tr("Disable Temp Check"),
      tr("When enabled, openpilot will disable device temperature check.\n**NOTED** An overheated device may result in random shutdowns or lag.\nReboot required."),
    },
    {
      "dp_device_no_ir_ctrl",
      tr("Disable IR"),
      tr("When enabled, openpilot will disable IR completely.\nReboot required."),
    },
    {
      "dp_device_auto_shutdown",
      tr("Enable Auto Shutdown"),
      tr("When enabled, openpilot will shutdown the device automatically.\nReboot required."),
    },
  };

  std::vector<QString> audible_alert_mode_texts{tr("Standard"), tr("Warning"), tr("Off")};
  ButtonParamControl* audible_alert_mode_setting = new ButtonParamControl("dp_device_audible_alert_mode", tr("Audible Alert Mode"),
                                          tr("Standard - Standard behaviour.\nWarning - Only emits sound when there is a warning.\nOff - Does not emit any sound at all."),
                                          "",
                                          audible_alert_mode_texts);


  auto_shutdown_timer_toggle = new ParamSpinBoxControl("dp_device_auto_shutdown_in", tr("Auto Shutdown In"), tr("Adjust your shutdown waiting period."), "", 0, 600, 1, tr(" mins"), tr("Immediately"));
  for (auto &[param, title, desc] : toggle_defs) {
    if (param == "") {
      auto label = new LabelControl(title, "");
      addItem(label);
      continue;
    }
    auto toggle = new ParamControl(param, title, desc, "", this);

    bool locked = params.getBool((param + "Lock").toStdString());
    toggle->setEnabled(!locked);

    addItem(toggle);
    toggles[param.toStdString()] = toggle;

    if (param == "dp_device_auto_shutdown") {
      connect(toggle, &ToggleControl::toggleFlipped, [=]() {
        updateToggles();
      });
      auto_shutdown_timer_toggle->setVisible(false);
      addItem(auto_shutdown_timer_toggle);
      // audible alert mode
      addItem(audible_alert_mode_setting);
    }
  }
}

void DPCtrlPanel::add_misc_toggles() {
  std::vector<std::tuple<QString, QString, QString>> toggle_defs{
    {
      "",
      QString::fromUtf8("🐉 ") + tr("Miscellaneous") + QString::fromUtf8(" 🐉"),
      "",
    },
  };
  add_generic_toggles(toggle_defs);
}

void DPCtrlPanel::add_toyota_toggles() {
  std::vector<std::tuple<QString, QString, QString>> toggle_defs{
    {
      "",
      QString::fromUtf8("🐉 ") + tr("Toyota / Lexus") + QString::fromUtf8(" 🐉"),
      "",
    },
    {
      "dp_toyota_sng",
      tr("Enable Stop and Go (SnG) Hack"),
      tr("When enabled, openpilot will stop sending standstill signal when the car is fully stopped.\nONLY WORK ON SOME VEHICLES.\nReboot Required."),
    },
    {
      "dp_toyota_enhanced_bsm",
      tr("Enable Enhanced BSM"),
      tr("When enabled, openpilot will use debugging CAN messages to receive unfiltered BSM signals, allowing detection of more objects.\nTested on Prius TSS2 and RAV4 TSS1.\nONLY WORK ON SOME VEHICLES.\nReboot Required."),
    },
    {
      "dp_toyota_auto_lock",
      tr("Enable Door Auto Locking"),
      tr("When enabled, openpilot will attempt to lock the doors when drive above 10 km/h (6.2 mph).\nReboot Required."),
    },
    {
      "dp_toyota_auto_unlock",
      tr("Enable Door Auto Unlocking"),
      tr("When enabled, openpilot will attempt to unlock the doors when shift to gear P.\nReboot Required."),
    },
    {
      "dp_toyota_zss",
      tr("Enable Zorro-Steering-Sensor (ZSS) Support"),
      tr("When enabled, openpilot will use sensor outputs from ZSS for more accurate steering control.\nDO NOT ENABLE unless you have ZSS installed.\nReboot Required."),
    },
  };
  for (auto &[param, title, desc] : toggle_defs) {
    if (param == "") {
      auto label = new LabelControl(title, "");
      addItem(label);
      continue;
    }
    auto toggle = new ParamControl(param, title, desc, "", this);

    bool locked = params.getBool((param + "Lock").toStdString());
    toggle->setEnabled(!locked);

    addItem(toggle);
    toggles[param.toStdString()] = toggle;
  }
}

void DPCtrlPanel::add_hkg_toggles() {
  std::vector<std::tuple<QString, QString, QString>> toggle_defs{
    {
      "",
      QString::fromUtf8("🐉 ") + tr("Hyundai / Kia / Genesis") + QString::fromUtf8(" 🐉"),
      "",
    },
    {
      "dp_hkg_min_steer_speed_bypass",
      tr("Enable Minimum Steer Speed Bypass"),
      tr("When enabled, openpilot will control the steering to 0 kph/mph.\nMDPS harness may required.\nReboot Required."),
    },
  };
  add_generic_toggles(toggle_defs);
}

void DPCtrlPanel::add_vag_toggles() {
  std::vector<std::tuple<QString, QString, QString>> toggle_defs{
    {
      "",
      QString::fromUtf8("🐉 ") + tr("VW / Skoda / Audi") + QString::fromUtf8(" 🐉"),
      "",
    },
    {
      "dp_vag_timebomb_bypass",
      tr("Enable Lateral Control Timebomb Bypass"),
      tr("When enabled, openpilot temporary disable lateral control when it reaches timebomb limit.\n At 5 mins 40 secs - Warning.\n At 5 mins 45 secs - Disable lateral control.\nAt 5 mins 48 secs - Resume lateral control.\nReboot Required."),
    },
  };
  add_generic_toggles(toggle_defs);
}

void DPCtrlPanel::add_car_specific_toggles() {
  add_toyota_toggles();
  add_hkg_toggles();
  add_vag_toggles();
}

DPCtrlPanel::DPCtrlPanel(QWidget *parent) : ListWidget(parent) {
  add_overall_toggles();
  add_lateral_toggles();
  add_longitudinal_toggles();
  add_car_specific_toggles();
  add_device_toggles();
  add_misc_toggles();

  auto resetBtn = new ButtonControl(tr("Reset Configuration"), tr("RESET"));
  connect(resetBtn, &ButtonControl::clicked, [&]() {
    if (ConfirmationDialog::confirm(tr("Are you sure you want to reset all dp configurations?"), tr("Reset"), this)) {
      params.putBool("dp_reset_conf", true);
    }
  });
  addItem(resetBtn);
}

void DPCtrlPanel::expandToggleDescription(const QString &param) {
  toggles[param.toStdString()]->showDescription();
}

void DPCtrlPanel::showEvent(QShowEvent *event) {
  updateToggles();
}

void DPCtrlPanel::updateToggles() {
  auto_shutdown_timer_toggle->setVisible(params.getBool("dp_device_auto_shutdown"));
  speed_based_lane_priority_toggle->setVisible(params.getBool("dp_lat_lane_priority_mode"));
}
