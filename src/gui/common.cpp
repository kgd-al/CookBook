#include <QWidget>

#include "common.h"

namespace gui {

bool saveGeometry(QWidget *w, QSettings &s) {
  auto g = w->saveGeometry();
  if (g.isNull()) return false;
  s.setValue("geometry", g);
  return true;
}

bool restoreGeometry (QWidget *w, QSettings &s) {
  auto g = s.value("geometry").toByteArray();
  if (!g.isNull())  return w->restoreGeometry(g);
  return false;
}

bool saveGeometry (QWidget *w) {
  return saveGeometry(w, localSettings(w));
}

bool restoreGeometry (QWidget *w) {
  return restoreGeometry(w, localSettings(w));
}

} // end of namespace gui
