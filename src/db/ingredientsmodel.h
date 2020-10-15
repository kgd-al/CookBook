#ifndef INGREDIENTSMODEL_H
#define INGREDIENTSMODEL_H

#include <QAbstractItemModel>

#include "ingredientdata.h"
#include "recipeslistmodel.h"

namespace db {

class IngredientsModel : public QAbstractTableModel {
public:
  static constexpr auto IngredientRole = Qt::UserRole + 1;
  using IID = IngredientData::ID;
  using IIDList = std::set<IID>;

  IngredientsModel(void) {}

  void clear (void);

  const IngredientData& at (IID id) const {
    return *_data.find(id);
  }

  IngredientData& at (IID id) {
    return const_cast<IngredientData&>(
      const_cast<const IngredientsModel*>(this)->at(id));
  }

  const IngredientData& atIndex (int index) const {
    auto it = _data.begin();
    std::advance(it, index);
    return *it;
  }

  IngredientData& atIndex (int index) {
    return const_cast<IngredientData&>(
      const_cast<const IngredientsModel*>(this)->atIndex(index));
  }

  int rowCount (const QModelIndex & = QModelIndex()) const override {
    return _data.size();
  }

  int columnCount(const QModelIndex &) const override;

  QVariant headerData(int section, Qt::Orientation orientation, int role)
    const override;
  QVariant data (const QModelIndex &index, int role) const override;

  bool setData(const QModelIndex &index, const QVariant &value,
               int role) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  bool insertRows(int row, int count, const QModelIndex&) override;
  int validateTemporaryData (const IIDList &ids);

  bool removeRows(int row, int count, const QModelIndex &) override;

  QJsonArray toJson (void) const;
  void fromJson (const QJsonArray &j);

  QStringListModel& allUnitsModel (void) {
    return _unitsModel;
  }

private:
  IngredientsDatabase _data;
  QStringListModel _unitsModel;

  IIDList _tmpData;

  IID _nextIngredientID;
  IID nextIngredientID (void) {
    auto v = _nextIngredientID;
    _nextIngredientID = IID(int(_nextIngredientID)+1);
    return v;
  }
};

}

#endif // INGREDIENTSMODEL_H
