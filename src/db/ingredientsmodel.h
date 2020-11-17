#ifndef INGREDIENTSMODEL_H
#define INGREDIENTSMODEL_H

#include <QAbstractItemModel>

#include "recipedata.h"
#include "basemodel.hpp"
#include "recipesmodel.h"

namespace db {

class IngredientsModel : public BaseModel<IngredientData> {
public:

  using IDList = std::set<ID>;
  using G_ID = AlimentaryGroupData::ID;

  IngredientsModel(void) {}

  void clear (void);
  void add (const QString &text, G_ID gid);
  void update (ID id, const QString &text, G_ID gid);

  int columnCount(const QModelIndex& = QModelIndex()) const override;

  QVariant headerData(int section, Qt::Orientation orientation, int role)
    const override;
  QVariant data (const QModelIndex &index, int role) const override;

  bool setData(const QModelIndex &index, const QVariant &value,
               int role) override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;

  bool insertRows (int row, int count,
                   const QModelIndex &parent = QModelIndex()) override;
  int validateTemporaryData (const IDList &ids);

  void valueModified (ID id) override;

  QJsonArray toJson (void) const;
  void fromJson (const QJsonArray &j);

private:
  IDList _tmpData;
};

}

#endif // INGREDIENTSMODEL_H
