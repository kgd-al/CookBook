#include <QGuiApplication>
#include <QScreen>

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

void save (QSettings &settings, const QString &key, const QSplitter *s) {
  settings.beginGroup(key);
  QVariantList sizes, visibles;
  for (int s: s->sizes()) sizes.append(s);
  settings.setValue("sizes", sizes);
  for (int i=0; i<s->count(); i++) visibles.append(s->widget(i)->isVisible());
  settings.setValue("visible", visibles);
  qDebug() << "Splitter" << key << "of size" << s->sizes() << "and visibilities"
           << visibles;
  settings.endGroup();
}

void restore (QSettings &settings, const QString &key, QSplitter *s) {
//  static const auto mSize = QGuiApplication::primaryScreen()->size();
//  static const auto M = std::max(mSize.width(), mSize.height());
//  static const auto defS = QVariant::fromValue(QList<int>{M,1});
  static const auto defV = QVariant::fromValue(QList<bool>{true,true});
  settings.beginGroup(key);

  auto vsizes = settings.value("sizes").toList();
  QList<int> sizes;
  int total = 0;
  for (const QVariant &v: vsizes) {
    sizes.append(v.toInt());
    total += sizes.back();
  }
  if (sizes.empty())
    sizes.append(1);
  else if (total == 0)
    sizes.first() = 1;
  s->setSizes(sizes);

  QVariantList visibles = settings.value("visible", defV).toList();
  for (int i=0; i<visibles.size(); i++)
    s->widget(i)->setVisible(visibles[i].toBool());

  qDebug() << "Splitter" << key << "of size" << s->sizes() << "and visibilities"
           << visibles;
  settings.endGroup();
}

} // end of namespace gui
