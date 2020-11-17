#ifndef RECIPESLISTMODEL_H
#define RECIPESLISTMODEL_H

#include "basemodel.hpp"
#include "recipe.h"

namespace db {

QByteArray toByteArray (const QSet<db::ID> &set);
QSet<db::ID> fromByteArray (const QByteArray &array);

class RecipesModel : public BaseModel<Recipe> {
public:
  static constexpr auto SortRole = PtrRole+42;

  RecipesModel(void);

  QModelIndex addRecipe(Recipe &&r);
  void delRecipe (Recipe *r);

  static int columnCount (void);
  static int titleColumn (void);
  int columnCount(const QModelIndex &parent) const override;

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  QVariant data (const QModelIndex &index, int role) const override;

  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QMimeData* mimeData(const QModelIndexList &indexes) const override;
  Qt::DropActions supportedDragActions(void) const override {
    return Qt::LinkAction;
  }

  void valueModified(ID id) override;

  void fromJson (const QJsonArray &a);
  QJsonArray toJson(void);
};

} // end of namespace db

#endif // RECIPESLISTMODEL_H
