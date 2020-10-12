#ifndef RECIPESLISTMODEL_H
#define RECIPESLISTMODEL_H

#include <QStringListModel>

#include "recipe.h"

namespace db {

class RecipesListModel : public QStringListModel {
public:
//  static constexpr int RecipeRole = Qt::UserRole + 1;

  RecipesListModel(void);

  void addRecipe (Recipe &&r);

  // Model Qt interface
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data (const QModelIndex &index, int role) const override;

  // map interface
  void clear (void);

  auto find (int key) const {
    return recipes.find(key);
  }

  auto begin (void) const {
    return recipes.cbegin();
  }

  auto end (void) const {
    return recipes.cend();
  }

  Recipe& at (int key) {
    return recipes.at(key);
  }

  Recipe& fromIndex(const QModelIndex &i);

  const Recipe& recipe (int i) const {
    auto it = recipes.begin();
    std::advance(it, i);
    return it->second;
  }

  Recipe& recipe (int i) {
    return const_cast<Recipe&>(
      const_cast<const RecipesListModel*>(this)->recipe(i));
  }

  void fromJson (const QJsonArray &a);
  QJsonArray toJson(void);

private:
  std::map<int, Recipe> recipes;

  Recipe::ID _nextRecipeID;
  Recipe::ID nextRecipeID (void) {
    auto v = _nextRecipeID;
    _nextRecipeID = ID(int(_nextRecipeID)+1);
    return v;
  }
};

} // end of namespace db

#endif // RECIPESLISTMODEL_H
