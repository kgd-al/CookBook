#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <QSettings>
#include <QVariant>

namespace gui {

class Settings : public QDialog {
  Q_OBJECT
public:
  enum Type {
    AUTOSAVE,
    TIGHT_RECIPE_ICONS,
    FONT,

    MODAL_IMANAGER, MODAL_REPAIRS, MODAL_SETTINGS,
  };
  Q_ENUM(Type)

  Settings(QWidget *parent);

  static QVariant value (Type t);

  template <typename T>
  static T value (Type t) {
    return value(t).value<T>();
  }

  void settingChanged (Type t, const QVariant &newValue);
};

} // end of namespace gui

#endif // SETTINGS_H
