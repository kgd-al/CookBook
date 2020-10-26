#include <QJsonArray>
#include <QPainter>

#include "recipesmodel.h"

#include <QDebug>

namespace db {

RecipesModel::RecipesModel(void) {}

void RecipesModel::addRecipe(Recipe &&r) {
  int i = rowCount();
  beginInsertRows(QModelIndex(), i, i);
  r.id = nextID();
  _data.emplace(r);
  endInsertRows();
}

void RecipesModel::delRecipe(Recipe *r) {
  removeItem(r->id);
}

int RecipesModel::columnCount(const QModelIndex&) const {
  return 1;
}

QVariant RecipesModel::data (const QModelIndex &index, int role) const {
  QVariant v;
  switch (role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    v = atIndex(index.row()).title;
    break;
  case IDRole:
    v = atIndex(index.row()).id;
    break;
  case PtrRole:
    v = QVariant::fromValue(&atIndex(index.row()));
    break;
  }
  return v;
}

void RecipesModel::valueModified(ID id) {
  int index = indexOf(id);
  emit dataChanged(createIndex(index, 0), createIndex(index, columnCount()));
}

void RecipesModel::fromJson(const QJsonArray &a) {
  for (const QJsonValue &v: a) {
    Recipe r = Recipe::fromJson(v);
    _data.emplace(r);
    _nextID = std::max(r.id, _nextID);
  }

  for (const Recipe &r: _data)
    for (auto &i: r.ingredients)
      if (i->etype == EntryType::SubRecipe)
        static_cast<SubRecipeEntry*>(i.data())->setRecipeFromHackedPointer();

  nextID();
}

QJsonArray RecipesModel::toJson(void) {
  QJsonArray a;
  for (const auto &p: _data)
    a.append(Recipe::toJson(p));
  return a;
}

} // end of namespace db
