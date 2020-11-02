#ifndef DB_INGREDIENT_H
#define DB_INGREDIENT_H

#include <QString>
#include <QJsonValue>
#include <QSharedPointer>

#include "recipedata.h"

namespace db {

enum class EntryType : int {
  Ingredient = 0, SubRecipe, Decoration
};

struct IngredientListEntry {
  using ptr = QSharedPointer<IngredientListEntry>;

  EntryType etype;

  IngredientListEntry (EntryType t) : etype(t) {}
  virtual ~IngredientListEntry (void) {}

  virtual QVariant data (int role, double r = 1) const = 0;

  QJsonValue toJson  (void) const;
  static ptr fromJson(const QJsonValue &j);

protected:
  virtual QJsonValue toJsonInternal  (void) const = 0;
  virtual void fromJsonInternal (const QJsonValue &j) = 0;
};

struct IngredientEntry : public IngredientListEntry {
  double amount;
  UnitData *unit;
  IngredientData *idata;
  QString qualif;

  IngredientEntry (double a, UnitData *u, IngredientData *d, const QString &q)
    : IngredientListEntry(EntryType::Ingredient),
      amount(a), unit(u), idata(d), qualif(q) {}
  IngredientEntry (void) : IngredientEntry(0, nullptr, nullptr, "") {}
  virtual ~IngredientEntry (void) {}

  QVariant data (int role, double r) const override;

  QJsonValue toJsonInternal (void) const override;
  void fromJsonInternal (const QJsonValue &j) override;

  bool valid (void) const {
    return unit && idata;
  }

  const QString& type (void) const {
    return idata->text;
  }

  const QString& group (void) const {
    return idata->group->text;
  }
};

struct Recipe;
struct SubRecipeEntry : public IngredientListEntry {
  Recipe *recipe;

  SubRecipeEntry (Recipe *recipe);
  SubRecipeEntry (void) : SubRecipeEntry(nullptr) {}
  virtual ~SubRecipeEntry (void) {}

  QVariant data (int role, double ratio) const override;

  QJsonValue toJsonInternal (void) const override;
  void fromJsonInternal (const QJsonValue &j) override;
  void setRecipeFromHackedPointer(void);

private:
};

struct DecorationEntry : public IngredientListEntry {
  QString text;

  DecorationEntry (const QString &t)
    : IngredientListEntry(EntryType::Decoration), text(t) {}
  DecorationEntry (void) : DecorationEntry("Not a decoration") {}
  virtual ~DecorationEntry (void) {}

  QVariant data (int role, double) const override;

  QJsonValue toJsonInternal (void) const override;
  void fromJsonInternal (const QJsonValue &j) override;
};

} // end of namespace db

#endif // DB_INGREDIENT_H
