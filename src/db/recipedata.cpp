#include <type_traits>

#include <QJsonArray>
#include <QPainter>

#include "recipedata.h"
#include "unitsmodel.h"
#include "book.h"

#include <QDebug>

namespace db {

static constexpr int icons = 4;
static constexpr int margin = 1;
const int ICON_SIZE = 15;
const int RECIPE_ICONS_SIZE = icons*ICON_SIZE + (icons-1)*margin;

QPixmap iconFromFile (const QString &filepath) {
  QPixmap p (filepath);
  qDebug() << "Loaded" << p << "from" << filepath;
  return p;
}

QPixmap rectangularIconFromColor (const QColor &d) {
  QPixmap p (ICON_SIZE, ICON_SIZE);
  p.fill(d);
  return p;
}

QPixmap circularIconFromColor (const QColor &d) {
  QPixmap p (ICON_SIZE, ICON_SIZE);
  p.fill(Qt::transparent);
  QPainter painter (&p);
  painter.setPen(Qt::NoPen);
  painter.setBrush(d);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.drawEllipse(0, 0, ICON_SIZE, ICON_SIZE);
  return p;
}

#define ENTRY(I, N, R, G, B) \
  { ID(I), N, rectangularIconFromColor(QColor::fromRgb(R,G,B)) }
AlimentaryGroupData::Database AlimentaryGroupData::_database;
void AlimentaryGroupData::loadDatabase(void) {
  _database = {
    ENTRY(0, "Protéines", 255,   0,   0),
    ENTRY(1,   "Verdure",   0, 255,   0),
    ENTRY(2,  "Céréales", 255, 165,   0),
    ENTRY(3,    "Sucres", 255, 105, 180),
    ENTRY(4,    "Épices",   0, 128,   0),
    ENTRY(5,  "Liquides", 255,   0, 190),
    ENTRY(6,   "Graines", 160,  82,  42),
    ENTRY(7,  "Laitiers", 255, 248, 220),
    ENTRY(8,   "Lipides", 255, 215,   0),
    ENTRY(9,   "Alcools",  64, 224, 208)
  };
}
#undef ENTRY

#define ENTRY(I, C) { ID(I), "", circularIconFromColor(C) }
StatusData::Database StatusData::_database;
void StatusData::loadDatabase (void) {
  _database = {
    ENTRY(0, QColor(Qt::red)   ),
    ENTRY(1, QColor("#ffb000") ),
    ENTRY(2, QColor(Qt::green) )
  };
}
#undef ENTRY

#define ENTRY(I, N, F) { ID(I), N, iconFromFile(F) }
RegimenData::Database RegimenData::_database;
void RegimenData::loadDatabase (void) {
  _database = {
    ENTRY(0, "Protéiné",   ":/icons/protein.svg"    ),
    ENTRY(1, "Végétarien", ":/icons/vegetarian.svg" ),
    ENTRY(2, "Vegan",      ":/icons/vegan.svg"      )
  };
}

DishTypeData::Database DishTypeData::_database;
void DishTypeData::loadDatabase (void) {
  _database = {
    ENTRY(0, "Neutre", ":/icons/neutral.svg"  ),
    ENTRY(1, "Salé",   ":/icons/salt.svg"     ),
    ENTRY(2, "Sucré",  ":/icons/sugar.svg"    ),
  };
}

DurationData::Database DurationData::_database;
void DurationData::loadDatabase (void) {
  _database = {
    ENTRY(0, "Rapide",    ":/icons/time-short.svg"  ),
    ENTRY(1, "Journée",   ":/icons/time-short.svg"  ),
    ENTRY(2, "Lendemain", ":/icons/time-long.svg"   ),
    ENTRY(3, "Très long", ":/icons/time-long.svg"   ),
  };
}
#undef ENTRY

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
  Q_ASSERT(j.size() == 4);
  return j;
}

IngredientData IngredientData::fromJson (QJsonArray j) {
  Q_ASSERT(j.size() == 4);
  IngredientData d;
  d.id = ID(j.takeAt(0).toInt());
  d.text = j.takeAt(0).toString();
  d.group = &at<AlimentaryGroupData>(ID(j.takeAt(0).toInt()));
  d.used = j.takeAt(0).toInt();
  return d;
}

} // end of namespace db
