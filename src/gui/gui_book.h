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
struct RecipeFilter;

class Book : public QMainWindow {
  Q_OBJECT
public:
  Book(QWidget *parent = 0);
  ~Book();

#ifndef Q_OS_ANDROID
  bool saveRecipes(void);
  bool saveRecipes(const QString &path);
  bool overwriteRecipes(void);
#endif

  bool loadRecipes(void);
  bool loadRecipes(const QString &path);

  bool load(const QJsonObject &json);

#ifndef Q_OS_ANDROID
  bool save(QJsonObject &json);
#endif

  void closeEvent(QCloseEvent *e) override;

private:
  QSplitter *_splitter;
  QTableView *_recipes;
  FilterView *_filter;
  RecipeFilter *_proxy;

#ifndef Q_OS_ANDROID
  void addRecipe (void);
#endif

  void showRecipe (const QModelIndex &index);

#ifndef Q_OS_ANDROID
  void showIngredientsManager (void);
  void showUpdateManager (void);
#endif

  void setAutoTitle (void);
  void setModified (bool m);

#ifdef Q_OS_ANDROID
  bool event (QEvent *event) override;
#endif
};

} // end of namespace gui

#endif // GUI_BOOK_H
