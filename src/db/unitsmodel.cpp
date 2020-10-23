#include <QJsonArray>

#include "unitsmodel.h"
#include "book.h"

#include <QDebug>

namespace db {

QVariant UnitsModel::headerData(int section, Qt::Orientation orientation,
                                int role) const {
  if (orientation != Qt::Horizontal)  return QVariant();
  if (role != Qt::DisplayRole)  return QVariant();

  switch (section) {
  case 0:   return "Name";
  case 1:   return "Used";
  default:  return QVariant();
  }
}

void UnitsModel::add (const QString &text) {
  int index = rowCount();
  insertRows(index, 1, QModelIndex());
  auto &item = atIndex(index);
  item.text = text;
  item.used = 0;
  valueModified(item.id);
}

void UnitsModel::update(ID id, const QString &text) {
  auto &item = at(id);
  if (!text.isEmpty())  item.text = text;
  valueModified(item.id);
}

QVariant UnitsModel::data (const QModelIndex &index, int role) const {
  switch (role) {
  case Qt::DisplayRole: {
    const UnitData &u = atIndex(index);
    if (index.column() == 0)
      return u.text;
    else
      return u.used;
  }
  case IDRole:
    return atIndex(index).id;
  case PtrRole:
    return QVariant::fromValue(&atIndex(index));
  case Qt::EditRole:
    return atIndex(index).text;
  }
  return QVariant();
}

bool UnitsModel::insertRows(int row, int count, const QModelIndex &) {
  QDebug q = qDebug().nospace();
  q << "Inserting rows [" << row << "," << row+count-1 << "] out of "
    << rowCount() << " in " << this;
  auto rows = rowCount();
  if (row < rows) return false;
  q << ": ok";

  beginInsertRows(QModelIndex(), rows, rows);
    UnitData d;
    d.id = nextID();
    _data.insert(d);
  endInsertRows();

  return true;
}

bool UnitsModel::setData(const QModelIndex &index, const QVariant &value,
                         int role) {

  qDebug() << "setData(" << index << value << role << ")";
  if (role != Qt::EditRole)  return false;

  atIndex(index.row()).text = value.toString();
  emit dataChanged(index, index, {role});
  return true;
}

Qt::ItemFlags UnitsModel::flags(const QModelIndex &index) const {
  auto flg = QAbstractTableModel::flags(index);
  if (index.isValid() && index.column() == 0)
    flg |= Qt::ItemIsEditable;

  return flg;
}

void UnitsModel::valueModified(ID id) {
  int index = indexOf(id);
  emit dataChanged(createIndex(index, 0), createIndex(index, columnCount()));
}

void UnitsModel::fromJson(const QJsonArray &j) {
  beginResetModel();
  for (const QJsonValue &v: j) {
    auto d = UnitData::fromJson(v.toArray());
    _data.insert(d);
    _nextID = std::max(_nextID, d.id);
  }
  if (_data.empty())
    _data.insert({nextID(), IngredientData::NoUnit, 0});
  else
    nextID();
  endResetModel();
}

QJsonArray UnitsModel::toJson(void) const {
  QJsonArray j;
  for (const UnitData &d: _data)  j.append(UnitData::toJson(d));
  return j;
}

} // end of namespace db
