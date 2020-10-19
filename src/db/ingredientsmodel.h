#ifndef INGREDIENTSMODEL_H
#define INGREDIENTSMODEL_H

#include <QAbstractItemModel>

#include "ingredientdata.h"
#include "basemodel.h"
#include "recipesmodel.h"

namespace db {

class IngredientsModel : public BaseModel<IngredientData> {
public:
  static constexpr auto IngredientRole = Qt::UserRole + 1;
//  using IID = IngredientData::ID;
  using IIDList = std::set<ID>;

  IngredientsModel(void) {}

  void clear (void);

  int columnCount(const QModelIndex& = QModelIndex()) const override;

  QVariant headerData(int section, Qt::Orientation orientation, int role)
    const override;
  QVariant data (const QModelIndex &index, int role) const override;

  bool setData(const QModelIndex &index, const QVariant &value,
               int role) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  bool insertRows(int row, int count, const QModelIndex&) override;
  int validateTemporaryData (const IIDList &ids);

  bool removeRows(int row, int count, const QModelIndex &) override;

  void valueModified (ID id) override;

  QJsonArray toJson (void) const;
  void fromJson (const QJsonArray &j);

private:
  IIDList _tmpData;
};

}

#endif // INGREDIENTSMODEL_H
