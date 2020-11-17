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

struct FilterView;
struct PlanningView;

class Book : public QMainWindow {
  Q_OBJECT
public:
  Book(QWidget *parent = 0);
  ~Book();

  bool loadDefaultBook(void);
#ifndef Q_OS_ANDROID
  bool overwriteRecipes(bool spontaneous = true);
#endif

  void closeEvent(QCloseEvent *e) override;

private:
#ifndef Q_OS_ANDROID
  QSplitter *_hsplitter, *_vsplitter;
#else
  QSplitter *_splitter;
#endif

  QTableView *_recipes;
  FilterView *_filter;

#ifndef Q_OS_ANDROID
  PlanningView *_planning;
#endif

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
  void showAbout (void);

  void setAutoTitle (void);
//  void setModified (bool m);

  void toggleFilterArea (void);
  void togglePlanningArea (void);

#ifdef Q_OS_ANDROID
  bool event (QEvent *event) override;
#endif
};

} // end of namespace gui

#endif // GUI_BOOK_H
