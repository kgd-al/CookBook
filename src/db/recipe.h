#ifndef DB_RECIPE_H
#define DB_RECIPE_H

#include <QList>
#include <QTime>

#include "ingredientlistentries.h"

namespace db {

struct Recipe {
  using ID = db::ID;
  using Database = transparent_set<Recipe>;

  ID id = ID::INVALID;
  int used;

  QString title;

  double portions;
  QString portionsLabel;

  const RegimenData *regimen;
  const StatusData *status;
  const DishTypeData *type;
  const DurationData *duration;

  using Ingredient_ptr = IngredientListEntry::ptr;
  using IngredientList = QList<Ingredient_ptr>;
  IngredientList ingredients;
  QStringList steps;
  QString notes;

  QStringList ingredientList (double r) const;

  // On update
  void updateUsageCounts (const IngredientList &newList);

  // On deletion
  void updateUsageCounts(void);

  static Recipe defaultRecipe (void);

  static QJsonValue toJson (const Recipe &r);
  static Recipe fromJson (const QJsonValue &j);

private:
  Recipe();
};

} // end of namespace db

Q_DECLARE_METATYPE(db::Recipe*)

#endif // DB_RECIPE_H
