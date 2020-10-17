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

// On update
void Recipe::updateUsageCounts (const IngredientList &newList) {

  QMap<db::IngredientData*, int> i_counts;
  QMap<db::Recipe*, int> r_counts;
  const auto process =
    [&i_counts, &r_counts] (const auto &in, int sign) {
    for (auto &i: in) {
      switch (i->etype) {
      case db::EntryType::Ingredient:
        i_counts[static_cast<db::IngredientEntry*>(i.data())->idata] += sign;
        break;
      case db::EntryType::SubRecipe:
        r_counts[static_cast<db::SubRecipeEntry*>(i.data())->recipe] += sign;
        break;
      default:  break;
      }
    }
  };
  process(ingredients, -1);
  process(newList,  +1);

  QDebug q = qDebug().nospace();
  q << "Processing differing usage counts\n";
  q << "Ingredients:\n";
  q << i_counts << "\n";
  q << "Subrecipes:\n";
  q << r_counts << "\n";

  for (auto it = i_counts.begin(); it != i_counts.end(); ++it)
    if (it.value() != 0)
      it.key()->used += it.value();

  for (auto it = r_counts.begin(); it != r_counts.end(); ++it)
    if (it.value() != 0)
      it.key()->used += it.value();
}

// On deletion
void Recipe::updateUsageCounts(void) {
  for (auto &i: ingredients) {
    switch (i->etype) {
    case db::EntryType::Ingredient:
      static_cast<db::IngredientEntry*>(i.data())->idata->used--;
      break;
    case db::EntryType::SubRecipe:
      static_cast<db::SubRecipeEntry*>(i.data())->recipe->used--;
      break;
    default:  break;
    }
  }
}

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
  r.used = jo["used"].toInt();

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
  j["used"] = r.used;

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
