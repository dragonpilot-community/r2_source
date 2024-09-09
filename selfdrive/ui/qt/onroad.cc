#include "selfdrive/ui/qt/onroad.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>

#include <QDebug>
#include <QMouseEvent>

#include "common/timing.h"
#include "selfdrive/ui/qt/util.h"
#ifdef ENABLE_MAPS
#include "selfdrive/ui/qt/maps/map_helpers.h"
#include "selfdrive/ui/qt/maps/map_panel.h"
#endif

static void drawIcon(QPainter &p, const QPoint &center, const QPixmap &img, const QBrush &bg, float opacity) {
  p.setRenderHint(QPainter::Antialiasing);
  p.setOpacity(1.0);  // bg dictates opacity of ellipse
  p.setPen(Qt::NoPen);
  p.setBrush(bg);
  p.drawEllipse(center, btn_size / 2, btn_size / 2);
  p.setOpacity(opacity);
  p.drawPixmap(center - QPoint(img.width() / 2, img.height() / 2), img);
  p.setOpacity(1.0);
}

OnroadWindow::OnroadWindow(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *main_layout  = new QVBoxLayout(this);
  main_layout->setMargin(UI_BORDER_SIZE);
  QStackedLayout *stacked_layout = new QStackedLayout;
  stacked_layout->setStackingMode(QStackedLayout::StackAll);
  main_layout->addLayout(stacked_layout);

  nvg = new AnnotatedCameraWidget(VISION_STREAM_ROAD, this);

  QWidget * split_wrapper = new QWidget;
  split = new QHBoxLayout(split_wrapper);
  split->setContentsMargins(0, 0, 0, 0);
  split->setSpacing(0);
  split->addWidget(nvg);

  if (getenv("DUAL_CAMERA_VIEW")) {
    CameraWidget *arCam = new CameraWidget("camerad", VISION_STREAM_ROAD, true, this);
    split->insertWidget(0, arCam);
  }

  if (getenv("MAP_RENDER_VIEW")) {
    CameraWidget *map_render = new CameraWidget("navd", VISION_STREAM_MAP, false, this);
    split->insertWidget(0, map_render);
  }

  stacked_layout->addWidget(split_wrapper);

  alerts = new OnroadAlerts(this);
  alerts->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  stacked_layout->addWidget(alerts);

  // setup stacking order
  alerts->raise();

  setAttribute(Qt::WA_OpaquePaintEvent);
  QObject::connect(uiState(), &UIState::uiUpdate, this, &OnroadWindow::updateState);
  QObject::connect(uiState(), &UIState::offroadTransition, this, &OnroadWindow::offroadTransition);

  dp_alka = Params().getBool("dp_alka");
}

// Function to update indicator state
void OnroadWindow::updateIndicatorState(bool blinkerState, bool bsmState, bool& indicatorShow, int& indicatorCount, QColor& indicatorColor) {
  if (!blinkerState && !bsmState) {
    indicatorShow = false;
    indicatorCount = 0;
  } else {
    indicatorCount += 1;

    if (bsmState && blinkerState) {
      indicatorShow = indicatorCount % 4 == 0? !indicatorShow : indicatorShow;
      indicatorColor = dp_yellow_color;
    } else if (blinkerState) {
      indicatorShow = indicatorCount % 8 == 0? !indicatorShow : indicatorShow;
      indicatorColor = dp_green_color;
    } else {
      indicatorShow = bsmState;
      indicatorColor = dp_yellow_color;
    }
  }
}

void OnroadWindow::updateState(const UIState &s) {
  if (!s.scene.started) {
    return;
  }

  QColor bgColor = bg_colors[s.scene.lat_active && s.scene.alka_active && s.status == STATUS_DISENGAGED? STATUS_ALKA : s.status];
  Alert alert = Alert::get(*(s.sm), s.scene.started_frame);
  alerts->updateAlert(alert);

  if (s.scene.map_on_left) {
    split->setDirection(QBoxLayout::LeftToRight);
  } else {
    split->setDirection(QBoxLayout::RightToLeft);
  }

  nvg->updateState(s);

  // dp blinker & bsm
  auto cs = (*s.sm)["carState"].getCarState();
  dp_brake_pressed = cs.getBrakePressed();
  dp_blinker_left = cs.getLeftBlinker();
  dp_blinker_right = cs.getRightBlinker();
  dp_bsm_left = cs.getLeftBlindspot();
  dp_bsm_right = cs.getRightBlindspot();

  // left
  updateIndicatorState(dp_blinker_left, dp_bsm_left, dp_indicator_left_show, dp_indicator_left_count, dp_indicator_left_color);

  // right
  updateIndicatorState(dp_blinker_right, dp_bsm_right, dp_indicator_right_show, dp_indicator_right_count, dp_indicator_right_color);

  bool dp_repaint = dp_indicator_left_show || dp_indicator_right_show || dp_brake_pressed;

  // repaint border
  if (bg != bgColor || dp_repaint || dp_repaint != dp_repaint_prev) {
    // repaint border
    bg = bgColor;
    dp_repaint_prev = dp_repaint;
    update();
  }
}

void OnroadWindow::mousePressEvent(QMouseEvent* e) {
#ifdef ENABLE_MAPS
  if (map != nullptr) {
    // Switch between map and sidebar when using navigate on openpilot
    bool sidebarVisible = geometry().x() > 0;
    bool show_map = uiState()->scene.navigate_on_openpilot ? sidebarVisible : !sidebarVisible;
    map->setVisible(show_map && !map->isVisible());
  }
#endif
  // propagation event to parent(HomeWindow)
  QWidget::mousePressEvent(e);
}

void OnroadWindow::offroadTransition(bool offroad) {
#ifdef ENABLE_MAPS
  if (!offroad) {
    if (map == nullptr && (uiState()->primeType() || !MAPBOX_TOKEN.isEmpty())) {
      auto m = new MapPanel(get_mapbox_settings());
      map = m;

      QObject::connect(m, &MapPanel::mapPanelRequested, this, &OnroadWindow::mapPanelRequested);
      QObject::connect(nvg->map_settings_btn, &MapSettingsButton::clicked, m, &MapPanel::toggleMapSettings);
      nvg->map_settings_btn->setEnabled(true);

      m->setFixedWidth(topWidget(this)->width() / 2 - UI_BORDER_SIZE);
      split->insertWidget(0, m);

      // hidden by default, made visible when navRoute is published
      m->setVisible(false);
    }
  }
#endif

  alerts->updateAlert({});
}

void OnroadWindow::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  p.fillRect(rect(), QColor(bg.red(), bg.green(), bg.blue(), 255));

  // dp - draw brake
  if (dp_brake_pressed) {
    p.fillRect(
      QRect(0, height() - UI_BORDER_SIZE, width(), 30),
      QColor(0xff, 0, 0, 255)
    );
  }
  // dp - draw indicators
  if (dp_indicator_left_show) {
    p.fillRect(
      QRect(0, 0, width()*0.2, height()),
      dp_indicator_left_color
    );
  }
  if (dp_indicator_right_show) {
    p.fillRect(
      QRect(width()*0.8, 0, width()*0.2, height()),
      dp_indicator_right_color
    );
  }
}

// ***** onroad widgets *****

// OnroadAlerts
void OnroadAlerts::updateAlert(const Alert &a) {
  if (!alert.equal(a)) {
    alert = a;
    update();
  }
}

void OnroadAlerts::paintEvent(QPaintEvent *event) {
  if (alert.size == cereal::ControlsState::AlertSize::NONE) {
    return;
  }
  static std::map<cereal::ControlsState::AlertSize, const int> alert_heights = {
    {cereal::ControlsState::AlertSize::SMALL, 271},
    {cereal::ControlsState::AlertSize::MID, 420},
    {cereal::ControlsState::AlertSize::FULL, height()},
  };
  int h = alert_heights[alert.size];

  int margin = 40;
  int radius = 30;
  if (alert.size == cereal::ControlsState::AlertSize::FULL) {
    margin = 0;
    radius = 0;
  }
  QRect r = QRect(0 + margin, height() - h + margin, width() - margin*2, h - margin*2);

  QPainter p(this);

  // draw background + gradient
  p.setPen(Qt::NoPen);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);
  p.setBrush(QBrush(alert_colors[alert.status]));
//  p.drawRoundedRect(r, radius, radius);
  p.drawRect(r);

  QLinearGradient g(0, r.y(), 0, r.bottom());
  g.setColorAt(0, QColor::fromRgbF(0, 0, 0, 0.05));
  g.setColorAt(1, QColor::fromRgbF(0, 0, 0, 0.35));

  p.setCompositionMode(QPainter::CompositionMode_DestinationOver);
  p.setBrush(QBrush(g));
//  p.drawRoundedRect(r, radius, radius);
  p.drawRect(r);
  p.setCompositionMode(QPainter::CompositionMode_SourceOver);

  // text
  const QPoint c = r.center();
  p.setPen(QColor(0xff, 0xff, 0xff));
  p.setRenderHint(QPainter::TextAntialiasing);
  if (alert.size == cereal::ControlsState::AlertSize::SMALL) {
    p.setFont(InterFont(74, QFont::DemiBold));
    p.drawText(r, Qt::AlignCenter, alert.text1);
  } else if (alert.size == cereal::ControlsState::AlertSize::MID) {
    p.setFont(InterFont(88, QFont::Bold));
    p.drawText(QRect(0, c.y() - 125, width(), 150), Qt::AlignHCenter | Qt::AlignTop, alert.text1);
    p.setFont(InterFont(66));
    p.drawText(QRect(0, c.y() + 21, width(), 90), Qt::AlignHCenter, alert.text2);
  } else if (alert.size == cereal::ControlsState::AlertSize::FULL) {
    bool l = alert.text1.length() > 15;
    p.setFont(InterFont(l ? 132 : 177, QFont::Bold));
    p.drawText(QRect(0, r.y() + (l ? 240 : 270), width(), 600), Qt::AlignHCenter | Qt::TextWordWrap, alert.text1);
    p.setFont(InterFont(88));
    p.drawText(QRect(0, r.height() - (l ? 361 : 420), width(), 300), Qt::AlignHCenter | Qt::TextWordWrap, alert.text2);
  }
}

// ExperimentalButton
ExperimentalButton::ExperimentalButton(QWidget *parent) : experimental_mode(false), engageable(false), QPushButton(parent) {
  setFixedSize(btn_size, btn_size);

  params = Params();
  engage_img = loadBased64Image("iVBORw0KGgoAAAANSUhEUgAAAQoAAAEKCAMAAADdFev7AAAAVFBMVEVHcEz///////////////////////////////////////////////////////////////////////////////////////////////////////////+DS+nTAAAAG3RSTlMAKEIf6zXh+gTzUBYOCM3XwoaRcHq4nVymZbCV8JeNAAAgAElEQVR42uxc2WKrOgysAe/GNmaH///PC17AEHoKSdqk5x6eWkIKlqXRaCT68fHv+Hf8O/4dv+cg/0zgD9g1Rdmm/69F86Qvmq6p2GbdbJwPs/gGgTDl+V/tKqCQ1K56xKqIjNHbc1myGExmSGr29xoi79EYHXJZ+UflznR5MFk2/9r/tZYQBXYOQb1nZMu2G3eChhNwvgIvHxPG/i4cqawlZMVg0mu7dAT9R513lEF47JgvpXCNF6wM43+NJRLr9I3bXm5wFBF57U2BW3dta50GLF+1TqJM8iutQYTYJoB8mJdXh8UI+2vmNl7IgB/afV7OP6t0Fz8jlkUifpcd0tYMWndlHOAw24S/2+mxctcrGzrTGeygsrC/h1VztWIt1eXvcY08aZDDx1H1+XK63W70h7Bw0ZE1YZSzo0h7RWM9KHyZOZBR/q9K9ksIB+zouoe4yDf5Ukfu3aw77xOGdZTZT8iwminERwbTtnPJmFb5b7AEQz5heluU4YNibwprG2SdILEJIyFzIlEgeIwJ4aaCjxDQa7z5q+9sCevqyrRJW8c46FeuojhvV9wMCcPiSRHAodjEh1+9sCk5g+8fHbNPYJO6fDljHCAxt17zY8iQbEkYk4MQ48hGimKy2Yzx4vMu9pj3zaD2qY2PZF5FmY/4IIjqkZVcu4TBvSmN846RxfEx5Btvkt69CHhT2LBLUOnnnwUKtS6xXPZd594oGbQOE8zW4m09YuMF+ZtA1bxnrFhH73aVByTRyquoXF9M4RLGMF8H5pNNG+EB2caHdyFvivnDzIA3NEUTo53z31L7QsNlhSYKEBTcJP5oRtdsiDbe4QbWJXShYI3lXCjwtPr9GCjZIRpnDVo9odlxLOvoFkfjhJHOHBxH2ab1xGrM6hJynrhCrorI/PiOqbWLSCIBlXSrkPGalsd2RYhFQ7fx/pM+LN1xENJFCkemlOMryMUEo5t8/VZl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", {img_size, img_size});
  experimental_img = loadPixmap("../assets/img_experimental.svg", {img_size, img_size});
  QObject::connect(this, &QPushButton::clicked, this, &ExperimentalButton::changeMode);
}

void ExperimentalButton::changeMode() {
  const auto cp = (*uiState()->sm)["carParams"].getCarParams();
  bool can_change = hasLongitudinalControl(cp) && params.getBool("ExperimentalModeConfirmed");
  if (can_change) {
    params.putBool("ExperimentalMode", !experimental_mode);
  }
}

void ExperimentalButton::updateState(const UIState &s) {
  const auto cs = (*s.sm)["controlsState"].getControlsState();
  bool eng = cs.getEngageable() || cs.getEnabled();
  if ((cs.getExperimentalMode() != experimental_mode) || (eng != engageable)) {
    engageable = eng;
    experimental_mode = cs.getExperimentalMode();
    update();
  }
}

void ExperimentalButton::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  QPixmap img = experimental_mode ? experimental_img : engage_img;
  drawIcon(p, QPoint(btn_size / 2, btn_size / 2), img, QColor(0, 0, 0, 166), (isDown() || !engageable) ? 0.6 : 1.0);
}


// MapSettingsButton
MapSettingsButton::MapSettingsButton(QWidget *parent) : QPushButton(parent) {
  setFixedSize(btn_size, btn_size);
  settings_img = loadPixmap("../assets/navigation/icon_directions_outlined.svg", {img_size, img_size});

  // hidden by default, made visible if map is created (has prime or mapbox token)
  setVisible(false);
  setEnabled(false);
}

void MapSettingsButton::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  drawIcon(p, QPoint(btn_size / 2, btn_size / 2), settings_img, QColor(0, 0, 0, 166), isDown() ? 0.6 : 1.0);
}

static void drawOnScreenButton(QPainter &p, const QPoint &center, const QBrush &bg, float opacity, QString main, QString bottom, QString top) {
  p.setRenderHint(QPainter::Antialiasing);
  p.setOpacity(0.65);  // bg dictates opacity of ellipse
  p.setPen(Qt::NoPen);
  p.setBrush(bg);
  p.drawEllipse(center, btn_size / 2, btn_size / 2);
  p.setOpacity(opacity);

  p.setFont(InterFont(32, QFont::DemiBold));
  p.setPen(Qt::white);

  // Calculate the bounding rectangle for the text
  QRect textRect(center.x() - btn_size / 2, center.y() - btn_size * 0.45, btn_size, btn_size);

  // Draw the main text in the center of the button
  p.drawText(textRect, Qt::AlignCenter, main);

  // Optionally, draw subtext below the main text
  if (!bottom.isEmpty()) {
    p.setFont(InterFont(20, QFont::DemiBold));
    p.setPen(Qt::white);

    QRect bottomTextRect(center.x() - btn_size / 2, center.y() - btn_size * 0.2, btn_size, btn_size);
    p.drawText(bottomTextRect, Qt::AlignCenter, bottom);
  }

  if (!top.isEmpty()) {
    p.setFont(InterFont(56, QFont::DemiBold));
    p.setPen(Qt::white);

   QRect topTextRect(center.x() - btn_size / 2, center.y() - btn_size * 0.7, btn_size, btn_size);
    p.drawText(topTextRect, Qt::AlignCenter, top);
  }

  p.setOpacity(1.0);
}

PersonalityButton::PersonalityButton(QWidget *parent) : QPushButton(parent) {
  setFixedSize(btn_size, btn_size);
  setVisible(false);

  val = std::atoi(Params().get("LongitudinalPersonality").c_str());
  updateText();
  QObject::connect(this, &QPushButton::clicked, this, &PersonalityButton::changeMode);
}

void PersonalityButton::changeMode() {
  val += 1;
  val = val > VAL_MAX ? VAL_MIN : val;
  updateText();
  Params().put("LongitudinalPersonality", std::to_string(val));
}

void PersonalityButton::updateState(const UIState &s) {
  const auto cp = (*uiState()->sm)["carParams"].getCarParams();
  bool available = cp.getOpenpilotLongitudinalControl() && s.scene.dp_long_personality_btn;
  setVisible(available);
  if (available) {
    update();
  }
}

void PersonalityButton::updateText() {
  switch (val) {
    case 0:
      top = "A";
      main = tr("Aggressive");
      return;
    case 1:
      top = "S";
      main = tr("Standard");
      return;
    case 2:
      top = "R";
      main = tr("Relaxed");
      return;
  }
}

void PersonalityButton::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  drawOnScreenButton(p, QPoint(btn_size / 2, btn_size / 2), QColor(0, 0, 0, 166), isDown() ? 0.6 : 1.0, main, "PERSNLY", top);
}

AccelButton::AccelButton(QWidget *parent) : QPushButton(parent) {
  setFixedSize(btn_size, btn_size);
  setVisible(false);

  val = std::atoi(Params().get("dp_long_accel_profile").c_str());
  updateText();
  QObject::connect(this, &QPushButton::clicked, this, &AccelButton::changeMode);
}

void AccelButton::changeMode() {
  val += 1;
  val = val > VAL_MAX ? VAL_MIN : val;
  Params().put("dp_long_accel_profile", std::to_string(val));
  updateText();
}

void AccelButton::updateState(const UIState &s) {
  const auto cp = (*uiState()->sm)["carParams"].getCarParams();
  bool available = cp.getOpenpilotLongitudinalControl() && s.scene.dp_long_accel_btn;
  setVisible(available);
  if (available) {
    update();
  }
}

void AccelButton::updateText() {
  switch (val) {
    case 0:
      top = "O";
      main = tr("OP");
      return;
    case 1:
      top = "E";
      main = tr("ECO");
      return;
    case 2:
      top = "N";
      main = tr("NOR");
      return;
    default:
      top = "S";
      main = tr("SPT");
  }
}

void AccelButton::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  drawOnScreenButton(p, QPoint(btn_size / 2, btn_size / 2), QColor(0, 0, 0, 166), isDown() ? 0.6 : 1.0, main, "ACCEL", top);
}

// Window that shows camera view and variety of info drawn on top
AnnotatedCameraWidget::AnnotatedCameraWidget(VisionStreamType type, QWidget* parent) : fps_filter(UI_FREQ, 3, 1. / UI_FREQ), CameraWidget("camerad", type, true, parent) {
  pm = std::make_unique<PubMaster, const std::initializer_list<const char *>>({"uiDebug"});

  main_layout = new QVBoxLayout(this);
  main_layout->setMargin(UI_BORDER_SIZE);
  main_layout->setSpacing(0);

  experimental_btn = new ExperimentalButton(this);
  main_layout->addWidget(experimental_btn, 0, Qt::AlignTop | Qt::AlignRight);

  // Create a spacer item to push the following layout to the bottom
  QSpacerItem* vSpacer = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
  main_layout->addSpacerItem(vSpacer);

  // Create a horizontal layout for map_settings_btn, accel_btn, and personality_btn
  QHBoxLayout* horizontalLayout = new QHBoxLayout();

  // Add the spacer item to push the buttons to the right
  QSpacerItem* hSpacer = new QSpacerItem(1, 1, QSizePolicy::Expanding);
  horizontalLayout->addItem(hSpacer);

  accel_btn = new AccelButton(this);
  horizontalLayout->addWidget(accel_btn);
  horizontalLayout->addSpacing(UI_BORDER_SIZE);

  personality_btn = new PersonalityButton(this);
  horizontalLayout->addWidget(personality_btn);
  horizontalLayout->addSpacing(UI_BORDER_SIZE);

  map_settings_btn = new MapSettingsButton(this);
  horizontalLayout->addWidget(map_settings_btn);

  // Add the horizontal layout to the main_layout
  main_layout->addLayout(horizontalLayout);

  #ifdef QCOM
  dm_img = loadPixmap("../assets/img_driver_face_qcom.png", {img_size + 5, img_size + 5});
  #else
  dm_img = loadPixmap("../assets/img_driver_face.png", {img_size + 5, img_size + 5});
  #endif
  dp_no_gps_ctrl = Params().getBool("dp_no_gps_ctrl");
}

void AnnotatedCameraWidget::updateState(const UIState &s) {
  const int SET_SPEED_NA = 255;
  const SubMaster &sm = *(s.sm);

  const bool cs_alive = sm.alive("controlsState");
  const bool nav_alive = sm.alive("navInstruction") && sm["navInstruction"].getValid();

  const auto cs = sm["controlsState"].getControlsState();
  const auto car_state = sm["carState"].getCarState();
  const auto nav_instruction = sm["navInstruction"].getNavInstruction();

  // Handle older routes where vCruiseCluster is not set
  float v_cruise =  cs.getVCruiseCluster() == 0.0 ? cs.getVCruise() : cs.getVCruiseCluster();
  float set_speed = cs_alive ? v_cruise : SET_SPEED_NA;
  bool cruise_set = set_speed > 0 && (int)set_speed != SET_SPEED_NA;
  if (cruise_set && !s.scene.is_metric) {
    set_speed *= KM_TO_MILE;
  }

  // Handle older routes where vEgoCluster is not set
  float v_ego;
  if (car_state.getVEgoCluster() == 0.0 && !v_ego_cluster_seen) {
    v_ego = car_state.getVEgo();
  } else {
    v_ego = car_state.getVEgoCluster();
    v_ego_cluster_seen = true;
  }
  float cur_speed = cs_alive ? std::max<float>(0.0, v_ego) : 0.0;
  cur_speed *= s.scene.is_metric ? MS_TO_KPH : MS_TO_MPH;

  auto speed_limit_sign = nav_instruction.getSpeedLimitSign();
  float speed_limit = nav_alive ? nav_instruction.getSpeedLimit() : 0.0;
  speed_limit *= (s.scene.is_metric ? MS_TO_KPH : MS_TO_MPH);

  setProperty("speedLimit", speed_limit);
  setProperty("has_us_speed_limit", nav_alive && speed_limit_sign == cereal::NavInstruction::SpeedLimitSign::MUTCD);
  setProperty("has_eu_speed_limit", nav_alive && speed_limit_sign == cereal::NavInstruction::SpeedLimitSign::VIENNA);

  setProperty("is_cruise_set", cruise_set);
  setProperty("is_metric", s.scene.is_metric);
  setProperty("speed", cur_speed);
  setProperty("setSpeed", set_speed);
  setProperty("speedUnit", s.scene.is_metric ? tr("km/h") : tr("mph"));
//  setProperty("hideBottomIcons", (cs.getAlertSize() != cereal::ControlsState::AlertSize::NONE));
  if (!dp_device_no_ir_ctrl_checked) {
    dp_device_no_ir_ctrl_checked = true;
    dp_device_no_ir_ctrl = Params().getBool("dp_device_no_ir_ctrl");
  }
  if (!dp_device_no_ir_ctrl) {
    setProperty("hideBottomIcons", (cs.getAlertSize() != cereal::ControlsState::AlertSize::NONE));
  } else {
    setProperty("hideBottomIcons", true);
  }
  setProperty("status", s.status);

  // update engageability/experimental mode button
  experimental_btn->updateState(s);
  accel_btn->updateState(s);
  personality_btn->updateState(s);

  #ifndef QCOM
  // update DM icon
  auto dm_state = sm["driverMonitoringState"].getDriverMonitoringState();
  setProperty("dmActive", dm_state.getIsActiveMode());
  setProperty("rightHandDM", dm_state.getIsRHD());
  // DM icon transition
  dm_fade_state = std::clamp(dm_fade_state+0.2*(0.5-dmActive), 0.0, 1.0);

  // hide map settings button for alerts and flip for right hand DM
  if (map_settings_btn->isEnabled()) {
    map_settings_btn->setVisible(!hideBottomIcons);
    main_layout->setAlignment(map_settings_btn, (rightHandDM ? Qt::AlignLeft : Qt::AlignRight) | Qt::AlignBottom);
  }
  #else
  // update DM icons at 2Hz
  if (sm.frame % (UI_FREQ / 2) == 0) {
    setProperty("dmActive", sm["driverMonitoringState"].getDriverMonitoringState().getIsActiveMode());
    setProperty("rightHandDM", sm["driverMonitoringState"].getDriverMonitoringState().getIsRHD());
  }
  #endif

  // mapd
  auto lmd = sm["liveMapData"].getLiveMapData();
  setProperty("roadName", QString::fromStdString(lmd.getCurrentRoadName()));
  if (!nav_alive && lmd.getSpeedLimit() > 0) {
    speed_limit = lmd.getSpeedLimit() * (s.scene.is_metric ? MS_TO_KPH : MS_TO_MPH);
    setProperty("speedLimit", speed_limit);
    setProperty("has_eu_speed_limit", speed_limit > 1);
    setProperty("speed_limit_valid", lmd.getSpeedLimitValid());
  }
  // laneline mode
  setProperty("use_lanelines", sm["lateralPlan"].getLateralPlan().getUseLaneLines());
}

void AnnotatedCameraWidget::drawHud(QPainter &p) {
  p.save();

  // Header gradient
  QLinearGradient bg(0, UI_HEADER_HEIGHT - (UI_HEADER_HEIGHT / 2.5), 0, UI_HEADER_HEIGHT);
  bg.setColorAt(0, QColor::fromRgbF(0, 0, 0, 0.45));
  bg.setColorAt(1, QColor::fromRgbF(0, 0, 0, 0));
  p.fillRect(0, 0, width(), UI_HEADER_HEIGHT, bg);

  QString speedLimitStr = (speedLimit > 1) ? QString::number(std::nearbyint(speedLimit)) : "–";
  QString speedStr = QString::number(std::nearbyint(speed));
  QString setSpeedStr = is_cruise_set ? QString::number(std::nearbyint(setSpeed)) : "–";

  // Draw outer box + border to contain set speed and speed limit
  const int sign_margin = 12;
  const int us_sign_height = 186;
  const int eu_sign_size = 176;

  const QSize default_size = {172, 204};
  QSize set_speed_size = default_size;
  if (is_metric || has_eu_speed_limit) set_speed_size.rwidth() = 200;
  if (has_us_speed_limit && speedLimitStr.size() >= 3) set_speed_size.rwidth() = 223;

  if (has_us_speed_limit) set_speed_size.rheight() += us_sign_height + sign_margin;
  else if (has_eu_speed_limit) set_speed_size.rheight() += eu_sign_size + sign_margin;

  int top_radius = 32;
  int bottom_radius = has_eu_speed_limit ? 100 : 32;

  QRect set_speed_rect(QPoint(60 + (default_size.width() - set_speed_size.width()) / 2, 45), set_speed_size);
  p.setPen(QPen(whiteColor(75), 6));
  p.setBrush(blackColor(166));
  drawRoundedRect(p, set_speed_rect, top_radius, top_radius, bottom_radius, bottom_radius);

  // Draw MAX
  QColor max_color = QColor(0x80, 0xd8, 0xa6, 0xff);
  QColor set_speed_color = whiteColor();
  if (is_cruise_set) {
    if (status == STATUS_DISENGAGED) {
      max_color = whiteColor();
    } else if (status == STATUS_OVERRIDE) {
      max_color = QColor(0x91, 0x9b, 0x95, 0xff);
    } else if (speedLimit > 0) {
      auto interp_color = [=](QColor c1, QColor c2, QColor c3) {
        return speedLimit > 0 ? interpColor(setSpeed, {speedLimit + 5, speedLimit + 15, speedLimit + 25}, {c1, c2, c3}) : c1;
      };
      max_color = interp_color(max_color, QColor(0xff, 0xe4, 0xbf), QColor(0xff, 0xbf, 0xbf));
      set_speed_color = interp_color(set_speed_color, QColor(0xff, 0x95, 0x00), QColor(0xff, 0x00, 0x00));
    }
  } else {
    max_color = QColor(0xa6, 0xa6, 0xa6, 0xff);
    set_speed_color = QColor(0x72, 0x72, 0x72, 0xff);
  }
  p.setFont(InterFont(40, QFont::DemiBold));
  p.setPen(max_color);
  p.drawText(set_speed_rect.adjusted(0, 27, 0, 0), Qt::AlignTop | Qt::AlignHCenter, tr("MAX"));
  p.setFont(InterFont(90, QFont::Bold));
  p.setPen(set_speed_color);
  p.drawText(set_speed_rect.adjusted(0, 77, 0, 0), Qt::AlignTop | Qt::AlignHCenter, setSpeedStr);

  const QRect sign_rect = set_speed_rect.adjusted(sign_margin, default_size.height(), -sign_margin, -sign_margin);
  // US/Canada (MUTCD style) sign
  if (has_us_speed_limit) {
    p.setPen(Qt::NoPen);
    p.setBrush(whiteColor());
    p.drawRoundedRect(sign_rect, 24, 24);
    p.setPen(QPen(blackColor(), 6));
    p.drawRoundedRect(sign_rect.adjusted(9, 9, -9, -9), 16, 16);

    p.setFont(InterFont(28, QFont::DemiBold));
    p.drawText(sign_rect.adjusted(0, 22, 0, 0), Qt::AlignTop | Qt::AlignHCenter, tr("SPEED"));
    p.drawText(sign_rect.adjusted(0, 51, 0, 0), Qt::AlignTop | Qt::AlignHCenter, tr("LIMIT"));
    p.setFont(InterFont(70, QFont::Bold));
    p.drawText(sign_rect.adjusted(0, 85, 0, 0), Qt::AlignTop | Qt::AlignHCenter, speedLimitStr);
  }

  // EU (Vienna style) sign
  if (has_eu_speed_limit) {
    p.setPen(Qt::NoPen);
    p.setBrush(whiteColor());
    p.drawEllipse(sign_rect);
    p.setPen(QPen(Qt::red, 20));
    p.drawEllipse(sign_rect.adjusted(16, 16, -16, -16));

    p.setFont(InterFont((speedLimitStr.size() >= 3) ? 60 : 70, QFont::Bold));
    p.setPen(blackColor());
    p.drawText(sign_rect, Qt::AlignCenter, speedLimitStr);
  }

  // current speed
  p.setFont(InterFont(176, QFont::Bold));
  drawText(p, rect().center().x(), 210, speedStr);
  p.setFont(InterFont(66));
  drawText(p, rect().center().x(), 290, speedUnit, 200);

  // dm icon
  if (!hideBottomIcons) {
    int offset = UI_BORDER_SIZE + btn_size / 2;
    int x = rightHandDM ? width() - offset : offset;
    int y = height() - offset;
    drawIcon(p, QPoint(x, y), dm_img, blackColor(70), dmActive ? 1.0 : 0.2);
  }

  p.restore();
}

void AnnotatedCameraWidget::drawText(QPainter &p, int x, int y, const QString &text, int alpha) {
  QRect real_rect = p.fontMetrics().boundingRect(text);
  real_rect.moveCenter({x, y - real_rect.height() / 2});

  p.setPen(QColor(0xff, 0xff, 0xff, alpha));
  p.drawText(real_rect.x(), real_rect.bottom(), text);
}

void AnnotatedCameraWidget::initializeGL() {
  CameraWidget::initializeGL();
  qInfo() << "OpenGL version:" << QString((const char*)glGetString(GL_VERSION));
  qInfo() << "OpenGL vendor:" << QString((const char*)glGetString(GL_VENDOR));
  qInfo() << "OpenGL renderer:" << QString((const char*)glGetString(GL_RENDERER));
  qInfo() << "OpenGL language version:" << QString((const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));

  prev_draw_t = millis_since_boot();
  setBackgroundColor(bg_colors[STATUS_DISENGAGED]);
}

void AnnotatedCameraWidget::updateFrameMat() {
  CameraWidget::updateFrameMat();
  UIState *s = uiState();
  int w = width(), h = height();

  s->fb_w = w;
  s->fb_h = h;

  #ifdef QCOM
  auto intrinsic_matrix = FCAM_INTRINSIC_MATRIX;
  float zoom = ZOOM / intrinsic_matrix.v[0];
  s->car_space_transform.reset();
  s->car_space_transform.translate(w / 2, h / 2 + y_offset)
      .scale(zoom, zoom)
      .translate(-intrinsic_matrix.v[2], -intrinsic_matrix.v[5]);
  #else

  // Apply transformation such that video pixel coordinates match video
  // 1) Put (0, 0) in the middle of the video
  // 2) Apply same scaling as video
  // 3) Put (0, 0) in top left corner of video
  s->car_space_transform.reset();
  s->car_space_transform.translate(w / 2 - x_offset, h / 2 - y_offset)
      .scale(zoom, zoom)
      .translate(-intrinsic_matrix.v[2], -intrinsic_matrix.v[5]);
  #endif
}

void AnnotatedCameraWidget::drawLaneLines(QPainter &painter, const UIState *s) {
  painter.save();

  const UIScene &scene = s->scene;
  SubMaster &sm = *(s->sm);

  // lanelines
  for (int i = 0; i < std::size(scene.lane_line_vertices); ++i) {
    if (use_lanelines) {
      painter.setBrush(QColor::fromRgbF(0.0, 1.0, 0.0, std::clamp<float>(scene.lane_line_probs[i], 0.0, 0.7)));
    } else {
      painter.setBrush(QColor::fromRgbF(1.0, 1.0, 1.0, std::clamp<float>(scene.lane_line_probs[i], 0.0, 0.7)));
    }
    painter.drawPolygon(scene.lane_line_vertices[i]);
  }

  // road edges
  for (int i = 0; i < std::size(scene.road_edge_vertices); ++i) {
    painter.setBrush(QColor::fromRgbF(1.0, 0, 0, std::clamp<float>(1.0 - scene.road_edge_stds[i], 0.0, 1.0)));
    painter.drawPolygon(scene.road_edge_vertices[i]);
  }

  // paint path
  #ifndef QCOM
  QLinearGradient bg(0, height(), 0, 0);
  if (sm["controlsState"].getControlsState().getExperimentalMode()) {
    // The first half of track_vertices are the points for the right side of the path
    // and the indices match the positions of accel from uiPlan
    const auto &acceleration = sm["uiPlan"].getUiPlan().getAccel();
    const int max_len = std::min<int>(scene.track_vertices.length() / 2, acceleration.size());

    for (int i = 0; i < max_len; ++i) {
      // Some points are out of frame
      if (scene.track_vertices[i].y() < 0 || scene.track_vertices[i].y() > height()) continue;

      // Flip so 0 is bottom of frame
      float lin_grad_point = (height() - scene.track_vertices[i].y()) / height();

      // speed up: 120, slow down: 0
      float path_hue = fmax(fmin(60 + acceleration[i] * 35, 120), 0);
      // FIXME: painter.drawPolygon can be slow if hue is not rounded
      path_hue = int(path_hue * 100 + 0.5) / 100;

      float saturation = fmin(fabs(acceleration[i] * 1.5), 1);
      float lightness = util::map_val(saturation, 0.0f, 1.0f, 0.95f, 0.62f);  // lighter when grey
      float alpha = util::map_val(lin_grad_point, 0.75f / 2.f, 0.75f, 0.4f, 0.0f);  // matches previous alpha fade
      bg.setColorAt(lin_grad_point, QColor::fromHslF(path_hue / 360., saturation, lightness, alpha));

      // Skip a point, unless next is last
      i += (i + 2) < max_len ? 1 : 0;
    }
  }
  #else
  QLinearGradient bg(0, height(), 0, height() / 4);
  if (sm["controlsState"].getControlsState().getExperimentalMode() && sm["longitudinalPlanExt"].getLongitudinalPlanExt().getDpE2EIsBlended()) {
    float start_hue, end_hue;
    const auto &acceleration = sm["modelV2"].getModelV2().getAcceleration();
    float acceleration_future = 0;
    if (acceleration.getZ().size() > 16) {
      acceleration_future = acceleration.getX()[16];  // 2.5 seconds
    }
    start_hue = 60;
    // speed up: 120, slow down: 0
    end_hue = fmax(fmin(start_hue + acceleration_future * 45, 148), 0);

    // FIXME: painter.drawPolygon can be slow if hue is not rounded
    end_hue = int(end_hue * 100 + 0.5) / 100;

    bg.setColorAt(0.0, QColor::fromHslF(start_hue / 360., 0.97, 0.56, 0.4));
    bg.setColorAt(0.5, QColor::fromHslF(end_hue / 360., 1.0, 0.68, 0.35));
    bg.setColorAt(1.0, QColor::fromHslF(end_hue / 360., 1.0, 0.68, 0.0));
  }
  #endif
  else {
    bg.setColorAt(0.0, QColor::fromHslF(148 / 360., 0.94, 0.51, 0.4));
    bg.setColorAt(0.5, QColor::fromHslF(112 / 360., 1.0, 0.68, 0.35));
    bg.setColorAt(1.0, QColor::fromHslF(112 / 360., 1.0, 0.68, 0.0));
  }

  painter.setBrush(bg);
  painter.drawPolygon(scene.track_vertices);
  drawKnightScanner(painter);

  painter.restore();
}

void AnnotatedCameraWidget::drawKnightScanner(QPainter &painter) {
    UIState *s = uiState();
    int widgetHeight = rect().height();
    float halfHeightAbs = std::abs(s->scene.dpAccel) * widgetHeight;
    const float scannerWidth = 15;
    QRect scannerRect;

    if (s->scene.dpAccel > 0) {
        painter.setBrush(QColor(0, 245, 0, 200));
        // Move scanner to the left side
        scannerRect = QRect(0, widgetHeight / 2 - halfHeightAbs / 2, scannerWidth, halfHeightAbs / 2);
    } else {
        painter.setBrush(QColor(245, 0, 0, 200));
        // Move scanner to the left side
        scannerRect = QRect(0, widgetHeight / 2, scannerWidth, halfHeightAbs / 2);
    }

    painter.drawRect(scannerRect);
}

#ifndef QCOM
void AnnotatedCameraWidget::drawDriverState(QPainter &painter, const UIState *s) {
  const UIScene &scene = s->scene;

  painter.save();

  // base icon
  int offset = UI_BORDER_SIZE + btn_size / 2;
  int x = rightHandDM ? width() - offset : offset;
  int y = height() - offset;
  float opacity = dmActive ? 0.65 : 0.2;
  drawIcon(painter, QPoint(x, y), dm_img, blackColor(70), opacity);

  // face
  QPointF face_kpts_draw[std::size(default_face_kpts_3d)];
  float kp;
  for (int i = 0; i < std::size(default_face_kpts_3d); ++i) {
    kp = (scene.face_kpts_draw[i].v[2] - 8) / 120 + 1.0;
    face_kpts_draw[i] = QPointF(scene.face_kpts_draw[i].v[0] * kp + x, scene.face_kpts_draw[i].v[1] * kp + y);
  }

  painter.setPen(QPen(QColor::fromRgbF(1.0, 1.0, 1.0, opacity), 5.2, Qt::SolidLine, Qt::RoundCap));
  painter.drawPolyline(face_kpts_draw, std::size(default_face_kpts_3d));

  // tracking arcs
  const int arc_l = 133;
  const float arc_t_default = 6.7;
  const float arc_t_extend = 12.0;
  QColor arc_color = QColor::fromRgbF(0.545 - 0.445 * s->engaged(),
                                      0.545 + 0.4 * s->engaged(),
                                      0.545 - 0.285 * s->engaged(),
                                      0.4 * (1.0 - dm_fade_state));
  float delta_x = -scene.driver_pose_sins[1] * arc_l / 2;
  float delta_y = -scene.driver_pose_sins[0] * arc_l / 2;
  painter.setPen(QPen(arc_color, arc_t_default+arc_t_extend*fmin(1.0, scene.driver_pose_diff[1] * 5.0), Qt::SolidLine, Qt::RoundCap));
  painter.drawArc(QRectF(std::fmin(x + delta_x, x), y - arc_l / 2, fabs(delta_x), arc_l), (scene.driver_pose_sins[1]>0 ? 90 : -90) * 16, 180 * 16);
  painter.setPen(QPen(arc_color, arc_t_default+arc_t_extend*fmin(1.0, scene.driver_pose_diff[0] * 5.0), Qt::SolidLine, Qt::RoundCap));
  painter.drawArc(QRectF(x - arc_l / 2, std::fmin(y + delta_y, y), arc_l, fabs(delta_y)), (scene.driver_pose_sins[0]>0 ? 0 : 180) * 16, 180 * 16);

  painter.restore();
}
#endif
void AnnotatedCameraWidget::drawLead(QPainter &painter, const cereal::RadarState::LeadData::Reader &lead_data, const QPointF &vd, float v_ego) {
  painter.save();

  const float speedBuff = 10.;
  const float leadBuff = 40.;
  const float d_rel = lead_data.getDRel();
  const float v_rel = lead_data.getVRel();

  float fillAlpha = 0;
  if (d_rel < leadBuff) {
    fillAlpha = 255 * (1.0 - (d_rel / leadBuff));
    if (v_rel < 0) {
      fillAlpha += 255 * (-1 * (v_rel / speedBuff));
    }
    fillAlpha = (int)(fmin(fillAlpha, 255));
  }

  float sz = std::clamp((25 * 30) / (d_rel / 3 + 30), 15.0f, 30.0f) * 2.35;
  float x = std::clamp((float)vd.x(), 0.f, width() - sz / 2);
  float y = std::fmin(height() - sz * .6, (float)vd.y());

  float g_xo = sz / 5;
  float g_yo = sz / 10;

  QPointF glow[] = {{x + (sz * 1.35) + g_xo, y + sz + g_yo}, {x, y - g_yo}, {x - (sz * 1.35) - g_xo, y + sz + g_yo}};
  painter.setBrush(QColor(218, 202, 37, 255));
  painter.drawPolygon(glow, std::size(glow));

  // chevron
  QPointF chevron[] = {{x + (sz * 1.25), y + sz}, {x, y}, {x - (sz * 1.25), y + sz}};
  painter.setBrush(redColor(fillAlpha));
  painter.drawPolygon(chevron, std::size(chevron));

  // DP: Chevron detailed info.
  if (d_rel > 0) {
    QString dist = is_metric? QString::number(d_rel,'f',1) + "m" : QString::number(d_rel*3.2808,'f',1) + "ft";
    int str_w = 350;
    painter.setFont(InterFont(40, QFont::Bold));
    painter.setPen(blackColor(200)); //Shadow
    painter.drawText(QRect(x+4-(str_w/2), y+42, str_w, 50), Qt::AlignVCenter | Qt::AlignCenter, dist);
    painter.setPen(whiteColor());
    painter.drawText(QRect(x+2-(str_w/2), y+40, str_w, 50), Qt::AlignVCenter | Qt::AlignCenter, dist);
    painter.setPen(Qt::NoPen);

    if (d_rel > 0 && v_ego > 0) {
      float ttc = d_rel / v_ego;
      if (ttc < 5) {
        QString ttc_str = QString::number(ttc, 'f', 1) + "s";
        painter.setFont(InterFont(56, QFont::Bold));
        painter.setPen(blackColor(200)); //Shadow
        painter.drawText(QRect(x+4-(str_w/2), y+87, str_w, 50), Qt::AlignVCenter | Qt::AlignCenter, ttc_str);
        painter.setPen(whiteColor());
        painter.drawText(QRect(x+2-(str_w/2), y+85, str_w, 50), Qt::AlignVCenter | Qt::AlignCenter, ttc_str);
        painter.setPen(Qt::NoPen);
      }
    }
  }
  painter.restore();
}

void AnnotatedCameraWidget::drawRoadName(QPainter &p) {
  p.setFont(InterFont(55, QFont::Bold));
  drawText(p, rect().center().x(), rect().bottom() - 20, roadName, 200);
}

#ifndef QCOM
void AnnotatedCameraWidget::paintGL() {
  UIState *s = uiState();
  SubMaster &sm = *(s->sm);
  const double start_draw_t = millis_since_boot();
  const cereal::ModelDataV2::Reader &model = sm["modelV2"].getModelV2();
  const cereal::RadarState::Reader &radar_state = sm["radarState"].getRadarState();

  // draw camera frame
  {
    std::lock_guard lk(frame_lock);

    if (frames.empty()) {
      if (skip_frame_count > 0) {
        skip_frame_count--;
        qDebug() << "skipping frame, not ready";
        return;
      }
    } else {
      // skip drawing up to this many frames if we're
      // missing camera frames. this smooths out the
      // transitions from the narrow and wide cameras
      skip_frame_count = 5;
    }

    // Wide or narrow cam dependent on speed
    bool has_wide_cam = available_streams.count(VISION_STREAM_WIDE_ROAD);
    if (has_wide_cam) {
      float v_ego = sm["carState"].getCarState().getVEgo();
      if ((v_ego < 10) || available_streams.size() == 1) {
        wide_cam_requested = true;
      } else if (v_ego > 15) {
        wide_cam_requested = false;
      }
      wide_cam_requested = wide_cam_requested && sm["controlsState"].getControlsState().getExperimentalMode();
      // for replay of old routes, never go to widecam
      wide_cam_requested = wide_cam_requested && s->scene.calibration_wide_valid;
    }
    CameraWidget::setStreamType(wide_cam_requested ? VISION_STREAM_WIDE_ROAD : VISION_STREAM_ROAD);

    s->scene.wide_cam = CameraWidget::getStreamType() == VISION_STREAM_WIDE_ROAD;
    if (s->scene.calibration_valid) {
      auto calib = s->scene.wide_cam ? s->scene.view_from_wide_calib : s->scene.view_from_calib;
      CameraWidget::updateCalibration(calib);
    } else {
      CameraWidget::updateCalibration(DEFAULT_CALIBRATION);
    }
    CameraWidget::setFrameId(model.getFrameId());
    CameraWidget::paintGL();
  }

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(Qt::NoPen);

  if (s->worldObjectsVisible()) {
    if (sm.rcv_frame("modelV2") > s->scene.started_frame) {
      update_model(s, model, sm["uiPlan"].getUiPlan());
      if (sm.rcv_frame("radarState") > s->scene.started_frame) {
        update_leads(s, radar_state, model.getPosition());
      }
    }

    drawLaneLines(painter, s);

    if (s->scene.longitudinal_control) {
      const cereal::CarState::Reader &car_state = sm["carState"].getCarState();
      float v_ego = car_state.getVEgo();
      auto lead_one = radar_state.getLeadOne();
      auto lead_two = radar_state.getLeadTwo();
      if (lead_one.getStatus()) {
        drawLead(painter, lead_one, s->scene.lead_vertices[0], v_ego); //, 0, radar_state.getLeadOne().getDRel(), v_ego, radar_state.getLeadOne().getVRel(), s->scene.is_metric);
      }
      if (lead_two.getStatus() && (std::abs(lead_one.getDRel() - lead_two.getDRel()) > 3.0)) {
        drawLead(painter, lead_two, s->scene.lead_vertices[1], v_ego); //, 1, radar_state.getLeadOne().getDRel(), v_ego, radar_state.getLeadTwo().getVRel(), s->scene.is_metric);
      }
    }
  }

  // DMoji
  if (!hideBottomIcons && (sm.rcv_frame("driverStateV2") > s->scene.started_frame)) {
    update_dmonitoring(s, sm["driverStateV2"].getDriverStateV2(), dm_fade_state, rightHandDM);
    drawDriverState(painter, s);
  }

  drawHud(painter);

  double cur_draw_t = millis_since_boot();
  double dt = cur_draw_t - prev_draw_t;
  double fps = fps_filter.update(1. / dt * 1000);
  if (fps < 15) {
    LOGW("slow frame rate: %.2f fps", fps);
  }
  prev_draw_t = cur_draw_t;

  // publish debug msg
  MessageBuilder msg;
  auto m = msg.initEvent().initUiDebug();
  m.setDrawTimeMillis(cur_draw_t - start_draw_t);
  pm->send("uiDebug", msg);
}
#else
void AnnotatedCameraWidget::paintGL() {
  UIState *s = uiState();
  SubMaster &sm = *(s->sm);
  const cereal::ModelDataV2::Reader &model = sm["modelV2"].getModelV2();
  const cereal::RadarState::Reader &radar_state = sm["radarState"].getRadarState();

  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.setPen(Qt::NoPen);

  CameraWidget::setStreamType(VISION_STREAM_ROAD);

  if (s->scene.calibration_valid) {
    CameraWidget::updateCalibration(s->scene.view_from_calib);
  } else {
    CameraWidget::updateCalibration(DEFAULT_CALIBRATION);
  }
  CameraWidget::setFrameId(model.getFrameId());
  CameraWidget::paintGL();

  if (s->worldObjectsVisible()) {
    if (sm.rcv_frame("modelV2") > s->scene.started_frame) {
      update_model(s, sm["modelV2"].getModelV2(), sm["uiPlan"].getUiPlan());
      if (sm.rcv_frame("radarState") > s->scene.started_frame) {
        update_leads(s, radar_state, sm["modelV2"].getModelV2().getPosition());
      }
    }
    drawLaneLines(painter, s);

    if (s->scene.longitudinal_control) {
      const cereal::CarState::Reader &car_state = sm["carState"].getCarState();
      float v_ego = car_state.getVEgo();
      auto lead_one = radar_state.getLeadOne();
      auto lead_two = radar_state.getLeadTwo();
      if (lead_one.getStatus()) {
        drawLead(painter, lead_one, s->scene.lead_vertices[0], v_ego);
      }
      if (lead_two.getStatus() && (std::abs(lead_one.getDRel() - lead_two.getDRel()) > 3.0)) {
        drawLead(painter, lead_two, s->scene.lead_vertices[1], v_ego);
      }
    }
  }

  drawHud(painter);
  drawRoadName(painter);

  double cur_draw_t = millis_since_boot();
  double dt = cur_draw_t - prev_draw_t;
  double fps = fps_filter.update(1. / dt * 1000);
  if (fps < 15) {
    LOGW("slow frame rate: %.2f fps", fps);
  }
  prev_draw_t = cur_draw_t;
}
#endif

void AnnotatedCameraWidget::showEvent(QShowEvent *event) {
  CameraWidget::showEvent(event);

  ui_update_params(uiState());
  prev_draw_t = millis_since_boot();
}
