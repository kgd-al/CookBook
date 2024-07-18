#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>

#include <QFile>

#include "book.h"
#include "recipesmodel.h"
#include "settings.h"

#include <QDebug>

namespace db {

Book::Book(void) : _modified(false) {
  for (QAbstractTableModel *m: std::initializer_list<QAbstractTableModel*>{
                                  &recipes, &ingredients, &units, &planning})
    connect(m, &QAbstractItemModel::dataChanged,
            this, QOverload<>::of(&Book::setModified));
}

void Book::setModified(bool m) {
  _modified = m;
  emit modified(_modified);
}

Book& Book::current (void) {
  static Book b;
  return b;
}

QModelIndex Book::addRecipe(Recipe &&r) {
  return recipes.addRecipe(std::move(r));
}

bool Book::close (QWidget *widget) {
#ifndef Q_OS_ANDROID
  if (!_modified) return true;
  auto ret = QMessageBox::warning(widget, "Confirmez",
                                  "Sauvegarder les changements?",
                                  QMessageBox::Yes, QMessageBox::No,
                                  QMessageBox::Cancel);
  switch (ret) {
  case QMessageBox::Yes:
    autosave(false);
    return true;
  case QMessageBox::No:
    return true;
  case QMessageBox::Cancel:
  default:
    return false;
  }
#else
  (void)widget;
  return true;
#endif
}

#ifndef Q_OS_ANDROID
bool Book::autosave(bool spontaneous) {
  // Nothing to save
  if (!_modified) return true;
  if (spontaneous && !Settings::value<bool>(Settings::AUTOSAVE))  return false;
  return save();
}

bool Book::save(void) {
  QJsonObject json;
  json["planning"] = planning.toJson();
  json["recipes"] = recipes.toJson();
  json["ingredients"] = ingredients.toJson();
  json["units"] = units.toJson();

  int eindex = monitoredPath().lastIndexOf('.');
  QString backup = monitoredPath().mid(0, eindex);
  backup += ".backup";
  if (eindex >= 0)  backup += monitoredPath().mid(eindex);
  qDebug() << backup;

  QFile (backup).remove();
  QFile::copy(monitoredPath(), backup);

  QFile saveFile (monitoredPath());

  if (!saveFile.open(QIODevice::WriteOnly)) {
    qWarning("Failed to save to file '%s'",
             monitoredPath().toStdString().c_str());
    return false;
  }

  qDebug() << "Saving:\n"
           << QJsonDocument(json).toJson(QJsonDocument::Indented)
                                 .toStdString().c_str();
  saveFile.write(QJsonDocument(json).toJson(QJsonDocument::Indented));

  setModified(false);
  return true;
}
#endif

bool Book::load (void) {
  QFile loadFile (monitoredPath());

  if (!loadFile.open(QIODevice::ReadOnly)) {
    qWarning("Failed to open file '%s'", monitoredPath().toStdString().c_str());
    return false;
  }

  QByteArray data = loadFile.readAll();
  QJsonParseError err;
  QJsonDocument json_doc = QJsonDocument::fromJson(data, &err);

  if (json_doc.isNull()) {
    qWarning("Failed to parse json file '%s': %s",
             monitoredPath().toStdString().c_str(),
             err.errorString().toStdString().c_str());
    return false;
  }

  QJsonObject json = json_doc.object();
  units.fromJson(json["units"].toArray());
  ingredients.fromJson(json["ingredients"].toArray());
  recipes.fromJson(json["recipes"].toArray());
  planning.fromJson(json["planning"].toArray());


  qInfo("Loaded and parsed database from '%s'",
        monitoredPath().toStdString().c_str());

  setModified(false);
  return true;
}

QString Book::extension(void) {
  return "rbk";
}

QString Book::monitoredName(void) {
  return "cookbook." + extension();
}

QString Book::monitoredDir(void) {
#ifndef Q_OS_ANDROID
  return QString(BASE_DIR) + "/data/";
#else
  return "/sdcard/";
#endif
}

QString Book::monitoredPath(void) {
  return monitoredDir() + monitoredName();
}

} // end of namespace db
