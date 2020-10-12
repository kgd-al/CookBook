#ifndef EDITABLESTRINGLISTMODEL_H
#define EDITABLESTRINGLISTMODEL_H

#include <QAbstractListModel>
#include <QStringList>

namespace db {

struct EditableStringListModel : public QAbstractListModel {
  EditableStringListModel (const QStringList &l) : _data(l) {}
  EditableStringListModel (void) : EditableStringListModel(QStringList()) {}

  EditableStringListModel (const EditableStringListModel &that)
    : QAbstractListModel(), _data(that._data) {}

  EditableStringListModel& operator= (EditableStringListModel that);

  int rowCount(const QModelIndex & = QModelIndex()) const override;
  QVariant data (const QModelIndex &index, int role) const override;

  bool insertRows(int row, int count, const QModelIndex&) override;

  bool
  setData(const QModelIndex &index, const QVariant &value, int role) override;

  int indexOf (const QString &s) const;
  void append (const QString &s);

  const QString& operator[] (int i) const { return _data[i];  }
  QString& operator[] (int i) { return _data[i];  }

  QString join (char c) const { return _data.join(c); }

  const QStringList& list (void) const { return _data; }

  friend void swap (EditableStringListModel &lhs, EditableStringListModel &rhs);

private:
  QStringList _data;
};

} // end of namespace db
#endif // EDITABLESTRINGLISTMODEL_H
