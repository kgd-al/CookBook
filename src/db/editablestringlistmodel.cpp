#include "editablestringlistmodel.h"

namespace db {

//using ESLM = EditableStringListModel;

//ESLM& ESLM::operator= (ESLM that) {
//  swap(*this, that);
//  return *this;
//}

//int ESLM::rowCount(const QModelIndex &) const {
//  return _data.size();
//}

//QVariant ESLM::data (const QModelIndex &index, int role) const {
//  switch (role) {
//  case Qt::EditRole:
//  case Qt::DisplayRole:  return _data.at(index.row());
//  default:
//    return QVariant();
//  }
//}

//bool ESLM::insertRows(int row, int count, const QModelIndex&) {
//  if (count != 1) return false;
//  if (row < rowCount()) return false;
//  _data.append("");
//  return true;
//}

//bool ESLM::setData(const QModelIndex &index, const QVariant &value, int role) {
//  if (!index.isValid()) return false;
//  if (!value.canConvert<QString>()) return false;
//  if (role != Qt::EditRole)  return false;
//  _data[index.row()] = value.toString();
//  return true;
//}

//int ESLM::indexOf (const QString &s) const {
//  return _data.indexOf(s);
//}

//void ESLM::append (const QString &s) {
//  beginInsertRows(QModelIndex(), rowCount(), rowCount());
//  _data.append(s);
//  endInsertRows();
//}

//void swap (ESLM &lhs, ESLM &rhs){
//  using std::swap;
//  lhs.beginResetModel();
//  rhs.beginResetModel();
//  swap(lhs._data, rhs._data);
//  rhs.endResetModel();
//  lhs.endResetModel();
//}

} // end of namespace db
