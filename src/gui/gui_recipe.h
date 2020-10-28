#ifndef GUI_RECIPE_H
#define GUI_RECIPE_H

#include <QDialog>
#include <QSplitter>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QStringListModel>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>

#include <QJsonObject>

#include "listcontrols.h"
#include "../db/recipe.h"

namespace gui {

using GUIList = QListWidget;
struct LabelEdit;

class Recipe : public QDialog {
  Q_OBJECT
public:
  using Ingredient_ptr = db::Recipe::Ingredient_ptr;

  Recipe(QWidget *parent);

  int show(db::Recipe *recipe, bool readOnly, double ratio = 1);

  void setReadOnly (bool ro);
  bool isReadOnly (void) {
    return _readOnly;
  }

signals:
  void validated (void);  // Confirm button has been clicked
  void deleted (void);  // Delete button has been cliked (and deletion is legal)

private:
  db::Recipe *_data;
  bool _readOnly;

  QSplitter *_vsplitter, *_hsplitter;

  LabelEdit *_title;

  double _displayedPortions;
  QDoubleSpinBox *_portions;
  LabelEdit *_portionsLabel;

  struct {
    QLabel *subrecipe, *basic, *regimen, *status, *type, *duration;
    QWidget *holder;
  } _consult;

  struct {
    QLabel *subrecipe;
    QCheckBox *basic;
    QComboBox *regimen, *status, *type, *duration;
    QWidget *holder;
  } _edit;

  GUIList *_ingredients, *_steps;
  ListControls *_icontrols, *_scontrols;

  QTextEdit *_notes;

  QPushButton *_toggle, *_apply;

  void toggleReadOnly (void);
  bool confirmed(void);
  void apply (void);

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

  void deleteRequested (void);
};

} // end of namespace gui

#endif // GUI_RECIPE_H
