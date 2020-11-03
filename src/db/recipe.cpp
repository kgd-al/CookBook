#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "recipe.h"
#include "book.h"

#include <QDebug>

namespace db {

Recipe::Recipe (void) {
  used = 0;

  title = "Poudre de pinrlinpinpin";

  portions = 0;
  portionsLabel = "";

  basic = false;

  regimen = &first<RegimenData>();
  type = &first<DishTypeData>();
  duration = &first<DurationData>();
  status = &first<StatusData>();

//  ingredients;
//  steps;
//  notes;
}

// On update
void Recipe::updateUsageCounts (const IngredientList &newList) {

  QMap<db::UnitData*, int> u_counts;
  QMap<db::IngredientData*, int> i_counts;
  QMap<db::Recipe*, int> r_counts;
  const auto process =
    [&u_counts, &i_counts, &r_counts] (const IngredientList &in, int sign) {
    for (auto &i: in) {
      switch (i->etype) {
      case db::EntryType::Ingredient: {
        auto entry = static_cast<db::IngredientEntry*>(i.data());
        i_counts[entry->idata] += sign;
        u_counts[entry->unit] += sign;
        break;
      }
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
  if (!u_counts.empty()) q << "Units:\n";
  for (auto key: u_counts.keys())
    q << "\t" << key->text << ": " << u_counts.value(key) << "\n";
  if (!i_counts.empty()) q << "Ingredients:\n";
  for (auto key: i_counts.keys())
    q << "\t" << key->text << ": " << i_counts.value(key) << "\n";
  if (!r_counts.empty()) q << "Subrecipes:\n";
  for (auto key: r_counts.keys())
    q << "\t" << key->title << ": " << r_counts.value(key) << "\n";

  auto &book = Book::current();
  auto &umodel = book.units;
  for (auto it = u_counts.begin(); it != u_counts.end(); ++it) {
    if (it.value() != 0) {
      it.key()->used += it.value();
      umodel.valueModified(it.key()->id);
    }
  }

  auto &imodel = Book::current().ingredients;
  for (auto it = i_counts.begin(); it != i_counts.end(); ++it) {
    if (it.value() != 0) {
      it.key()->used += it.value();
      imodel.valueModified(it.key()->id);
    }
  }

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
      static_cast<db::IngredientEntry*>(i.data())->unit->used--;
      break;
    case db::EntryType::SubRecipe:
      static_cast<db::SubRecipeEntry*>(i.data())->recipe->used--;
      break;
    default:  break;
    }
  }
}

QIcon Recipe::basicIcon(void) const {
  return basic ? MiscIcons::basic_recipe() : QIcon();
}

QIcon Recipe::subrecipeIcon(void) const {
  return used > 0 ? MiscIcons::sub_recipe() : QIcon();
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

  r.regimen = &at<RegimenData>(db::ID(jo["regimen"].toInt()));
  r.status = &at<StatusData>(db::ID(jo["status"].toInt()));
  r.type = &at<DishTypeData>(db::ID(jo["type"].toInt()));
  r.duration = &at<DurationData>(db::ID(jo["duration"].toInt()));
  r.basic = jo["basic"].toBool();

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

  j["regimen"] = r.regimen->id;
  j["status"] = r.status->id;
  j["type"] = r.type->id;
  j["duration"] = r.duration->id;
  j["basic"] = r.basic;

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
