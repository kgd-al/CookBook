#ifndef GUI_BOOK_H
#define GUI_BOOK_H

#include <QMainWindow>
#include <QSplitter>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QTableView>

#include "../db/book.h"
#include "gui_recipe.h"
#include "autofiltercombobox.hpp"

namespace gui {

class FilterView;
class PlanningView;

class Book : public QMainWindow {
  Q_OBJECT
public:
  Book(QWidget *parent = 0);
  ~Book();

  bool loadDefaultBook(void);
#ifndef Q_OS_ANDROID
  bool overwriteRecipes(bool spontaneous = true);
  bool printRecipes(void);
#endif

  void closeEvent(QCloseEvent *e) override;

private:
#ifndef Q_OS_ANDROID
  QSplitter *_hsplitter, *_vsplitter;
#else
  QSplitter *_outterSplitter, *_innerSplitter;
#endif

  QTableView *_recipes;
  FilterView *_filter;
  PlanningView *_planning;

  void buildLayout (void);

#ifndef Q_OS_ANDROID
  void addRecipe (void);
#endif

  void showRecipe (const QModelIndex &index);

#ifndef Q_OS_ANDROID
  void showIngredientsManager (void);
  void showUpdateManager (void);
  void showRepairUtility (void);
  void showSettings (void);
#endif
  void showSynchronizer(void);
  void showAbout (void);

  void setAutoTitle (void);
//  void setModified (bool m);

  void toggleFilterArea (void);

  void togglePlanningArea (void);
  void showPlanningArea (bool show);
};

} // end of namespace gui

#endif // GUI_BOOK_H
