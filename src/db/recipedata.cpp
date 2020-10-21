#include <type_traits>

#include <QJsonArray>
#include <QPainter>

#include "recipedata.h"
#include "unitsmodel.h"
#include "book.h"

#include <QDebug>

namespace db {

const int ICON_SIZE = 45;

QPixmap iconFromFile (const QString &filepath) {
  QPixmap p (filepath);
  qDebug() << "Loaded" << p << "from" << filepath;
  return p;
}

const AlimentaryGroupData::Database AlimentaryGroupData::database {
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
QPixmap AlimentaryGroupData::iconFromDecoration (const QColor &d) {
  QPixmap p (ICON_SIZE, ICON_SIZE);
  p.fill(d);
  return p;
}

const RegimenData::Database RegimenData::database {
  { ID(0), "Protéiné",   ":/icons/protein.svg"    },
  { ID(1), "Végétarien", ":/icons/vegetarian.svg" },
  { ID(2), "Vegan",      ":/icons/vegan.svg"      },
};
QPixmap RegimenData::iconFromDecoration (const QString &d) {
  return iconFromFile(d);
}

const StatusData::Database StatusData::database {
  { ID(0), "", QColor(Qt::red)   },
  { ID(1), "", QColor("#ffb000") },
  { ID(2), "", QColor(Qt::green) },
};
QPixmap StatusData::iconFromDecoration (const QColor &d) {
  QPixmap p (ICON_SIZE, ICON_SIZE);
  p.fill(Qt::transparent);
  QPainter painter (&p);
  painter.setPen(Qt::NoPen);
  painter.setBrush(d);
  painter.setRenderHint(QPainter::Antialiasing, true);
  painter.drawEllipse(0, 0, ICON_SIZE, ICON_SIZE);
  return p;
}

const DishTypeData::Database DishTypeData::database {
  { ID(0), "Neutre", ":/icons/neutral.svg"  },
  { ID(1), "Salé",   ":/icons/salt.svg"     },
  { ID(2), "Sucré",  ":/icons/sugar.svg"    },
};
QPixmap DishTypeData::iconFromDecoration (const QString &d) {
  return iconFromFile(d);
}

const DurationData::Database DurationData::database {
  { ID(0), "Rapide",    ":/icons/time-short.svg"  },
  { ID(1), "Journée",   ":/icons/time-short.svg"  },
  { ID(2), "Lendemain", ":/icons/time-long.svg"   },
  { ID(3), "Très long", ":/icons/time-long.svg"   },
};
QPixmap DurationData::iconFromDecoration (const QString &d) {
  return iconFromFile(d);
}

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
  d.group = &(*AlimentaryGroupData::database.find(ID(j.takeAt(0).toInt())));
  d.used = j.takeAt(0).toInt();
  return d;
}

} // end of namespace db
