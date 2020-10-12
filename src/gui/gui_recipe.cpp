#include <QVBoxLayout>
#include <QStackedLayout>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QToolButton>

#include <QJsonDocument>
#include <QJsonArray>

#include <QInputDialog>
#include <QMessageBox>

#include <QDebug>

#include "gui_recipe.h"
#include "ingrediententrydialog.h"

#include "../db/book.h"
#include "../db/recipeslistmodel.h"

namespace gui {

QWidget* spacer (void) {
  auto w = new QWidget;
  w->setMinimumHeight(10);
  return w;
}

struct IngredientListItem : public QListWidgetItem {
  using Ingredient_ptr = db::IngredientEntry::ptr;
  Ingredient_ptr ing;
  double ratio;

  IngredientListItem (Ingredient_ptr i) : ing(i), ratio(1) {}

  QVariant data (int role) const {
    QVariant d = ing->data(role, ratio);
    if (d.isValid())
      return d;
    else
      return QListWidgetItem::data(role);
  }

  void setRatio (double r) {
    ratio = r;
    setText(data(Qt::DisplayRole).toString());
  }
};

struct StepListItem : public QListWidgetItem {
  StepListItem (const QString &text) : QListWidgetItem(text) {}
  QVariant data (int role) const override {
    auto d = QListWidgetItem::data(role);
    if (role == Qt::DisplayRole) {
      return "Step " + QString::number(listWidget()->row(this)+1) + ":\n"
          + d.toString();
    } else
      return d;
  }

  QString step (void) const {
    return QListWidgetItem::data(Qt::DisplayRole).toString();
  }

  void setStep (const QString &step) {
    setData(Qt::DisplayRole, step);
  }
};

ListControls::ListControls (GUIList *view) : _view(view) {
  QHBoxLayout *layout = new QHBoxLayout;
  layout->addWidget(_add = new QToolButton);
  _add->setIcon(QIcon::fromTheme("list-add"));
  layout->addWidget(_edit = new QToolButton);
  _edit->setIcon(QIcon::fromTheme("insert-text"));
  layout->addWidget(_del = new QToolButton);
  _del->setIcon(QIcon::fromTheme("list-remove"));
  QWidget *spacer = new QWidget;
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  layout->addWidget(spacer);
  layout->setMargin(0);
  setLayout(layout);

  connect(_view->selectionModel(), &QItemSelectionModel::currentRowChanged,
          this, &ListControls::setState);

//  connect(_del, &QToolButton::clicked,
//          [this] {
//    if (QMessageBox::question(this, "Confirm", "Confirm suppression?")
//        == QMessageBox::Yes)
//      qDeleteAll(_view->selectedItems());
//  });

  setState();
}

void ListControls::setState(void) {
  bool hasSelection = _view->currentIndex().isValid();
  _edit->setEnabled(hasSelection);
  _del->setEnabled(hasSelection);
}

Recipe::Recipe(QWidget *parent) : QDialog(parent) {
  QVBoxLayout *mainLayout = new QVBoxLayout;

    _title = new QLineEdit ("Untitled");
    _title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(_title);

    mainLayout->addWidget(spacer());
    QHBoxLayout *midLayout = new QHBoxLayout;
      QVBoxLayout *ilayout = new QVBoxLayout;
        ilayout->addWidget(new QLabel ("Ingredients:"));
        QHBoxLayout *playout = new QHBoxLayout;
          _portions = new QDoubleSpinBox;
          _portions->setMinimum(0);
          _portions->setSingleStep(.25);
          playout->addWidget(_portions);
          _portions->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
          playout->addWidget(_portionsLabel = new QLineEdit("?"));
        ilayout->addLayout(playout);
        _ingredients = new GUIList;
        _ingredients->setAcceptDrops(true);
        _ingredients->setDragDropMode(QAbstractItemView::InternalMove);
        ilayout->addWidget(_ingredients);
        ilayout->addWidget(_icontrols = new ListControls(_ingredients));
      midLayout->addLayout(ilayout, 0);

      QVBoxLayout *nlayout = new QVBoxLayout;
        nlayout->addWidget(new QLabel ("Notes:"));
        _notes = new QTextEdit;
        nlayout->addWidget(_notes);
      midLayout->addLayout(nlayout, 1);
    mainLayout->addLayout(midLayout);

    mainLayout->addWidget(spacer());
    QVBoxLayout *slayout = new QVBoxLayout;
      slayout->addWidget(new QLabel("Steps:"));
      _steps = new GUIList;
      _steps->setAcceptDrops(true);
      _steps->setDragDropMode(QAbstractItemView::InternalMove);
      _steps->setWordWrap(true);
      slayout->addWidget(_steps);
      slayout->addWidget(_scontrols = new ListControls(_steps));
    mainLayout->addLayout(slayout);

    QDialogButtonBox *buttons = new QDialogButtonBox;
      auto close = new QPushButton("Close");
      close->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      connect(close, &QPushButton::clicked, this, &Recipe::done);
      buttons->addButton(close, QDialogButtonBox::AcceptRole);

      _toggle = new QPushButton("");
      connect(_toggle, &QPushButton::clicked, this, &Recipe::toggleReadOnly);
      _toggle->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
      buttons->addButton(_toggle, QDialogButtonBox::ActionRole);
    mainLayout->addWidget(buttons);

  connect(_icontrols->addButton(), &QToolButton::clicked,
          this, qOverload<>(&Recipe::addIngredient));

  connect(_icontrols->editButton(), &QToolButton::clicked,
          this, &Recipe::editIngredient);

  connect(_scontrols->addButton(), &QToolButton::clicked,
          this, qOverload<>(&Recipe::addStep));

  connect(_scontrols->editButton(), &QToolButton::clicked,
          this, &Recipe::editStep);

  connect(_portions, qOverload<double>(&QDoubleSpinBox::valueChanged),
          this, &Recipe::updateDisplayedPortions);

  connect(_ingredients, &QListWidget::itemActivated,
          this, &Recipe::showSubRecipe);

  _data = nullptr;
  setLayout(mainLayout);
  setReadOnly(false);
}

int Recipe::show (db::Recipe *recipe, bool readOnly, double ratio) {
  _data = recipe;

  _title->setText(_data->title);

  for (const db::Recipe::Ingredient_ptr &i: recipe->ingredients) addIngredient(i);
  for (const QString &s: recipe->steps)  addStep(s);

  _displayedPortions = ratio * _data->portions;
  _portions->setValue(_data->portions);
  _portionsLabel->setText(_data->portionsLabel);

  _notes->setText(_data->notes);

  setReadOnly(readOnly);

  return exec();
}

void Recipe::writeThrough(void) {
  _data->title = _title->text();

  _data->ingredients.clear();
  for (int i=0; i<_ingredients->count(); i++)
    _data->ingredients.append(
      static_cast<const IngredientListItem*>(_ingredients->item(i))->ing);

  _data->steps.clear();
  for (int i=0; i<_steps->count(); i++)
    _data->steps.append(
      static_cast<const StepListItem*>(_steps->item(i))->step());

  _data->portions = _displayedPortions;
  _data->portionsLabel = _portionsLabel->text();

  _data->notes = _notes->toPlainText();
}

void Recipe::setReadOnly(bool ro) {
  _title->setEnabled(!ro);
  _notes->setEnabled(!ro);

  if (!ro) {
    _displayedPortions = _portions->value();
    if (_data)  _portions->setValue(_data->portions);
  } else {
    _portions->setValue(_displayedPortions);
  }
  _portionsLabel->setEnabled(!ro);

  _ingredients->setDragEnabled(!ro);
  _icontrols->setVisible(!ro);
  _steps->setDragEnabled(!ro);
  _scontrols->setVisible(!ro);

  if (isReadOnly()) _toggle->setText("Edit");
  else              _toggle->setText("Confirm");

  if (_data) {
    QString wtitle = _data->title;
    if (!ro)  wtitle += " (editing)";
    setWindowTitle(wtitle);
  }
}

void Recipe::toggleReadOnly(void) {
  setReadOnly(!isReadOnly());
  if (isReadOnly()) confirmed();
}

void Recipe::confirmed(void) {
  emit validated();
  if (QMessageBox::question(this, "Validate?", "Confirm recipe modification?",
                            QMessageBox::Yes | QMessageBox::No)
      == QMessageBox::Yes)
    writeThrough();
}

void Recipe::addIngredient(void) {
  IngredientDialog d (this, "Input ingredient details");
  if (QDialog::Accepted == d.exec())
    addIngredient(d.ingredient());
}

void Recipe::addIngredient(Ingredient_ptr i) {
  _ingredients->addItem(new IngredientListItem(i));
}

void Recipe::editIngredient(void) {
  auto item = static_cast<IngredientListItem*>(_ingredients->currentItem());
  auto ing = item->ing;

  IngredientDialog d (this, "Update ingredient details");
  d.setIngredient(ing);

  if (QDialog::Accepted == d.exec())
    item->ing = d.ingredient();
}

void Recipe::addStep(void) {
  bool ok = false;
  QString string = QInputDialog::getMultiLineText(
    _steps, "Input", "Input steps details", "", &ok);
  if (ok && !string.isEmpty())  addStep(string);
}

void Recipe::addStep(const QString &text) {
  _steps->addItem(new StepListItem(text));
}

void Recipe::editStep(void) {
  auto item = static_cast<StepListItem*>(_steps->currentItem());
  QString step = item->step();
  bool ok = false;
  step = QInputDialog::getMultiLineText(
        _steps, "Update", "Edit steps details", step, &ok);
  if (ok && !step.isEmpty())  item->setStep(step);
}

double Recipe::currentRatio(void) const {
  return _portions->value() / _data->portions;
}

void Recipe::updateDisplayedPortions(void) {
  if (!isReadOnly())  return;
  double r = currentRatio();
  for (int i=0; i<_ingredients->count(); i++)
    static_cast<IngredientListItem*>(_ingredients->item(i))->setRatio(r);
  /// TODO Kind of ugly (but functional)
}

void Recipe::showSubRecipe(QListWidgetItem *li) {
  auto item = static_cast<IngredientListItem*>(li);
  if (item->ing->etype == db::EntryType::SubRecipe) {
    Recipe dsubrecipe (this);
    db::Recipe *subrecipe =
      static_cast<db::SubRecipeEntry*>(item->ing.data())->recipe;
    dsubrecipe.show(subrecipe, true, currentRatio());
  }
}

} // end of namespace gui
