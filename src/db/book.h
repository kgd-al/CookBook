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
  RecipesModel recipes;
  IngredientsModel ingredients;
  UnitsModel units;

  PlanningModel planning;

  Book(void);

  QModelIndex addRecipe (Recipe &&r);

  bool load (void);
#ifndef Q_OS_ANDROID
  bool autosave (bool spontaneous);
  bool save (void);
#endif
  bool close (QWidget *widget = nullptr);

  bool isModified (void) {
    return _modified;
  }

  static Book& current (void);

  static QString monitoredName (void);
  static QString monitoredDir (void);
  static QString monitoredPath (void);

signals:
  void modified (bool m);

private:
  bool _modified;

  void setModified (bool m);
  void setModified (void) {
    setModified(true);
  }
};

} // end of namespace db

#endif // DB_BOOK_H
