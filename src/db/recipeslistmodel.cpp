#include <QJsonArray>

#include "recipeslistmodel.h"

#include <QDebug>

namespace db {

RecipesListModel::RecipesListModel(void) : _nextRecipeID(ID(1)) {}

void RecipesListModel::addRecipe(Recipe &&r) {
  beginInsertRows(QModelIndex(),rowCount(),rowCount());
  recipes.emplace(nextRecipeID(), r);
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
    auto id = Recipe::idFromJson(v);
    recipes.emplace(id, Recipe::fromJson(v));
    _nextRecipeID = std::max(id, _nextRecipeID);
  }

  nextRecipeID();
}

QJsonArray RecipesListModel::toJson(void) {
  QJsonArray a;
  for (const auto &p: recipes)
    a.append(Recipe::toJson(p.second));
  return a;
}

} // end of namespace db
