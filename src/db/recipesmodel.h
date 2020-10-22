#ifndef RECIPESLISTMODEL_H
#define RECIPESLISTMODEL_H

#include "basemodel.h"
#include "recipe.h"

namespace db {

class RecipesModel : public BaseModel<Recipe> {
public:
  RecipesModel(void);

  void addRecipe (Recipe &&r);
  void delRecipe (Recipe *r);

  // Model Qt interface
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data (const QModelIndex &index, int role) const override;

  void valueModified(ID id) override;

  void fromJson (const QJsonArray &a);
  QJsonArray toJson(void);
};

} // end of namespace db

#endif // RECIPESLISTMODEL_H
