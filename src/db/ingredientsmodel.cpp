#include <QJsonArray>

#include "ingredientsmodel.h"

#include <QDebug>

namespace db {

int IngredientsModel::columnCount(const QModelIndex&) const {
  return 3;
}

QVariant IngredientsModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
  if (orientation != Qt::Horizontal)  return QVariant();
  if (role != Qt::DisplayRole)  return QVariant();

  switch (section) {
  case 0:   return "Name";
  case 1:   return "Units";
  case 2:   return "Used";
  default:  return "N/A";
  }
}

QVariant IngredientsModel::data (const QModelIndex &index, int role) const {
  if (role == Qt::DisplayRole) {
    auto &item = atIndex(index.row());
    switch (index.column()) {
    case 0:   return item.text;
    case 1:   return item.units.join(',');
    case 2:   return item.used;
    default:  return "N/A";
    }

  } else if (role == Qt::EditRole && index.column() == 0)
    return atIndex(index.row()).text;

  else if (role == Qt::DecorationRole && index.column() == 0) {
    auto item = atIndex(index.row());
    if (!item.group)  return QVariant();
    return item.group->color;

  } else if (role == IngredientRole)
    return atIndex(index.row()).id;

  else
    return QVariant();
}

bool IngredientsModel::setData(const QModelIndex &index, const QVariant &value,
                               int role) {

  qDebug() << "setData(" << index << value << role << ")";
  if (role != Qt::EditRole)  return false;

  atIndex(index.row()).text = value.toString();
  return true;
}

Qt::ItemFlags IngredientsModel::flags(const QModelIndex &index) const {
  auto flg = QAbstractTableModel::flags(index);
  if (index.isValid())
    flg |= Qt::ItemIsEditable;

  return flg;
}

bool IngredientsModel::insertRows(int row, int count, const QModelIndex &) {
  auto rows = rowCount();
  if (row < rows)  return false;

  qDebug().nospace() << "Inserting rows [" << row << "," << row+count-1 << "]. ";

  beginInsertRows(QModelIndex(), rows, rows);
    IngredientData d;
    d.id = nextIngredientID();
    _data.insert(d);
    _tmpData.insert(d.id);
  endInsertRows();

  return true;
}

int IngredientsModel::validateTemporaryData(const IIDList &ids) {
  for (auto id: ids)  _tmpData.erase(id);

  uint removed = 0;
  for (auto id: _tmpData) {
    auto it = _data.find(id);
    Q_ASSERT(it != _data.end());
    auto index = std::distance(_data.begin(), it);
    beginRemoveRows(QModelIndex(), index, index);
    _data.erase(it);
    endRemoveRows();
  }

  _tmpData.clear();

  return removed;
}

bool IngredientsModel::removeRows(int row, int count, const QModelIndex&) {
  qDebug().nospace() << "Removing rows [" << row << "," << row+count-1 << "]. "
                     << "Not implemented -> aborting";
  Q_ASSERT(false);
  return false;
}

void IngredientsModel::clear(void) {
  beginResetModel();
  _data.clear();
  endResetModel();
}

//void IngredientsModel::fromScratch(const RecipesListModel &recipes) {
//  beginResetModel();
//  _data.clear();
//  for (const auto &p: recipes) {
//    const Recipe &r = p.second;

//  }
//  endResetModel();
//}

void IngredientsModel::fromJson(const QJsonArray &j) {
  beginResetModel();
  QStringList units;
  _nextIngredientID = ID(0);
  for (const QJsonValue &v: j) {
    auto d = IngredientData::fromJson(v.toArray());
    units.append(d.units.list());
    _data.insert(d);
    _nextIngredientID = std::max(_nextIngredientID, d.id);
  }
  units.sort(Qt::CaseInsensitive);
  _unitsModel.setStringList(units);
  nextIngredientID();
  endResetModel();
}

QJsonArray IngredientsModel::toJson(void) const {
  QJsonArray j;
  for (const IngredientData &d: _data) {
    Q_ASSERT(d.group);
    j.append(IngredientData::toJson(d));
  }
  return j;
}

} // end of namespace db
