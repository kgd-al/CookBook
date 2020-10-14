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

  nextRecipeID();
}

QJsonArray RecipesListModel::toJson(void) {
  QJsonArray a;
  for (const auto &p: recipes)
    a.append(Recipe::toJson(p));
  return a;
}

} // end of namespace db
