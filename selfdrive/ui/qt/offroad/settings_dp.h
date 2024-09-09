#pragma once

//#include <QButtonGroup>
//#include <QFileSystemWatcher>
//#include <QFrame>
//#include <QLabel>
//#include <QPushButton>
//#include <QStackedWidget>
//#include <QWidget>


#include "selfdrive/ui/qt/widgets/controls.h"

class DPCtrlPanel : public ListWidget {
  Q_OBJECT
public:
  explicit DPCtrlPanel(QWidget *parent);
  void showEvent(QShowEvent *event) override;

public slots:
  void expandToggleDescription(const QString &param);

private:
  Params params;
  std::map<std::string, ParamControl*> toggles;
  ParamSpinBoxControl* auto_shutdown_timer_toggle;
  ParamSpinBoxControl* speed_based_lane_priority_toggle;

  void add_overall_toggles();
  void add_lateral_toggles();
  void add_longitudinal_toggles();
  void add_device_toggles();
  void add_misc_toggles();
  void add_car_specific_toggles();
  void add_toyota_toggles();
  void add_hkg_toggles();
  void add_vag_toggles();

  void add_generic_toggles(std::vector<std::tuple<QString, QString, QString>> &toggle_defs);

  void updateToggles();
};
