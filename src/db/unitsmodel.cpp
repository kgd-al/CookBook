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

QVariant UnitsModel::data (const QModelIndex &index, int role) const {
  switch (role) {
  case Qt::DisplayRole: {
    const UnitData &u = atIndex(index.row());
    if (index.column() == 0)
      return u.text;
    else
      return u.used;
  }
  case IDRole:
    return atIndex(index.row()).id;
  case Qt::EditRole:
    return atIndex(index.row()).text;
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

// =============================================================================
//UnitsSubModel::UnitsSubModel (UnitReferenceDatabase &db) : _database(db) {
//  setSourceModel(&Book::current().units);
//}

//bool UnitsSubModel::filterAcceptsRow(int source_row, const QModelIndex&) const {
//  QDebug q = qDebug().nospace();
//  q << "accept(" << source_row << "/" << _database.size() << ") @" << this;
//  if (_database.empty())  return true;

//  auto b = _database.find(model().atIndex(source_row).id) != _database.end();
//  q << ": " << b;
//  return b;
//}

//bool UnitsSubModel::insertRows(int row, int count, const QModelIndex &parent) {
//  QDebug q = qDebug().nospace();
//  q << "Overwriting row = " << row << " with row = model.rowCount() = "
//    << model().rowCount() << "\n";
//  row = model().rowCount();
//  q << "Inserting rows [" << row << "," << row+count-1 << "] out of "
//    << rowCount() << " in " << this;

//  auto id = model().nextIDNoIncrement();
//  auto inserted = QSortFilterProxyModel::insertRows(model().rowCount(), count, parent);
//  q << ": " << inserted;
//  if (!inserted)  return inserted;

//  auto rows = rowCount();
////  if (row < rows) return false;

//  q = qDebug();
//  beginInsertRows(QModelIndex(), rows, rows);
//    q << _database.size() << "\n";
//    _database.insert(model().at(id));
//    q << _database.size() << "\n";
//  endInsertRows();

//  return true;
//}

//bool UnitsSubModel::setData(const QModelIndex &index, const QVariant &value,
//                            int role) {

//  qDebug() << "setData(" << index << value << role << ")";
//  return QSortFilterProxyModel::setData(index, value, role);
//}

//const UnitsModel& UnitsSubModel::model (void) const {
//  return dynamic_cast<UnitsModel&>(*QSortFilterProxyModel::sourceModel());
//}

// =============================================================================
//UnitsModelSorter::UnitsModelSorter (void) {
//  setSourceModel(&db::Book::current().units);
//}

//bool UnitsModelSorter::lessThan(const QModelIndex &lhs,
//                                const QModelIndex &rhs) const {
//  int u_lhs = lhs.siblingAtColumn(1).data().toInt(),
//      u_rhs = rhs.siblingAtColumn(1).data().toInt();
//  if (u_lhs != u_rhs) return u_lhs < u_rhs;
//  return lhs.
//}
} // end of namespace db
