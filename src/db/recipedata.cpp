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
#ifndef Q_OS_ANDROID
  return 15;
#else
  return 32;
#endif
}

QSize iconQSize (void) {
  return QSize (iconSize(), iconSize());
}

QSize iconDrawQSize (void) {
  return 4 * iconQSize();
}

QIcon iconFromFile (const QString &filepath) {
  QIcon i (filepath);
//  qDebug() << "Loaded" << i << "from" << filepath;
  return i;
}

QPixmap rectangularIconFromColor (const QColor &d) {
  QPixmap p (iconDrawQSize());
  p.fill(d);
  return p;
}

QPixmap circularIconFromColor (const QColor &d) {
  QPixmap p (iconDrawQSize());
  p.fill(Qt::transparent);
  QPainter painter (&p);
  painter.setPen(Qt::NoPen);
  painter.setBrush(d);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.drawEllipse(0, 0, iconDrawQSize().width(), iconDrawQSize().height());
  return p;
}

const QIcon& MiscIcons::sub_recipe (void) {
  static const auto i = iconFromFile(":/icons/sub-recipe.png");
  return i;
}
const QIcon& MiscIcons::basic_recipe (void) {
  static const auto i = iconFromFile(":/icons/basic-recipe.png");
  return i;
}

#define ENTRY(I, N, R, G, B) \
{ ID(I), { ID(I), N, rectangularIconFromColor(QColor::fromRgb(R,G,B)) } }
template <>
AlimentaryGroupData::Database AlimentaryGroupData::loadDatabase(void) {
  return AlimentaryGroupData::Database {
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

#define ENTRY(I, C) { ID(I), { ID(I), "", circularIconFromColor(C) } }
template <>
StatusData::Database StatusData::loadDatabase (void) {
  return StatusData::Database {
    ENTRY(1, QColor(Qt::red)   ),
    ENTRY(2, QColor("#ffb000") ),
    ENTRY(3, QColor(Qt::green) )
  };
}
#undef ENTRY

#define ENTRY(I, N, F) { ID(I), { ID(I), N, iconFromFile(F) } }
template <>
RegimenData::Database RegimenData::loadDatabase (void) {
  return RegimenData::Database {
    ENTRY(1, "Protéiné",   ":/icons/regimen-protein.png"    ),
    ENTRY(2, "Végétarien", ":/icons/regimen-vegetarian.png" ),
    ENTRY(3, "Vegan",      ":/icons/regimen-vegan.png"      )
  };
}

template <>
DishTypeData::Database DishTypeData::loadDatabase (void) {
  return DishTypeData::Database {
    ENTRY(1, "Neutre", ":/icons/type-neutral.png"  ),
    ENTRY(2, "Salé",   ":/icons/type-salted.png"   ),
    ENTRY(3, "Sucré",  ":/icons/type-sugar.ico"    ),
  };
}

template <>
DurationData::Database DurationData::loadDatabase (void) {
  return DurationData::Database {
    ENTRY(1, "Rapide",    ":/icons/time-short-2.png"    ),
    ENTRY(2, "Journée",   ":/icons/time-medium-2.png"   ),
    ENTRY(3, "Lendemain", ":/icons/time-long.png"       ),
    ENTRY(4, "Très long", ":/icons/time-very-long.png"  ),
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
