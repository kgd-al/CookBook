#include <type_traits>

#include <QJsonArray>
#include <QPainter>
#include <QSettings>
#include <QApplication>
#include <QStyle>

#include "recipedata.h"
#include "unitsmodel.h"
#include "book.h"

#include <QDebug>

namespace db {

int iconSize (void) {
  QSettings settings;
  auto s = settings.value("iconSize", 15).toInt();
  qDebug() << "using" << s << "as icon size";
  return s;
}

void fontChanged (const QFont &font) {
  QSettings settings;
  settings.setValue("font", font);
  QFontMetrics metrics (font);
  settings.setValue("iconSize", metrics.height());
  qDebug() << font << "is" << metrics.height() << "pixels high";
}

QPixmap iconFromFile (const QString &filepath) {
  QPixmap p (filepath);
  qDebug() << "Loaded" << p << "from" << filepath;
  return p;
}

QPixmap rectangularIconFromColor (const QColor &d) {
  QPixmap p (iconSize(), iconSize());
  p.fill(d);
  return p;
}

QPixmap circularIconFromColor (const QColor &d) {
  QPixmap p (iconSize(), iconSize());
  p.fill(Qt::transparent);
  QPainter painter (&p);
  painter.setPen(Qt::NoPen);
  painter.setBrush(d);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.drawEllipse(0, 0, iconSize(), iconSize());
  return p;
}

#define ENTRY(I, N, R, G, B) \
  { ID(I), N, rectangularIconFromColor(QColor::fromRgb(R,G,B)) }
AlimentaryGroupData::Database AlimentaryGroupData::_database;
void AlimentaryGroupData::loadDatabase(void) {
  _database = {
    ENTRY( 1, "Protéines", 255,   0,   0),
    ENTRY( 2,   "Verdure",   0, 255,   0),
    ENTRY( 3,  "Céréales", 255, 165,   0),
    ENTRY( 4,    "Sucres", 255, 105, 180),
    ENTRY( 5,    "Épices",   0, 128,   0),
    ENTRY( 6,  "Liquides", 255,   0, 190),
    ENTRY( 7,   "Graines", 160,  82,  42),
    ENTRY( 8,  "Laitiers", 255, 248, 220),
    ENTRY( 9,   "Lipides", 255, 215,   0),
    ENTRY(10,   "Alcools",  64, 224, 208)
  };
}
#undef ENTRY

#define ENTRY(I, C) { ID(I), "", circularIconFromColor(C) }
StatusData::Database StatusData::_database;
void StatusData::loadDatabase (void) {
  _database = {
    ENTRY(1, QColor(Qt::red)   ),
    ENTRY(2, QColor("#ffb000") ),
    ENTRY(3, QColor(Qt::green) )
  };
}
#undef ENTRY

#define ENTRY(I, N, F) { ID(I), N, iconFromFile(F) }
RegimenData::Database RegimenData::_database;
void RegimenData::loadDatabase (void) {
  _database = {
    ENTRY(1, "Protéiné",   ":/icons/protein.svg"    ),
    ENTRY(2, "Végétarien", ":/icons/vegetarian.svg" ),
    ENTRY(3, "Vegan",      ":/icons/vegan.svg"      )
  };
}

DishTypeData::Database DishTypeData::_database;
void DishTypeData::loadDatabase (void) {
  _database = {
    ENTRY(1, "Neutre", ":/icons/neutral.svg"  ),
    ENTRY(2, "Salé",   ":/icons/salt.svg"     ),
    ENTRY(3, "Sucré",  ":/icons/sugar.svg"    ),
  };
}

DurationData::Database DurationData::_database;
void DurationData::loadDatabase (void) {
  _database = {
    ENTRY(1, "Rapide",    ":/icons/time-short.svg"  ),
    ENTRY(2, "Journée",   ":/icons/time-short.svg"  ),
    ENTRY(3, "Lendemain", ":/icons/time-long.svg"   ),
    ENTRY(4, "Très long", ":/icons/time-long.svg"   ),
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
