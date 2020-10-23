#include <QJsonArray>

#include "ingredientsmodel.h"

#include <QDebug>

namespace db {

int IngredientsModel::columnCount(const QModelIndex&) const {
  return 2;
}

QVariant IngredientsModel::headerData(int section, Qt::Orientation orientation,
                                      int role) const {
  if (orientation != Qt::Horizontal)  return QVariant();
  if (role != Qt::DisplayRole)  return QVariant();

  switch (section) {
  case 0:   return "Name";
  case 1:   return "Used";
  default:  return "N/A";
  }
}

void IngredientsModel::add (const QString &text, G_ID gid) {
  int index = rowCount();
  insertRows(index, 1, QModelIndex());
  auto &item = atIndex(index);
  item.text = text;
  item.used = 0;
  item.group = &db::at<AlimentaryGroupData>(gid);
  valueModified(item.id);
}

void IngredientsModel::update(ID id, const QString &text, G_ID gid) {
  auto &item = at(id);
  if (!text.isEmpty())  item.text = text;
  if (gid != ID::INVALID && gid != item.group->id)
    item.group = &db::at<AlimentaryGroupData>(gid);
  valueModified(item.id);
}

QVariant IngredientsModel::data (const QModelIndex &index, int role) const {
  switch (role) {
  case Qt::DisplayRole: {
    auto &item = atIndex(index.row());
    switch (index.column()) {
    case 0:   return item.text;
    case 1:   return item.used;
    default:  return "N/A";
    }
  }

  case Qt::EditRole:
    return (index.column() == 0) ? atIndex(index.row()).text : QVariant();

  case Qt::DecorationRole:
    if (index.column() == 0) {
      auto item = atIndex(index.row());
      if (!item.group)  return QVariant();
      return item.group->decoration;
    } else
      return QVariant();

  case IDRole:
    return atIndex(index.row()).id;

  case PtrRole:
    return QVariant::fromValue(&atIndex(index.row()));

  default:
    return QVariant();
  }
}

bool IngredientsModel::setData(const QModelIndex &index, const QVariant &value,
                               int role) {

  qDebug() << "setData(" << index << value << role << ")";
  if (role != Qt::EditRole)  return false;
  Q_ASSERT(index.isValid());

  atIndex(index.row()).text = value.toString();
  emit dataChanged(index, index, {role});
  return true;
}

Qt::ItemFlags IngredientsModel::flags(const QModelIndex &index) const {
  auto flg = QAbstractTableModel::flags(index);
  if (index.isValid() && index.column() == 0)
    flg |= Qt::ItemIsEditable;

  return flg;
}

bool IngredientsModel::insertRows(int row, int count, const QModelIndex &) {
  auto q = qDebug().nospace();
  q << "Inserting rows [" << row << "," << row+count-1 << "] in " << this;

  auto rows = rowCount();
  if (row < rows)  return false;

  q << ": ok";

  beginInsertRows(QModelIndex(), rows, rows);
    IngredientData d;
    d.id = nextID();
    _data.insert(d);
    _tmpData.insert(d.id);
  endInsertRows();

  return true;
}

int IngredientsModel::validateTemporaryData(const IDList &ids) {
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

//bool IngredientsModel::removeRows(int row, int count, const QModelIndex&) {
//  qDebug().nospace() << "Removing rows [" << row << "," << row+count-1 << "]. "
//                     << "Not implemented -> aborting";
//  Q_ASSERT(false);
//  return false;
//}

void IngredientsModel::valueModified(ID id) {
  int index = indexOf(id);
  emit dataChanged(createIndex(index, 0), createIndex(index, columnCount()));
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
  for (const QJsonValue &v: j) {
    auto d = IngredientData::fromJson(v.toArray());
    _data.insert(d);
    _nextID = std::max(_nextID, d.id);
  }
  nextID();
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
