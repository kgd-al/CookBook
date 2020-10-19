#ifndef DB_BOOK_H
#define DB_BOOK_H

#include <map>

#include "recipesmodel.h"
#include "ingredientsmodel.h"
#include "unitsmodel.h"

namespace db {

struct Book {
  QString path;
  bool modified;

  RecipesModel recipes;
  IngredientsModel ingredients;
  UnitsModel units;

  Book(void);

  void addRecipe (Recipe &&r);

  bool save (const QString &path);
  bool load (const QString &path);

  void clear (void);

  const QString& title (Recipe::ID id) const;

  static Book& current (void);
};

} // end of namespace db

#endif // DB_BOOK_H
