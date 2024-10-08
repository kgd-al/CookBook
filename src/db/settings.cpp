#include <QSettings>
#include <QMetaEnum>
#include <QFont>

#include <QDebug>

#include "settings.h"

namespace db {

const Settings::Data& Settings::data (Settings::Type type) {
  using namespace db;
  static std::map<Settings::Type, Settings::Data> sdata {
    {           Settings::AUTOSAVE, { "Sauvegarde automatique", false   } },
    { Settings::TIGHT_RECIPE_ICONS, { "Icônes collés",          true    } },
    {               Settings::FONT, { "Police",                 QFont() } },

    { Settings::MODAL_IMANAGER, { "Gestion d'ingrédients", true } },
    {  Settings::MODAL_REPAIRS, { "Réparation",            true } },
    { Settings::MODAL_SETTINGS, { "Configuration",         true } },

    { Settings::PLANNING_WINDOW, { "Planning", 7 } },
  };

  return sdata.at(type);
}

QVariant Settings::value (Type t) {
//  return settings().value(QMetaEnum::fromType<Type>().key(t));
  QSettings settings;
  settings.beginGroup("global-settings");
  auto v = settings.value(QMetaEnum::fromType<Type>().key(t),
                          data(t).defaultValue);
  qDebug().nospace() << "Settings[" << t << "] = " << v;
  return v;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
auto qvType(const QVariant &v) { return v.type(); }
bool canConvert(const QVariant &from, const QVariant &to) {
  return from.canConvert(to.type());
}
#else
auto qvType(const QVariant &v) { return v.typeId(); }
bool canConvert(const QVariant &from, const QVariant &to) {
  return from.canConvert(to.metaType());
}
#endif

void Settings::updateSetting(Type t, const QVariant &newValue) {
  qDebug() << "Setting" << t << "changed from" << value(t) << "to" << newValue;
  QSettings settings;
  settings.beginGroup("global-settings");
#ifndef QT_NO_DEBUG
  Q_ASSERT(qvType(newValue) == qvType(data(t).defaultValue));
  auto prevValue = value(t);
  Q_ASSERT(canConvert(prevValue, newValue));
#endif
  settings.setValue(QMetaEnum::fromType<Type>().key(t), newValue);
  emit instance()->settingChanged(t, newValue);
}

const Settings* Settings::instance (void) {
  static Settings singleton;
  return &singleton;
}

} // end of namespace db
