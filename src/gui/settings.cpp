#include <QMetaEnum>
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QDialogButtonBox>

#include <QApplication>
#include <QFontDialog>

#include "settings.h"
#include "common.h"

namespace gui {

struct SettingData {
  QString displayName;
  QVariant defaultValue;
};

SettingData settingData (Settings::Type type) {
  static QMap<Settings::Type, SettingData> sdata {
    {           Settings::AUTOSAVE, { "Sauvegarde automatique", false   } },
    { Settings::TIGHT_RECIPE_ICONS, { "Icônes collés",          true    } },
    {               Settings::FONT, { "Police",                 QFont() } },

    { Settings::MODAL_IMANAGER, { "Gestion d'ingrédients", true } },
    {  Settings::MODAL_REPAIRS, { "Réparation",            true } },
    { Settings::MODAL_SETTINGS, { "Configuration",         true } },

  };
  return sdata.value(type);
}

QString description (const QFont &font) {
  return font.family()
      + " " + font.style()
      + " " + QString::number(font.pointSizeF());
}

QWidget* factory (Settings *settings, Settings::Type stype) {
  SettingData data = settingData(stype);
  QVariant::Type dtype = data.defaultValue.type();
  if (QVariant::Bool == dtype) {
    auto cb = new QCheckBox (data.displayName);
    cb->setChecked(Settings::value<bool>(stype));
    Settings::connect(cb, &QCheckBox::clicked, [settings, stype, cb] {
      settings->settingChanged(stype, cb->isChecked());
    });
    return cb;

  } else if (QVariant::Font == dtype) {
    QWidget *holder = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout;
    QLabel *label = new QLabel ("Police: ");
    QLabel *flabel = new QLabel (description(Settings::value<QFont>(stype)));
    QToolButton *button = new QToolButton();
    layout->addWidget(label);
    layout->addWidget(flabel);
    layout->addWidget(button);
    holder->setLayout(layout);

    button->setText("...");
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    Settings::connect(button, &QToolButton::clicked,
                      [settings, holder, flabel, stype] {
      bool ok;
      QFont font = QApplication::font();
      font = QFontDialog::getFont(&ok, font, holder);
      if (ok) {
        QApplication::setFont(font);
//        db::fontChanged(font);
        settings->settingChanged(stype, font);
        flabel->setText(description(font));
      }
    });

    return holder;

  } else
    return nullptr;
}

void addSection (QVBoxLayout *layout, const char *label) {
  layout->addSpacing(10);
  layout->addWidget(new QLabel(Settings::tr(label) + ":"));
}

Settings::Settings(QWidget *parent) : QDialog(parent) {
  QVBoxLayout *layout = new QVBoxLayout;
  auto meta = QMetaEnum::fromType<Type>();
  for (int i=0; i<meta.keyCount(); i++) {
    Type t = (Type)meta.value(i);

    if (t == AUTOSAVE) {
      addSection(layout, "Générique");

    } else if (t == MODAL_IMANAGER) {
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

QVariant Settings::value (Type t) {
//  return settings().value(QMetaEnum::fromType<Type>().key(t));
  QSettings settings;
  settings.beginGroup("global-settings");
  auto v = settings.value(QMetaEnum::fromType<Type>().key(t),
                          settingData(t).defaultValue);
  qDebug().nospace() << "Settings[" << t << "] = " << v;
  return v;
}

void Settings::settingChanged(Type t, const QVariant &newValue) {
  qDebug() << "Setting" << t << "changed from" << value(t) << "to" << newValue;
  QSettings settings;
  settings.beginGroup("global-settings");
#ifndef QT_NO_DEBUG
  Q_ASSERT(newValue.type() == settingData(t).defaultValue.type());
  auto prevValue = value(t);
  Q_ASSERT(prevValue.canConvert(newValue.type()));
#endif
  settings.setValue(QMetaEnum::fromType<Type>().key(t), newValue);
}

} // end of namespace gui
