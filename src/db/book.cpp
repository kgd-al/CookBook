#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "book.h"
#include "recipeslistmodel.h"

#include <QDebug>

namespace db {

Book::Book(void) : modified(false) {}

Book& Book::current (void) {
  static Book b;
  return b;
}

void Book::addRecipe(Recipe &&r) {
  recipes.addRecipe(std::move(r));
}

//#define FAKE_SAVE 1
bool Book::save(const QString &path) {
  QJsonObject json;
  json["recipes"] = recipes.toJson();
  json["ingredients"] = ingredients.toJson();

  int eindex = path.lastIndexOf('.');
  QString backup = path.mid(0, eindex);
  backup += ".backup";
  if (eindex >= 0)  backup += path.mid(eindex);
  qDebug() << backup;

  QFile (backup).remove();
  QFile::copy(path, backup);

  QFile saveFile (path);

#ifndef FAKE_SAVE
  if (!saveFile.open(QIODevice::WriteOnly)) {
    qWarning("Failed to save to file '%s'", path.toStdString().c_str());
    return false;
  }
#endif

#ifdef FAKE_SAVE
  qDebug() << "Not saving! Displaying below instead:\n"
           << QJsonDocument(json).toJson(QJsonDocument::Indented)
                                 .toStdString().c_str();
#else
  qDebug() << "Saving:\n"
           << QJsonDocument(json).toJson(QJsonDocument::Indented)
                                 .toStdString().c_str();
  saveFile.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
#endif

  modified = false;
  this->path = path;
  return true;
}

bool Book::load (const QString &path) {
  QFile loadFile (path);

  if (!loadFile.open(QIODevice::ReadOnly)) {
    qWarning("Failed to open file '%s'", path.toStdString().c_str());
    return false;
  }

  QByteArray data = loadFile.readAll();
  QJsonParseError err;
  QJsonDocument json_doc = QJsonDocument::fromJson(data, &err);

  if (json_doc.isNull()) {
    qWarning("Failed to parse json file '%s': %s",
             path.toStdString().c_str(),
             err.errorString().toStdString().c_str());
    return false;
  }

//  qDebug() << "Loading:\n"
//           << json_doc.toJson(QJsonDocument::Indented)
//                      .toStdString().c_str();

  clear();
  QJsonObject json = json_doc.object();
  ingredients.fromJson(json["ingredients"].toArray());
  recipes.fromJson(json["recipes"].toArray());


  qInfo("Loaded and parsed database from '%s'",
        path.toStdString().c_str());

  modified = false;
  this->path = path;
  return true;
}

void Book::clear (void) {
  recipes.clear();
  ingredients.clear();
  modified = false;
  path = "";
}

const QString& Book::title (Recipe::ID id) const {
  static const QString error = "Recipe not found";
  auto it = recipes.find(id);
  if (it != recipes.end())
    return it->title;
  else
    return error;
}

} // end of namespace db
