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

  int show(db::Recipe *recipe, bool readOnly,
           QModelIndex index = QModelIndex(),
           double ratio = 1);

  void setReadOnly (bool ro);
  bool isReadOnly (void) {
    return _readOnly;
  }

  void setIndex (QModelIndex index);

signals:
  void validated (void);  // Confirm button has been clicked
  void deleted (void);  // Delete button has been cliked (and deletion is legal)

private:
  QModelIndex _index;
  db::Recipe *_data;
  bool _readOnly;

#ifndef Q_OS_ANDROID
  QSplitter *_vsplitter, *_hsplitter;
#else
#endif

  LabelEdit *_title;

  double _displayedPortions;
  QDoubleSpinBox *_portions;
  LabelEdit *_portionsLabel;

  struct {
    QLabel *subrecipe, *basic, *regimen, *type, *duration, *status;
    QWidget *holder;
  } _consult;

#ifndef Q_OS_ANDROID
  struct {
    QLabel *subrecipe;
    QCheckBox *basic;
    QComboBox *regimen, *type, *duration, *status;
    QWidget *holder;
  } _edit;
#endif

  GUIList *_ingredients, *_steps;
#ifndef Q_OS_ANDROID
  ListControls *_icontrols, *_scontrols;
#endif

  QTextEdit *_notes;

#ifndef Q_OS_ANDROID
  QPushButton *_prev, *_next;

  QPushButton *_toggle, *_apply;
#endif

  void makeLayout (QLayout *mainLayout,
                   const QMap<QString, QWidget *> &widgets);

  void update (db::Recipe *recipe, bool readOnly, QModelIndex index,
               double ratio);
  bool hasSibling (int dir);

#ifndef Q_OS_ANDROID
  void updateNavigation (void);

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

#else
  void addIngredient (Ingredient_ptr i);
  void addStep (const QString &text);
#endif

  double currentRatio (void) const;
  void updateDisplayedPortions (bool spontaneous);
  void updateDisplayedPortions (void) {
    return updateDisplayedPortions(true);
  }

  void showPrevious (void);
  void showNext (void);

  void showSubRecipe (QListWidgetItem *li);

  void keyPressEvent(QKeyEvent *e) override;
  void closeEvent(QCloseEvent *e) override;
  bool safeQuit (QEvent *e);

#ifndef Q_OS_ANDROID
  void deleteRequested (void);
#endif

#ifdef Q_OS_ANDROID
  bool event(QEvent *event) override;
#endif
};

} // end of namespace gui

#endif // GUI_RECIPE_H
