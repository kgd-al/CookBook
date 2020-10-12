#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "recipe.h"

#include <QDebug>

namespace db {

Recipe Recipe::defaultRecipe (void) {
  return Recipe::fromJson(
    QJsonDocument::fromJson(R"(
      {
        "title": "Sans titre",

        "d-portions": 2,
        "t-portions": "personnes",

        "ing": [],
        "steps": [],
        "notes": ""
      }
    )").object());
}

Recipe::Recipe() {}

QStringList Recipe::ingredientList(double r) const {
  QStringList l;
  l.append("Pour " + QString::number(r * portions) + " " + portionsLabel + ":");
  for (const auto &e: ingredients)
    l.append(e->data(Qt::DisplayRole, r).toString());
  qDebug() << l;
  return l;
}

Recipe Recipe::fromJson(const QJsonValue &j) {
  const QJsonObject jo = j.toObject();
  Recipe r;

  r.id = ID(jo["id"].toInt());

  r.title = jo["title"].toString();

  r.portions = jo["d-portions"].toDouble();
  r.portionsLabel = jo["t-portions"].toString();

  for (const auto &i: jo["ing"].toArray())
    r.ingredients.append(IngredientEntry::fromJson(i));

  for (const auto &s: jo["steps"].toArray())  r.steps.append(s.toString());

  r.notes = jo["notes"].toString();

  return r;
}

QJsonValue Recipe::toJson(const Recipe &r) {
  QJsonObject j;

  j["id"] = r.id;

  j["title"] = r.title;

  j["d-portions"] = r.portions;
  j["t-portions"] = r.portionsLabel;

  QJsonArray ia;
  for (const auto &i: r.ingredients)  ia.append(i->toJson());
  j["ing"] = ia;

  j["steps"] = QJsonArray::fromStringList(r.steps);

  j["notes"] = r.notes;

  return j;
}

} // end of namespace db
