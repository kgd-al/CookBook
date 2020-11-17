#include <QMetaEnum>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QDialogButtonBox>

#include <QApplication>
#include <QFontDialog>

#include "gui_settings.h"
#include "common.h"
#include "../db/settings.h"

namespace gui {

QString description (const QFont &font) {
  return font.family()
      + " " + font.style()
      + " " + QString::number(font.pointSizeF());
}

QWidget* factory (SettingsView *settings, db::Settings::Type stype) {
  auto data = db::Settings::data(stype);
  QVariant::Type dtype = data.defaultValue.type();
  if (QVariant::Bool == dtype) {
    auto cb = new QCheckBox (data.displayName);
    cb->setChecked(db::Settings::value<bool>(stype));
    SettingsView::connect(cb, &QCheckBox::clicked, [settings, stype, cb] {
      db::Settings::settingChanged(stype, cb->isChecked());
    });
    return cb;

  } else if (QVariant::Font == dtype) {
    QWidget *holder = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout;
    QLabel *label = new QLabel ("Police: ");
    QLabel *flabel = new QLabel (description(db::Settings::value<QFont>(stype)));
    QToolButton *button = new QToolButton();
    layout->addWidget(label);
    layout->addWidget(flabel);
    layout->addWidget(button);
    holder->setLayout(layout);

    button->setText("...");
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    SettingsView::connect(button, &QToolButton::clicked,
                      [settings, holder, flabel, stype] {
      bool ok;
      QFont font = QApplication::font();
      font = QFontDialog::getFont(&ok, font, holder);
      if (ok) {
        QApplication::setFont(font);
        db::Settings::settingChanged(stype, font);
        flabel->setText(description(font));
      }
    });

    return holder;

  } else
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

    layout->addWidget(factory(this, t));
  }
  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::close);
  layout->addWidget(buttons);
  setLayout(layout);

  setWindowTitle("Configuration");
}

} // end of namespace gui
