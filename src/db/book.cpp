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

  QJsonObject json;

  json["recipes"] = recipes.toJson();

//  /// TODO Remove
//  IngredientsModel ingredients2 (false);
//  ingredients2.fromScratch(recipes);

  json["ingredients"] = ingredients.toJson();
//  json["ingredients-rebuilt"] = ingredients2.toJson();

#ifdef FAKE_SAVE
  qDebug() << "Not saving! Displaying below instead:\n"
           << QJsonDocument(json).toJson(QJsonDocument::Indented)
                                 .toStdString().c_str();
#else
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
}

const QString& Book::title (int id) const {
  static const QString error = "Recipe not found";
  auto it = recipes.find(id);
  if (it != recipes.end())
    return it->second.title;
  else
    return error;
}

} // end of namespace db
