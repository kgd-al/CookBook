#include <QJsonArray>

#include "ingredientdata.h"
#include "unitsmodel.h"
#include "book.h"

namespace db {

const AlimentaryGroupsDatabase AlimentaryGroupData::database {
  { ID(0), "Protéines", QColor::fromRgb(255,   0,   0) },
  { ID(1),   "Verdure", QColor::fromRgb(  0, 255,   0) },
  { ID(2),  "Céréales", QColor::fromRgb(255, 165,   0) },
  { ID(3),    "Sucres", QColor::fromRgb(255, 105, 180) },
  { ID(4),    "Épices", QColor::fromRgb(  0, 128,   0) },
  { ID(5),  "Liquides", QColor::fromRgb(  0,   0, 190) },
  { ID(6),   "Graines", QColor::fromRgb(160,  82,  42) },
  { ID(7),  "Laitiers", QColor::fromRgb(255, 248, 220) },
  { ID(8),   "Lipides", QColor::fromRgb(255, 215,   0) },
  { ID(9),  "Alcools", QColor::fromRgb( 64, 224, 208) },
};

const QString IngredientData::NoUnit = "Ø";

QJsonArray UnitData::toJson (const UnitData &d) {
  QJsonArray j;
  j.append(d.id);
  j.append(d.text);
  j.append(d.used);
  Q_ASSERT(j.size() == 3);
  return j;
}

UnitData UnitData::fromJson (QJsonArray j) {
  Q_ASSERT(j.size() == 3);
  UnitData d;
  d.id = ID(j.takeAt(0).toInt());
  d.text = j.takeAt(0).toString();
  d.used = j.takeAt(0).toInt();
  return d;
}

QJsonArray IngredientData::toJson (const IngredientData &d) {
  QJsonArray j;
  j.append(d.id);
  j.append(d.text);
  j.append(d.group->id);
  j.append(d.used);
  Q_ASSERT(j.size() == 5);
  return j;
}

IngredientData IngredientData::fromJson (QJsonArray j) {
  Q_ASSERT(j.size() == 5);
  IngredientData d;
  d.id = ID(j.takeAt(0).toInt());
  d.text = j.takeAt(0).toString();
  d.group = &(*AlimentaryGroupData::database.find(ID(j.takeAt(0).toInt())));
  d.used = j.takeAt(0).toInt();
  return d;
}

} // end of namespace db
