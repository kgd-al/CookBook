#ifndef UNITSMODEL_H
#define UNITSMODEL_H

#include <QSortFilterProxyModel>

#include "basemodel.hpp"
#include "recipedata.h"

namespace db {

class UnitsModel : public BaseModel<UnitData> {
  Q_OBJECT
public:
  UnitsModel (void) {}

  void add (const QString &text);
  void update (ID id, const QString &text);

  int columnCount (const QModelIndex& = QModelIndex()) const override {
    return 2;
  }

  QVariant
  headerData(int section, Qt::Orientation orientation, int role) const override;

  QVariant data (const QModelIndex &index, int role) const override;

  bool setData(const QModelIndex &index, const QVariant &value,
               int role) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  void valueModified(ID id) override;

  QJsonArray toJson (void) const;
  void fromJson (const QJsonArray &j);
};

} // end of namespace db

#endif // UNITSMODEL_H
