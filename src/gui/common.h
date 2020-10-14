#ifndef COMMON_HPP
#define COMMON_HPP

#include <QSettings>
#include <QDebug>

namespace gui {

template <typename T>
QSettings& localSettings (const T *qobject) {
  static QSettings settings;
  while (!settings.group().isEmpty()) settings.endGroup();
  settings.beginGroup(qobject->metaObject()->className());
  return settings;
}

bool saveGeometry (QWidget *w);
bool saveGeometry (QWidget *w, QSettings &s);

bool restoreGeometry (QWidget *w);
bool restoreGeometry (QWidget *w, QSettings &s);

} // end of namespace gui

#endif // COMMON_HPP
