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

  bool saveRecipes(void);
  bool saveRecipes(const QString &path);
  bool overwriteRecipes(void);

  bool loadRecipes(void);
  bool loadRecipes(const QString &path);

  bool load(const QJsonObject &json);
  bool save(QJsonObject &json);

  void closeEvent(QCloseEvent *e) override;

private:
  QSplitter *_splitter;
  QTableView *_recipes;
  FilterView *_filter;
  RecipeFilter *_proxy;

  void addRecipe (void);

  void showRecipe (const QModelIndex &index);

  void showIngredientsManager (void);
  void showUpdateManager (void);

  void setAutoTitle (void);
  void setModified (bool m);
};

} // end of namespace gui

#endif // GUI_BOOK_H
