#include <QJsonArray>

#include "recipeslistmodel.h"

namespace db {

RecipesListModel::RecipesListModel(void) : _nextRecipeID(ID(0)) {}

void RecipesListModel::addRecipe(Recipe &&r) {
  int i = rowCount();
  beginInsertRows(QModelIndex(), i, i);
  r.id = nextRecipeID();
  recipes.emplace(r);
  endInsertRows();
}

void RecipesListModel::delRecipe(Recipe *r) {
  auto it = find(r->id);
  if (it != recipes.end()) {
    auto index = std::distance(recipes.begin(), it);
    beginRemoveRows(QModelIndex(), index, index);

    recipes.erase(it);
    endRemoveRows();
  }
}

Recipe& RecipesListModel::fromIndex (const QModelIndex &i) {
  return recipe(i.row());
}

int RecipesListModel::rowCount(const QModelIndex&) const {
  return recipes.size();
}

QVariant RecipesListModel::data (const QModelIndex &index, int role) const {
  QVariant v;
  switch (role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    v = recipe(index.row()).title;
    break;
  }
  return v;
}

void RecipesListModel::clear(void) {
  beginResetModel();
  recipes.clear();
  endResetModel();
}

void RecipesListModel::fromJson(const QJsonArray &a) {
  for (const QJsonValue &v: a) {
    Recipe r = Recipe::fromJson(v);
    recipes.emplace(r);
    _nextRecipeID = std::max(r.id, _nextRecipeID);
  }

  for (const Recipe &r: recipes)
    for (auto &i: r.ingredients)
      if (i->etype == EntryType::SubRecipe)
        static_cast<SubRecipeEntry*>(i.data())->setRecipeFromHackedPointer();

  nextRecipeID();
}

QJsonArray RecipesListModel::toJson(void) {
  QJsonArray a;
  for (const auto &p: recipes)
    a.append(Recipe::toJson(p));
  return a;
}

} // end of namespace db
