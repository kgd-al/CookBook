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
    if (!unit->isEmpty()) {
      d += *unit + " ";

      /// NOTE Does not work for mute 'h'
      if (VOWELS.contains(type().at(0).toUpper()))
        d += "d'";
      else
        d += "de ";
    }
    d += type();
    return d;

  } else if (role == Qt::DecorationRole)
    return idata->group->color;

  else
    return QVariant();
}

QJsonValue IngredientEntry::toJsonInternal (void) const {
  Q_ASSERT(valid());
  return QJsonArray { amount, *unit, idata->id };
}

void IngredientEntry::fromJsonInternal (const QJsonValue &j) {
  const auto &idb = db::Book::current().ingredients;
  const QJsonArray ja = j.toArray();
  uint k=0;
  amount = ja[k++].toDouble();
  QString unit = ja[k++].toString();

  auto ingredient_id = ID(ja[k++].toInt());
  idata = &idb.at(ingredient_id);

  int uindex = idata->units.indexOf(unit);
  Q_ASSERT(uindex >= 0);
  this->unit = &idata->units[uindex];
}

// =============================================================================

SubRecipeEntry::SubRecipeEntry (const Recipe *r)
  : IngredientListEntry(EntryType::SubRecipe),
    recipe(r) {}

QVariant SubRecipeEntry::data (int role, double r) const {
  switch (role) {
  case Qt::DisplayRole:
    return recipe->title;
  case Qt::DecorationRole:
    return QIcon::fromTheme("accessories-text-editor");
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
  recipe = fromID(Recipe::ID(j.toInt()));
}

const Recipe* SubRecipeEntry::fromID(Recipe::ID id) {
  return &Book::current().recipes.at(id);
}

// =============================================================================

QVariant DecorationEntry::data (int role, double) const {
  if (role == Qt::DisplayRole)
    return text;
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
