#ifndef DB_BOOK_H
#define DB_BOOK_H

#include <map>

#include "recipesmodel.h"
#include "ingredientsmodel.h"
#include "unitsmodel.h"
#include "planningmodel.h"

namespace db {

struct Book : public QObject {
  Q_OBJECT
public:
  QString path;

  RecipesModel recipes;
  IngredientsModel ingredients;
  UnitsModel units;

  PlanningModel planning;

  Book(void);

  QModelIndex addRecipe (Recipe &&r);

  bool save (const QString &path);
  bool load (const QString &path = monitoredPath());

  void clear (void);

  void setModified (void);
  bool isModified (void) {
    return _modified;
  }

  static Book& current (void);

  static QString monitoredName (void);
  static QString monitoredDir (void);
  static QString monitoredPath (void);

signals:
  void modified (void);

private:
  bool _modified;
};

} // end of namespace db

#endif // DB_BOOK_H
