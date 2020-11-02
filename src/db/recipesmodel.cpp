#include <QJsonArray>
#include <QPainter>

#include "recipesmodel.h"

#include <QDebug>

namespace db {

RecipesModel::RecipesModel(void) {}

QModelIndex RecipesModel::addRecipe(Recipe &&r) {
  int i = rowCount();
  beginInsertRows(QModelIndex(), i, i);
  r.id = nextID();
  auto p = _data.emplace(r.id, r);
  endInsertRows();

  Q_ASSERT(p.second);
  return index(std::distance(_data.begin(), p.first), 0);
}

void RecipesModel::delRecipe(Recipe *r) {
  removeItem(r->id);
}

int RecipesModel::columnCount(void) {
  return 7;
}

int RecipesModel::titleColumn(void) {
  return columnCount()-1;
}

int RecipesModel::columnCount(const QModelIndex&) const {
  return RecipesModel::columnCount();
}

QVariant RecipesModel::headerData(int section, Qt::Orientation orientation,
                                  int role) const {
  if (role != Qt::SizeHintRole) return QVariant();
  if (section == titleColumn() && orientation == Qt::Horizontal)
    return QVariant();
  return QSize(iconSize(), iconSize());
}

QVariant RecipesModel::data (const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  const Recipe &r = atIndex(index.row());
  switch (role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    return (index.column() == titleColumn()) ? r.title : QVariant();

  case Qt::DecorationRole:
    switch (index.column()) {
    case 0:   return r.basicIcon();
    case 1:   return r.subrecipeIcon();
    case 2:   return r.regimen->decoration;
    case 3:   return r.type->decoration;
    case 4:   return r.duration->decoration;
    case 5:   return r.status->decoration;
    default:  return QVariant();
    }

  case Qt::SizeHintRole:
    return (index.column() < titleColumn()) ? QSize(0, 0)
                                            : QVariant();

  case IDRole:
    return r.id;

  case PtrRole:
    return QVariant::fromValue(&r);

  case SortRole:
    switch (index.column()) {
    case 0: return r.basic;
    case 1: return r.used;
    case 2: return r.regimen->id;
    case 3: return r.type->id;
    case 4: return r.duration->id;
    case 5: return r.status->id;
    case 6: return r.title;
    default:  return QVariant();
    }
  }
  return QVariant();
}

void RecipesModel::valueModified(ID id) {
  int index = indexOf(id);
  emit dataChanged(createIndex(index, 0), createIndex(index, columnCount()));
}

void RecipesModel::fromJson(const QJsonArray &a) {
  for (const QJsonValue &v: a) {
    Recipe r = Recipe::fromJson(v);
    _data.insert({r.id, r});
    _nextID = std::max(r.id, _nextID);
  }

  for (const auto &p: _data)
    for (auto &i: p.second.ingredients)
      if (i->etype == EntryType::SubRecipe)
        static_cast<SubRecipeEntry*>(i.data())->setRecipeFromHackedPointer();

  nextID();
}

QJsonArray RecipesModel::toJson(void) {
  QJsonArray a;
  for (const auto &p: _data)
    a.append(Recipe::toJson(p.second));
  return a;
}

} // end of namespace db
