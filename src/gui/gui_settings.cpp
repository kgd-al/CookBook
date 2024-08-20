#include <QMetaEnum>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QDialogButtonBox>
#include <QSpinBox>

#include <QApplication>
#include <QFontDialog>

#include "gui_settings.h"
#include "../db/settings.h"

namespace gui {

QWidget* factory (db::Settings::Type stype) {
  auto data = db::Settings::data(stype);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  auto dtype = data.defaultValue.type();
  const auto Bool = QVariant::Bool;
  const auto Font = QVariant::Font;
  const auto Int = QVariant::Int;
#else
  auto dtype = data.defaultValue.typeId();
  const auto Bool = QMetaType::Bool;
  const auto Font = QMetaType::QFont;
  const auto Int = QMetaType::Int;
#endif

  if (Bool == dtype) {
    auto cb = new QCheckBox (data.displayName);
    cb->setChecked(db::Settings::value<bool>(stype));
    SettingsView::connect(cb, &QCheckBox::clicked, [stype, cb] {
      db::Settings::updateSetting(stype, cb->isChecked());
    });
    return cb;

  } else if (Font == dtype) {
    QWidget *holder = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout;
    QLabel *label = new QLabel ("Police: ");
    QLabel *flabel = new QLabel (db::Settings::value<QFont>(stype).toString());
    QToolButton *button = new QToolButton();
    layout->addWidget(label);
    layout->addWidget(flabel);
    layout->addWidget(button);
    holder->setLayout(layout);

    button->setText("...");
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    SettingsView::connect(button, &QToolButton::clicked,
                          [holder, flabel, stype] {
      bool ok;
      QFont font = QApplication::font();
      font = QFontDialog::getFont(&ok, font, holder);
      if (ok) {
        QApplication::setFont(font);
        db::Settings::updateSetting(stype, font);
        flabel->setText(font.toString());
      }
    });

    return holder;

  } else if (Int == dtype) {
    QWidget *holder = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(new QLabel(data.displayName));
    auto sb = new QSpinBox;
    sb->setValue(db::Settings::value<int>(stype));
    SettingsView::connect(sb, QOverload<int>::of(&QSpinBox::valueChanged),
                          [stype] (int value) {
      db::Settings::updateSetting(stype, value);
    });
    layout->addWidget(sb);
    holder->setLayout(layout);
    return holder;

  }

  return nullptr;
}

void addSection (QVBoxLayout *layout, const char *label) {
  layout->addSpacing(10);
  layout->addWidget(new QLabel(SettingsView::tr(label) + ":"));
}

SettingsView::SettingsView(QWidget *parent) : QDialog(parent) {
  using Type = db::Settings::Type;
  QVBoxLayout *layout = new QVBoxLayout;
  auto meta = QMetaEnum::fromType<Type>();
  for (int i=0; i<meta.keyCount(); i++) {
    Type t = (Type)meta.value(i);

    if (t == Type::AUTOSAVE) {
      addSection(layout, "Générique");

    } else if (t == Type::MODAL_IMANAGER) {
      addSection(layout, "Fenêtres bloquantes");
    }

    layout->addWidget(factory(t));
  }
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
  layout->addWidget(buttons);
  setLayout(layout);

  setWindowTitle("Configuration");
}

} // end of namespace gui
