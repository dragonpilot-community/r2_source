#pragma once

#include <string>
#include <vector>

#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>

#include "common/params.h"
#include "selfdrive/ui/qt/widgets/input.h"
#include "selfdrive/ui/qt/widgets/toggle.h"

// dp
#include <QSpinBox>

class ElidedLabel : public QLabel {
  Q_OBJECT

public:
  explicit ElidedLabel(QWidget *parent = 0);
  explicit ElidedLabel(const QString &text, QWidget *parent = 0);

signals:
  void clicked();

protected:
  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent* event) override;
  void mouseReleaseEvent(QMouseEvent *event) override {
    if (rect().contains(event->pos())) {
      emit clicked();
    }
  }
  QString lastText_, elidedText_;
};


class AbstractControl : public QFrame {
  Q_OBJECT

public:
  void setDescription(const QString &desc) {
    if (description) description->setText(desc);
  }

  void setTitle(const QString &title) {
    title_label->setText(title);
  }

  void setValue(const QString &val) {
    value->setText(val);
  }

  const QString getDescription() {
    return description->text();
  }

  QLabel *icon_label;
  QPixmap icon_pixmap;

public slots:
  void showDescription() {
    description->setVisible(true);
  }

signals:
  void showDescriptionEvent();

protected:
  AbstractControl(const QString &title, const QString &desc = "", const QString &icon = "", QWidget *parent = nullptr);
  void hideEvent(QHideEvent *e) override;

  QHBoxLayout *hlayout;
  QPushButton *title_label;

private:
  ElidedLabel *value;
  QLabel *description = nullptr;
};

// widget to display a value
class LabelControl : public AbstractControl {
  Q_OBJECT

public:
  LabelControl(const QString &title, const QString &text = "", const QString &desc = "", QWidget *parent = nullptr) : AbstractControl(title, desc, "", parent) {
    label.setText(text);
    label.setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    hlayout->addWidget(&label);
  }
  void setText(const QString &text) { label.setText(text); }

private:
  ElidedLabel label;
};

// widget for a button with a label
class ButtonControl : public AbstractControl {
  Q_OBJECT

public:
  ButtonControl(const QString &title, const QString &text, const QString &desc = "", QWidget *parent = nullptr);
  inline void setText(const QString &text) { btn.setText(text); }
  inline QString text() const { return btn.text(); }

signals:
  void clicked();

public slots:
  void setEnabled(bool enabled) { btn.setEnabled(enabled); }

private:
  QPushButton btn;
};

class ToggleControl : public AbstractControl {
  Q_OBJECT

public:
  ToggleControl(const QString &title, const QString &desc = "", const QString &icon = "", const bool state = false, QWidget *parent = nullptr) : AbstractControl(title, desc, icon, parent) {
    toggle.setFixedSize(150, 100);
    if (state) {
      toggle.togglePosition();
    }
    hlayout->addWidget(&toggle);
    QObject::connect(&toggle, &Toggle::stateChanged, this, &ToggleControl::toggleFlipped);
  }

  void setEnabled(bool enabled) {
    toggle.setEnabled(enabled);
    toggle.update();
  }

signals:
  void toggleFlipped(bool state);

protected:
  Toggle toggle;
};

// widget to toggle params
class ParamControl : public ToggleControl {
  Q_OBJECT

public:
  ParamControl(const QString &param, const QString &title, const QString &desc, const QString &icon, QWidget *parent = nullptr) : ToggleControl(title, desc, icon, false, parent) {
    key = param.toStdString();
    QObject::connect(this, &ParamControl::toggleFlipped, [=](bool state) {
      QString content("<body><h2 style=\"text-align: center;\">" + title + "</h2><br>"
                      "<p style=\"text-align: center; margin: 0 128px; font-size: 36px;\">" + getDescription() + "</p></body>");
      ConfirmationDialog dialog(content, tr("Enable"), tr("Cancel"), true, this);

      bool confirmed = store_confirm && params.getBool(key + "Confirmed");
      if (!confirm || confirmed || !state || dialog.exec()) {
        if (store_confirm && state) params.putBool(key + "Confirmed", true);
        params.putBool(key, state);
        setIcon(state);
      } else {
        toggle.togglePosition();
      }
    });
  }

  void setConfirmation(bool _confirm, bool _store_confirm) {
    confirm = _confirm;
    store_confirm = _store_confirm;
  }

  void setActiveIcon(const QString &icon) {
    active_icon_pixmap = QPixmap(icon).scaledToWidth(80, Qt::SmoothTransformation);
  }

  void refresh() {
    bool state = params.getBool(key);
    if (state != toggle.on) {
      toggle.togglePosition();
      setIcon(state);
    }
  }

  void showEvent(QShowEvent *event) override {
    refresh();
  }

private:
  void setIcon(bool state) {
    if (state && !active_icon_pixmap.isNull()) {
      icon_label->setPixmap(active_icon_pixmap);
    } else if (!icon_pixmap.isNull()) {
      icon_label->setPixmap(icon_pixmap);
    }
  }

  std::string key;
  Params params;
  QPixmap active_icon_pixmap;
  bool confirm = false;
  bool store_confirm = false;
};

class ButtonParamControl : public AbstractControl {
  Q_OBJECT
public:
  ButtonParamControl(const QString &param, const QString &title, const QString &desc, const QString &icon,
                     const std::vector<QString> &button_texts, const int minimum_button_width = 150) : AbstractControl(title, desc, icon) {
    const QString style = R"(
      QPushButton {
        border-radius: 0px;
        font-size: 32px;
        font-weight: 500;
        height:80px;
        padding: 0 10 0 10;
        color: #E4E4E4;
        background-color: #393939;
      }
      QPushButton:pressed {
        background-color: #4a4a4a;
      }
      QPushButton:checked:enabled {
        background-color: #33Ab4C;
      }
      QPushButton:disabled {
        color: #33E4E4E4;
      }
    )";
    key = param.toStdString();
    int value = atoi(params.get(key).c_str());

    button_group = new QButtonGroup(this);
    button_group->setExclusive(true);
    for (int i = 0; i < button_texts.size(); i++) {
      QPushButton *button = new QPushButton(button_texts[i], this);
      button->setCheckable(true);
      button->setChecked(i == value);
      button->setStyleSheet(style);
      button->setMinimumWidth(minimum_button_width);
      hlayout->addWidget(button);
      button_group->addButton(button, i);
    }

    QObject::connect(button_group, QOverload<int, bool>::of(&QButtonGroup::buttonToggled), [=](int id, bool checked) {
      if (checked) {
        params.put(key, std::to_string(id));
      }
    });
  }

  void setEnabled(bool enable) {
    for (auto btn : button_group->buttons()) {
      btn->setEnabled(enable);
    }
  }

  // Public method to manually refresh the control
  void refreshControl() {
    int value = atoi(params.get(key).c_str());

    // Loop through buttons and update their checked state based on the current value
    for (int i = 0; i < button_group->buttons().size(); i++) {
      QPushButton *button = dynamic_cast<QPushButton*>(button_group->button(i));
      if (button) {
        button->setChecked(i == value);
      }
    }
  }

private:
  std::string key;
  Params params;
  QButtonGroup *button_group;
};

class ListWidget : public QWidget {
  Q_OBJECT
 public:
  explicit ListWidget(QWidget *parent = 0) : QWidget(parent), outer_layout(this) {
    outer_layout.setMargin(0);
    outer_layout.setSpacing(0);
    outer_layout.addLayout(&inner_layout);
    inner_layout.setMargin(0);
    inner_layout.setSpacing(20); // default spacing is 25
    outer_layout.addStretch();
  }
  inline void addItem(QWidget *w) { inner_layout.addWidget(w); }
  inline void addItem(QLayout *layout) { inner_layout.addLayout(layout); }
  inline void setSpacing(int spacing) { inner_layout.setSpacing(spacing); }

private:
  void paintEvent(QPaintEvent *) override {
    QPainter p(this);
    p.setPen(Qt::gray);
    for (int i = 0; i < inner_layout.count() - 1; ++i) {
      QWidget *widget = inner_layout.itemAt(i)->widget();
      if (widget == nullptr || widget->isVisible()) {
        QRect r = inner_layout.itemAt(i)->geometry();
        int bottom = r.bottom() + inner_layout.spacing() / 2;
        p.drawLine(r.left() + 40, bottom, r.right() - 40, bottom);
      }
    }
  }
  QVBoxLayout outer_layout;
  QVBoxLayout inner_layout;
};

// convenience class for wrapping layouts
class LayoutWidget : public QWidget {
  Q_OBJECT

public:
  LayoutWidget(QLayout *l, QWidget *parent = nullptr) : QWidget(parent) {
    setLayout(l);
  }
};

// dp
class SpinBoxControl : public AbstractControl {
  Q_OBJECT

public:
  SpinBoxControl(const QString &title, const QString &desc = "", const QString &icon = "", const int val = 0, QWidget *parent = nullptr) : AbstractControl(title, desc, icon, parent) {
    spinbox.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    spinbox.setAlignment(Qt::AlignCenter);
    spinbox.setStyleSheet(R"(
      QSpinBox {
        width: 300px;
        height: 80px;
        padding: 0px;
        border-radius:0px;
        font-size: 36px;
      }
      QSpinBox::up-button  {
        subcontrol-origin: margin;
        subcontrol-position: center right;
        image: url(../assets/offroad/icon_plus.png);
        right: 1px;
        height: 80px;
        width: 80px;
      }
      QSpinBox::down-button  {
        subcontrol-origin: margin;
        subcontrol-position: center left;
        image: url(../assets/offroad/icon_minus.png);
        left: 1px;
        height: 80px;
        width: 80px;
      }
    )");

    hlayout->addWidget(&spinbox);
    QObject::connect(&spinbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SpinBoxControl::valChanged);
  }

signals:
  void valChanged(int val);

protected:
  QSpinBox spinbox;
};

class ParamSpinBoxControl : public SpinBoxControl {
  Q_OBJECT

public:
  ParamSpinBoxControl(const QString &param, const QString &title, const QString &desc, const QString &icon, const int minimum = 0, const int maximum = 100, const int step = 1, const QString suffix = "", const QString minText = "", QWidget *parent = nullptr) : SpinBoxControl(title, desc, icon, 0, parent) {
    spinbox.setRange(minimum, maximum);
    spinbox.setSingleStep(step);
    spinbox.setSuffix(suffix);
    spinbox.setSpecialValueText(minText);

    std::string str_val = Params().get(param.toStdString().c_str());
    if (str_val != "") {
      int val = std::stoi(str_val);
      spinbox.setValue(val);
      QObject::connect(this, &SpinBoxControl::valChanged, [=](int val) {
        Params().put(param.toStdString(), std::to_string(val));
      });
    }
  }
};
