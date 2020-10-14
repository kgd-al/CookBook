#ifndef GUI_RECIPE_H
#define GUI_RECIPE_H

#include <QDialog>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QStringListModel>
#include <QSpinBox>
#include <QLabel>

#include <QJsonObject>

#include "../db/recipe.h"

namespace gui {

using GUIList = QListWidget;

class ListControls : public QWidget {
public:
  ListControls (GUIList *view);

  QToolButton* addButton (void) {  return _add;  }
  QToolButton* editButton (void) { return _edit;  }

private:
  const GUIList *_view;
  QToolButton *_add, *_edit, *_del;

  void setState (void);
};

class Recipe : public QDialog {
  Q_OBJECT
public:
  using Ingredient_ptr = db::Recipe::Ingredient_ptr;

  Recipe(QWidget *parent);

  int show(db::Recipe *recipe, bool readOnly, double ratio = 1);

  void setReadOnly (bool ro);
  bool isReadOnly (void) {
    return !_title->isEnabled();
  }

signals:
  void validated (void);  // Confirm button has been clicked

private:
  db::Recipe *_data;

  QLineEdit *_title;

  double _displayedPortions;
  QDoubleSpinBox *_portions;
  QLineEdit *_portionsLabel;
  GUIList *_ingredients, *_steps;
  ListControls *_icontrols, *_scontrols;

  QTextEdit *_notes;

  QPushButton *_toggle;

  void toggleReadOnly (void);
  bool confirmed(void);

  void writeThrough (void);

  void addIngredient (void);
  void addIngredient (Ingredient_ptr i);
  void editIngredient (void);

  void addStep (void);
  void addStep (const QString &text);
  void editStep (void);

  double currentRatio (void) const;
  void updateDisplayedPortions (bool spontaneous);
  void updateDisplayedPortions (void) {
    return updateDisplayedPortions(true);
  }

  void showSubRecipe (QListWidgetItem *li);

  void keyPressEvent(QKeyEvent *e) override;
  void closeEvent(QCloseEvent *e) override;
  bool safeQuit (QEvent *e);
};

} // end of namespace gui

#endif // GUI_RECIPE_H
