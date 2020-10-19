#ifndef RECIPESLISTMODEL_H
#define RECIPESLISTMODEL_H

#include <QStringListModel>

#include "recipe.h"

namespace db {

class RecipesModel : public QAbstractListModel {
public:
  using RecipesDatabase = transparent_set<Recipe>;
//  static constexpr int RecipeRole = Qt::UserRole + 1;

  RecipesModel(void);

  void addRecipe (Recipe &&r);
  void delRecipe (Recipe *r);

  // Model Qt interface
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data (const QModelIndex &index, int role) const override;

  // map interface
  void clear (void);

  auto find (Recipe::ID key) const {
    return recipes.find(key);
  }

  auto begin (void) const {
    return recipes.cbegin();
  }

  auto end (void) const {
    return recipes.cend();
  }

  const Recipe& at (Recipe::ID key) const {
    auto it = recipes.find(key);
    if (it == recipes.end())
      throw std::invalid_argument("No recipe for this key");
    return *it;
  }

  Recipe& at (Recipe::ID key) {
    return const_cast<Recipe&>(
      const_cast<const RecipesModel*>(this)->at(key));
  }

  Recipe& fromIndex(const QModelIndex &i);

  const Recipe& recipe (int i) const {
    auto it = recipes.begin();
    std::advance(it, i);
    return *it;
  }

  Recipe& recipe (int i) {
    return const_cast<Recipe&>(
      const_cast<const RecipesModel*>(this)->recipe(i));
  }

  void fromJson (const QJsonArray &a);
  QJsonArray toJson(void);

private:
  RecipesDatabase recipes;

  Recipe::ID _nextRecipeID;
  Recipe::ID nextRecipeID (void) {
    auto v = _nextRecipeID;
    _nextRecipeID = ID(int(_nextRecipeID)+1);
    return v;
  }
};

} // end of namespace db

#endif // RECIPESLISTMODEL_H
