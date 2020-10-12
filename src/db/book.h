#ifndef DB_BOOK_H
#define DB_BOOK_H

#include <map>

#include "recipeslistmodel.h"
#include "ingredientsmodel.h"

namespace db {

struct Book {
  QString path;
  bool modified;

  RecipesListModel recipes;
  IngredientsModel ingredients;

  Book(void);

  void addRecipe (Recipe &&r);

  bool save (const QString &path);
  bool load (const QString &path);

  void clear (void);

  const QString& title (int id) const;

  static Book& current (void);
};

} // end of namespace db

#endif // DB_BOOK_H
