#pragma once

#include <memory>

#include <QPushButton>
#include <QStackedLayout>
#include <QWidget>

#include "common/util.h"
#include "selfdrive/ui/ui.h"
#ifdef QCOM
#include "selfdrive/ui/qt/widgets/cameraview_qcom.h"
#else
#include "selfdrive/ui/qt/widgets/cameraview.h"
#endif

const int btn_size = 192;
const int img_size = (btn_size / 4) * 3;


// ***** onroad widgets *****
class OnroadAlerts : public QWidget {
  Q_OBJECT

public:
  OnroadAlerts(QWidget *parent = 0) : QWidget(parent) {}
  void updateAlert(const Alert &a);

protected:
  void paintEvent(QPaintEvent*) override;

private:
  QColor bg;
  Alert alert = {};
};

class ExperimentalButton : public QPushButton {
  Q_OBJECT

public:
  explicit ExperimentalButton(QWidget *parent = 0);
  void updateState(const UIState &s);

private:
  void paintEvent(QPaintEvent *event) override;
  void changeMode();

  Params params;
  QPixmap engage_img;
  QPixmap experimental_img;
  bool experimental_mode;
  bool engageable;
};

class PersonalityButton : public QPushButton {
  Q_OBJECT
  const int VAL_MIN = 0;
  const int VAL_MAX = 2;

public:
  explicit PersonalityButton(QWidget *parent = 0);
  void updateState(const UIState &s);
  void updateText();

private:
  void paintEvent(QPaintEvent *event) override;
  void changeMode();

  Params params;
  QString main;
  QString top;
  int val;
};

class AccelButton : public QPushButton {
  Q_OBJECT
  const int VAL_MIN = 0;
  const int VAL_MAX = 3;

public:
  explicit AccelButton(QWidget *parent = 0);
  void updateState(const UIState &s);

private:
  void paintEvent(QPaintEvent *event) override;
  void changeMode();
  void updateText();

  Params params;
  QString main;
  QString top;
  int val;
};

class MapSettingsButton : public QPushButton {
  Q_OBJECT

public:
  explicit MapSettingsButton(QWidget *parent = 0);

private:
  void paintEvent(QPaintEvent *event) override;

  QPixmap settings_img;
};

// container window for the NVG UI
class AnnotatedCameraWidget : public CameraWidget {
  Q_OBJECT
  Q_PROPERTY(float speed MEMBER speed);
  Q_PROPERTY(QString speedUnit MEMBER speedUnit);
  Q_PROPERTY(float setSpeed MEMBER setSpeed);
  Q_PROPERTY(float speedLimit MEMBER speedLimit);
  Q_PROPERTY(bool is_cruise_set MEMBER is_cruise_set);
  Q_PROPERTY(bool has_eu_speed_limit MEMBER has_eu_speed_limit);
  Q_PROPERTY(bool has_us_speed_limit MEMBER has_us_speed_limit);
  Q_PROPERTY(bool is_metric MEMBER is_metric);

  Q_PROPERTY(bool dmActive MEMBER dmActive);
  Q_PROPERTY(bool hideBottomIcons MEMBER hideBottomIcons);
  Q_PROPERTY(bool rightHandDM MEMBER rightHandDM);
  Q_PROPERTY(int status MEMBER status);

  Q_PROPERTY(QString roadName MEMBER roadName);
  Q_PROPERTY(bool use_lanelines MEMBER use_lanelines);
  Q_PROPERTY(bool speed_limit_valid MEMBER speed_limit_valid);

public:
  explicit AnnotatedCameraWidget(VisionStreamType type, QWidget* parent = 0);
  void updateState(const UIState &s);

  MapSettingsButton *map_settings_btn;

private:
  void drawText(QPainter &p, int x, int y, const QString &text, int alpha = 255);

  QVBoxLayout *main_layout;
  ExperimentalButton *experimental_btn;
  QPixmap dm_img;
  float speed;
  QString speedUnit;
  float setSpeed;
  float speedLimit;
  bool is_cruise_set = false;
  bool is_metric = false;
  bool dmActive = false;
  bool hideBottomIcons = false;
  bool rightHandDM = false;
  float dm_fade_state = 1.0;
  bool has_us_speed_limit = false;
  bool has_eu_speed_limit = false;
  bool v_ego_cluster_seen = false;
  int status = STATUS_DISENGAGED;
  std::unique_ptr<PubMaster> pm;

  int skip_frame_count = 0;
  bool wide_cam_requested = false;

  QString roadName;
  bool dp_device_no_ir_ctrl = false;
  bool dp_device_no_ir_ctrl_checked = false;
  bool use_lanelines = false;
  bool speed_limit_valid = false;
  AccelButton *accel_btn;
  PersonalityButton *personality_btn;

  bool dp_no_gps_ctrl = false;

protected:
  void paintGL() override;
  void initializeGL() override;
  void showEvent(QShowEvent *event) override;
  void updateFrameMat() override;
  void drawLaneLines(QPainter &painter, const UIState *s);
  void drawLead(QPainter &painter, const cereal::RadarState::LeadData::Reader &lead_data, const QPointF &vd, float v_ego);
  void drawHud(QPainter &p);
  #ifndef QCOM
  void drawDriverState(QPainter &painter, const UIState *s);
  #endif
  void drawRoadName(QPainter &p);
  inline QColor redColor(int alpha = 255) { return QColor(201, 34, 49, alpha); }
  inline QColor whiteColor(int alpha = 255) { return QColor(255, 255, 255, alpha); }
  inline QColor blackColor(int alpha = 255) { return QColor(0, 0, 0, alpha); }

  double prev_draw_t = 0;
  FirstOrderFilter fps_filter;
  const int radius = 192;

  //dp
  void drawKnightScanner(QPainter &p);
};

// container for all onroad widgets
class OnroadWindow : public QWidget {
  Q_OBJECT

public:
  OnroadWindow(QWidget* parent = 0);
  bool isMapVisible() const { return map && map->isVisible(); }
  void showMapPanel(bool show) { if (map) map->setVisible(show); }

signals:
  void mapPanelRequested();

private:
  void paintEvent(QPaintEvent *event);
  void mousePressEvent(QMouseEvent* e) override;
  OnroadAlerts *alerts;
  AnnotatedCameraWidget *nvg;
  QColor bg = bg_colors[STATUS_DISENGAGED];
  QWidget *map = nullptr;
  QHBoxLayout* split;
  bool dp_alka = false;
  // dp indicators
  bool dp_brake_pressed = false;
  bool dp_blinker_left = false;
  bool dp_blinker_right = false;
  bool dp_bsm_left = false;
  bool dp_bsm_right = false;
  bool dp_indicator_left_show = false;
  bool dp_indicator_right_show = false;
  int dp_indicator_left_count = 0;
  int dp_indicator_right_count = 0;
  bool dp_repaint_prev = false;
  void updateIndicatorState(bool blinkerState, bool bsmState, bool& indicatorShow, int& indicatorCount, QColor& indicatorColor);
  QColor dp_indicator_left_color;
  QColor dp_indicator_right_color;
  QColor dp_yellow_color = QColor(0xff, 0xff, 0, 255);
  QColor dp_green_color = QColor(0, 0xff, 0, 255);

private slots:
  void offroadTransition(bool offroad);
  void updateState(const UIState &s);
};
