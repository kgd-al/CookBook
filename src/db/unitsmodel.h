#ifndef UNITSMODEL_H
#define UNITSMODEL_H

#include <QSortFilterProxyModel>

#include "basemodel.h"
#include "ingredientdata.h"

namespace db {

class UnitsModel : public BaseModel<UnitData> {
  Q_OBJECT
public:
  static constexpr auto IDRole = Qt::UserRole + 1;
//  static constexpr auto UnitRole = IDRole + 1;

  UnitsModel (void) {}

//  EditableStringListModel (const EditableStringListModel &that)
//    : QAbstractListModel(), _data(that._data) {}

//  EditableStringListModel& operator= (EditableStringListModel that);

  int columnCount (const QModelIndex& = QModelIndex()) const override {
    return 2;
  }

  QVariant
  headerData(int section, Qt::Orientation orientation, int role) const override;

  QVariant data (const QModelIndex &index, int role) const override;

  bool insertRows(int row, int count, const QModelIndex&) override;

  bool
  setData(const QModelIndex &index, const QVariant &value, int role) override;

  void valueModified(ID id) override;

//  int indexOf (const QString &s) const;
//  void append (const QString &s);

//  const QString& operator[] (int i) const { return _data[i];  }
//  QString& operator[] (int i) { return _data[i];  }

//  QString join (char c) const { return _data.join(c); }

//  const QStringList& list (void) const { return _data; }

//  friend void swap (EditableStringListModel &lhs, EditableStringListModel &rhs);

  QJsonArray toJson (void) const;
  void fromJson (const QJsonArray &j);
};

//class UnitsSubModel : public QSortFilterProxyModel {
//  Q_OBJECT
//public:
//  UnitsSubModel (UnitReferenceDatabase &db);
//  bool filterAcceptsRow (int source_row, const QModelIndex&) const override;

//  bool insertRows(int row, int count, const QModelIndex&) override;

//  bool
//  setData(const QModelIndex &index, const QVariant &value, int role) override;

//private:
//  UnitReferenceDatabase &_database;

//  const UnitsModel& model (void) const;
//  UnitsModel& model (void) {
//    return const_cast<UnitsModel&>(
//      const_cast<const UnitsSubModel*>(this)->model());
//  }
//};

//class UnitsModelSorter : public QSortFilterProxyModel {
//  Q_OBJECT
//public:
//  UnitsModelSorter (void);
//  bool lessThan(const QModelIndex &lhs, const QModelIndex &rhs) const override;
//};

} // end of namespace db

#endif // UNITSMODEL_H
