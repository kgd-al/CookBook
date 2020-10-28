#include <QJsonArray>
#include <QColor>
#include <QMap>
#include <QVariant>
#include <QtMath>
#include <QIcon>

#include "ingredientlistentries.h"
#include "book.h"

namespace db {

// =============================================================================

QJsonValue IngredientListEntry::toJson(void) const {
  return QJsonArray { int(etype), toJsonInternal() };
}

IngredientListEntry::ptr IngredientListEntry::fromJson(const QJsonValue &j) {
  QJsonArray ja = j.toArray();
  EntryType type = EntryType(ja[0].toInt());
  IngredientListEntry *entry = nullptr;
  switch (type) {
  case EntryType::Ingredient:  entry = new IngredientEntry;  break;
  case  EntryType::SubRecipe:  entry = new SubRecipeEntry;   break;
  case EntryType::Decoration:  entry = new DecorationEntry;  break;
  }
  entry->etype = type;
  entry->fromJsonInternal(ja[1]);
  return ptr(entry);
}

// =============================================================================

static const QList<QChar> VOWELS { 'A', 'E', 'I', 'O', 'U' };
QVariant IngredientEntry::data (int role, double r) const {
  if (!valid()) return QVariant();

  if (role == Qt::DisplayRole) {
    static const int P = qPow(10, 2);
    QString d;
    double n = amount * r;
    int i = qRound(n * P);
    if (i % P)
      d = QString::number(n, 'f', i%10?2:1);
    else
      d = QString::number(i / P);
    d += " ";

    if (!unit->text.isEmpty() && unit->text != IngredientData::NoUnit) {
      d += unit->text + " ";

      /// NOTE Does not work for mute 'h'
      if (VOWELS.contains(type().at(0).toUpper()))
        d += "d'";
      else
        d += "de ";
    }

    d += type();

    if (!qualif.isEmpty())
      d += " (" + qualif + ")";

    return d;

  } else if (role == Qt::DecorationRole)
    return idata->group->decoration;

  else
    return QVariant();
}

QJsonValue IngredientEntry::toJsonInternal (void) const {
  Q_ASSERT(valid());
  return QJsonArray { amount, unit->id, idata->id, qualif };
}

void IngredientEntry::fromJsonInternal (const QJsonValue &j) {
  auto &idb = db::Book::current().ingredients;
  const QJsonArray ja = j.toArray();
  uint k=0;
  amount = ja[k++].toDouble();
  auto u_id = UnitData::ID(ja[k++].toInt());

  auto ingredient_id = ID(ja[k++].toInt());
  idata = &idb.at(ingredient_id);

//  auto u_it = idata->units.find(u_id);
//  Q_ASSERT(u_it != idata->units.end());
//  unit = &((*u_it).get());
  unit = &Book::current().units.at(u_id);

  qualif = ja[k++].toString();
}

// =============================================================================

SubRecipeEntry::SubRecipeEntry (Recipe *r)
  : IngredientListEntry(EntryType::SubRecipe),
    recipe(r) {}

QVariant SubRecipeEntry::data (int role, double r) const {
  switch (role) {
  case Qt::DisplayRole:
    return recipe->title;
  case Qt::DecorationRole:
//    return QIcon::fromTheme("accessories-text-editor");
    return db::MiscIcons::sub_recipe();
  case Qt::ToolTipRole:
    return recipe->ingredientList(r).join("\n");
  default:
    return QVariant();
  }
}

QJsonValue SubRecipeEntry::toJsonInternal (void) const {
  return recipe->id;
}

void SubRecipeEntry::fromJsonInternal (const QJsonValue &j) {
  recipe = (db::Recipe*)(uintptr_t)j.toInt();
}

void SubRecipeEntry::setRecipeFromHackedPointer(void) {
  Recipe::ID id = (Recipe::ID)reinterpret_cast<uintptr_t>(recipe);
  recipe = &Book::current().recipes.at(id);
}

// =============================================================================

QVariant DecorationEntry::data (int role, double) const {
  if (role == Qt::DisplayRole)
    return "\n" + text;
  else
    return QVariant();
}

QJsonValue DecorationEntry::toJsonInternal (void) const {
  return text;
}

void DecorationEntry::fromJsonInternal (const QJsonValue &j) {
  text = j.toString();
}


} // end of namespace db
