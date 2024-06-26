#ifndef INGREDIENTENTRYDIALOG_H
#define INGREDIENTENTRYDIALOG_H

#include <QDialog>
#include <QStackedLayout>

#include "autofiltercombobox.hpp"
#include "../db/ingredientlistentries.h"
#include "../db/unitsmodel.h"

namespace gui {

struct IngredientDialog : public QDialog {
  using Ingredient_ptr = db::IngredientListEntry::ptr;

  QComboBox *entryTypeSelection;
  QStackedLayout *entryLayout;
  static const QMap<db::EntryType, QString> entryTypeDesc;

  QLineEdit *amount;
  AutoFilterComboBox *unit, *type, *group;
  QDoubleValidator validator;
  QLineEdit *qualif;

  AutoFilterComboBox *recipe;

  QLineEdit *decoration;

  IngredientDialog (QWidget *parent, const QString &title);

  void setIngredient (const Ingredient_ptr &e);

  Ingredient_ptr ingredient (void) const;

  db::EntryType entryType (void) const;

  void updateLayout (void);

  void closeEvent(QCloseEvent *e) override;

private:
  bool validate (void);

  void typeChanged (void);
};


} // end of namespace gui

#endif // INGREDIENTENTRYDIALOG_H
