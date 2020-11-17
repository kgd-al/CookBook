#ifndef SETTINGS_H
#define SETTINGS_H

#include <QVariant>

namespace db {

class Settings : public QObject {
  Q_OBJECT
public:
  enum Type {
    AUTOSAVE,
    TIGHT_RECIPE_ICONS,
    FONT,

    MODAL_IMANAGER, MODAL_REPAIRS, MODAL_SETTINGS,
  };
  Q_ENUM(Type)

  struct Data {
    QString displayName;
    QVariant defaultValue;
  };

  static QVariant value (Type t);

  template <typename T>
  static T value (Type t) {
    return value(t).value<T>();
  }

  static const Data& data (Type t);

  static void settingChanged (Type t, const QVariant &newValue);
};

} // end of namespace db

#endif // SETTINGS_H
