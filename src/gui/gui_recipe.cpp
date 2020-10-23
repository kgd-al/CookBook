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
#include "common.h"

#include "../db/book.h"
#include "../db/recipesmodel.h"

namespace gui {

QWidget* spacer (void) {
  auto w = new QWidget;
  w->setMinimumHeight(10);
  return w;
}

/// TODO Make it work and make it pretty
struct StrangeWidget : public QLineEdit {
  bool edit = false;
  StrangeWidget (const QString &s) : QLineEdit(s) {}
  void setEdit (bool e) {
    edit = e;
//    setFrame(edit);
    setReadOnly(!edit);
//    setSizePolicy(edit ? QSizePolicy::MinimumExpanding : QSizePolicy::Fixed,
//                  QSizePolicy::Fixed);
//    setAutoFillBackground(edit);
    updateGeometry();
    update();
  }
};

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
      return "Étape " + QString::number(listWidget()->row(this)+1) + ":\n"
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

struct ObstinateCB : public QComboBox {
  bool readOnly;
  bool event (QEvent *event) {
    if (!readOnly)  return QComboBox::event(event);
    else
      switch (event->type()) {
      case QEvent::MouseButtonPress:
      case QEvent::Wheel:
      case QEvent::FocusIn:
      case QEvent::FocusOut:
      case QEvent::HoverEnter:
      case QEvent::HoverLeave:
      case QEvent::HoverMove:
        return false;
      default:
        return QComboBox::event(event);
      }
    return false;
  }
};

Recipe::Recipe(QWidget *parent) : QDialog(parent) {
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->setSizeConstraint(QLayout::SetNoConstraint);

    _title = new StrangeWidget ("Sans titre");
    _title->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(_title, 0, Qt::AlignCenter);

    QGridLayout *dlayout = new QGridLayout; {
      dlayout->addWidget(_references = new QLabel(""));

      int r = 0, c = 0;
      dlayout->addWidget(new QLabel (tr("Régime")),
                         r, c++, 1, 1, Qt::AlignCenter);
      dlayout->addWidget(new QLabel (tr("Status")),
                         r, c++, 1, 1, Qt::AlignCenter);
      dlayout->addWidget(new QLabel (tr("Type")),
                         r, c++, 1, 1, Qt::AlignCenter);
      dlayout->addWidget(new QLabel (tr("Durée")),
                         r, c++, 1, 1, Qt::AlignCenter);
      r++; c = 0;
      dlayout->addWidget(_regimen = new ObstinateCB, r, c++, Qt::AlignCenter);
      _regimen->setModel(db::getStaticModel<db::RegimenData>());
      dlayout->addWidget(_status = new ObstinateCB, r, c++, Qt::AlignCenter);
      _status->setModel(db::getStaticModel<db::StatusData>());
      _status->setStyleSheet(":disabled {background-color:#ff0000;}");
      dlayout->addWidget(_type = new ObstinateCB, r, c++, Qt::AlignCenter);
      _type->setModel(db::getStaticModel<db::DishTypeData>());
      dlayout->addWidget(_duration = new ObstinateCB, r, c++, Qt::AlignCenter);
      _duration->setModel(db::getStaticModel<db::DurationData>());

      r++; c = 0;
      dlayout->addWidget(spacer(), r, c, 1, 3);

    } mainLayout->addLayout(dlayout);

    mainLayout->addWidget(spacer());
    QHBoxLayout *midLayout = new QHBoxLayout;
      QVBoxLayout *ilayout = new QVBoxLayout;
        ilayout->addWidget(new QLabel ("Ingrédients:"));
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
      slayout->addWidget(new QLabel("Étapes:"));
      _steps = new GUIList;
      _steps->setAcceptDrops(true);
      _steps->setDragDropMode(QAbstractItemView::InternalMove);
      _steps->setWordWrap(true);
      slayout->addWidget(_steps);
      slayout->addWidget(_scontrols = new ListControls(_steps));
    mainLayout->addLayout(slayout);

    QDialogButtonBox *buttons = new QDialogButtonBox;
      auto del = new QToolButton();
      buttons->addButton(del, QDialogButtonBox::DestructiveRole);
      connect(del, &QToolButton::clicked, this, &Recipe::deleteRequested);

      auto close = buttons->addButton("Quitter", QDialogButtonBox::RejectRole);
      connect(close, &QPushButton::clicked, this, &Recipe::close);

      _toggle = buttons->addButton("", QDialogButtonBox::ActionRole);
      connect(_toggle, &QPushButton::clicked, this, &Recipe::toggleReadOnly);
    mainLayout->addWidget(buttons);

  _icontrols->addButton()->setShortcut(QKeySequence("Ctrl+I"));
  connect(_icontrols->addButton(), &QToolButton::clicked,
          this, qOverload<>(&Recipe::addIngredient));

  _icontrols->editButton()->setShortcut(QKeySequence("Ctrl+Shift+I"));
  connect(_icontrols->editButton(), &QToolButton::clicked,
          this, &Recipe::editIngredient);

  _scontrols->addButton()->setShortcut(QKeySequence("Ctrl+E"));
  connect(_scontrols->addButton(), &QToolButton::clicked,
          this, qOverload<>(&Recipe::addStep));

  _scontrols->editButton()->setShortcut(QKeySequence("Ctrl+Shift+E"));
  connect(_scontrols->editButton(), &QToolButton::clicked,
          this, &Recipe::editStep);

  connect(_portions, qOverload<double>(&QDoubleSpinBox::valueChanged),
          this, qOverload<>(&Recipe::updateDisplayedPortions));

  connect(_ingredients, &QListWidget::itemActivated,
          this, &Recipe::showSubRecipe);

  _data = nullptr;
  setLayout(mainLayout);
  setReadOnly(false);

  gui::restoreGeometry(this);
}

int Recipe::show (db::Recipe *recipe, bool readOnly, double ratio) {
  _data = recipe;

  _title->setText(_data->title);

  if (_data->used > 0) {
    QString t = QString::number(_data->used) + " reference";
    if (_data->used > 1)  t += "s";
    _references->setText(t);
  } else
    _references->setText("");
  _references->setVisible(_data->used > 0);

  _regimen->setCurrentIndex(_data->regimen->id-1);
  _status->setCurrentIndex(_data->status->id-1);
  _type->setCurrentIndex(_data->type->id-1);
  _duration->setCurrentIndex(_data->duration->id-1);

  for (const db::Recipe::Ingredient_ptr &i: recipe->ingredients) addIngredient(i);
  for (const QString &s: recipe->steps)  addStep(s);

  _displayedPortions = ratio * _data->portions;
  _portions->setValue(_data->portions);
  _portionsLabel->setText(_data->portionsLabel);

  _notes->setText(_data->notes);

  setReadOnly(readOnly);

  return exec();
}

template <typename T>
const T* getData (const QComboBox *cb) {
  return &db::at<T>(db::ID(cb->currentData(db::IDRole).toInt()));
}

void Recipe::writeThrough(void) {
  _data->title = _title->text();

  _data->regimen = getData<db::RegimenData>(_regimen);
  _data->status = getData<db::StatusData>(_status);
  _data->type = getData<db::DishTypeData>(_type);
  _data->duration = getData<db::DurationData>(_duration);

  // Make a copy for usage comparison
  decltype(_data->ingredients) newIngredients;
  for (int i=0; i<_ingredients->count(); i++)
    newIngredients.append(
      static_cast<const IngredientListItem*>(_ingredients->item(i))->ing);
  _data->updateUsageCounts(newIngredients);
  _data->ingredients = newIngredients;

  _data->steps.clear();
  for (int i=0; i<_steps->count(); i++)
    _data->steps.append(
      static_cast<const StepListItem*>(_steps->item(i))->step());

  _data->portions = _displayedPortions = _portions->value();
  _data->portionsLabel = _portionsLabel->text();

  _data->notes = _notes->toPlainText();

  emit validated();
}

void Recipe::setReadOnly(bool ro) {
  _readOnly = ro;

//  _title->setEnabled(!ro);
  static_cast<StrangeWidget*>(_title)->setEdit(!ro);

  static_cast<ObstinateCB*>(_regimen)->readOnly = ro;
  static_cast<ObstinateCB*>(_status)->readOnly = ro;
  static_cast<ObstinateCB*>(_type)->readOnly = ro;
  static_cast<ObstinateCB*>(_duration)->readOnly = ro;

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

  if (isReadOnly()) _toggle->setText("Modifier");
  else              _toggle->setText("Valider");

  setWindowTitle(ro ? "Consultation" : "Édition");
}

void Recipe::toggleReadOnly(void) {
  if (!isReadOnly() && !confirmed()) return;
  setReadOnly(!isReadOnly());
  updateDisplayedPortions(false);
}

bool Recipe::confirmed(void) {
  if (QMessageBox::question(this, tr("Valider?"), "Confirmez la modification?",
                            QMessageBox::Yes | QMessageBox::No)
      == QMessageBox::Yes) {
    writeThrough();
    return true;
  }
  return false;
}

void Recipe::addIngredient(void) {
  IngredientDialog d (this, "Nouvel ingrédient");
  if (QDialog::Accepted == d.exec())
    addIngredient(d.ingredient());
}

void Recipe::addIngredient(Ingredient_ptr i) {
  auto item = new IngredientListItem(i);
  _ingredients->addItem(item);
  if (_ingredients->currentItem() == nullptr)
    _ingredients->setCurrentItem(item);
}

void Recipe::editIngredient(void) {
  auto item = static_cast<IngredientListItem*>(_ingredients->currentItem());
  auto ing = item->ing;

  IngredientDialog d (this, "Mise à jour");
  d.setIngredient(ing);

  if (QDialog::Accepted == d.exec())
    item->ing = d.ingredient();
}

void Recipe::addStep(void) {
  bool ok = false;
  QString string = QInputDialog::getMultiLineText(
    _steps, "Saisissez", "Nouvelle étape", "", &ok);
  if (ok && !string.isEmpty())  addStep(string);
}

void Recipe::addStep(const QString &text) {
  auto item = new StepListItem(text);
  _steps->addItem(item);
  if (_steps->currentItem() == nullptr) _steps->setCurrentItem(item);
}

void Recipe::editStep(void) {
  auto item = static_cast<StepListItem*>(_steps->currentItem());
  QString step = item->step();
  bool ok = false;
  step = QInputDialog::getMultiLineText(
        _steps, "Saisissez", "Mise à jour", step, &ok);
  if (ok && !step.isEmpty())  item->setStep(step);
}

double Recipe::currentRatio(void) const {
  return _portions->value() / _data->portions;
}

void Recipe::updateDisplayedPortions(bool spontaneous) {
//  qDebug() << __PRETTY_FUNCTION__ << ": ratio = "
//           << currentRatio() << " = " << _portions->value()
//           << " / " << _data->portions;
  if (!isReadOnly() && spontaneous)  return;
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
    /// FIXME ugly const cast
  }
}

void Recipe::keyPressEvent(QKeyEvent *e) {
//  qDebug() << __PRETTY_FUNCTION__ << "(" << e << ");";
  static const QList<int> commit_keys {
    /*Qt::Key_Return, Qt::Key_Enter, */Qt::Key_Escape
  };
  if (commit_keys.contains(e->key()) && !safeQuit(e)) return;
  QDialog::keyPressEvent(e);
}

void Recipe::closeEvent(QCloseEvent *e) {
//  qDebug() << __PRETTY_FUNCTION__ << "(" << e << ");";
  safeQuit(e);
  gui::saveGeometry(this);
}

bool Recipe::safeQuit(QEvent *e) {
//  qDebug() << __PRETTY_FUNCTION__ << "(" << e << ");";
  if (isReadOnly()) return true;

  auto ret = QMessageBox::warning(this, "Confirmation",
                                  "Voulez vous sauvegarder?",
                                  QMessageBox::Yes,
                                  QMessageBox::No,
                                  QMessageBox::Cancel);
  switch (ret) {
  case QMessageBox::Yes:
    writeThrough();
    return true;
  case QMessageBox::No:
    e->accept();
    return true;
    break;
  case QMessageBox::Cancel:
  default:
    e->ignore();
    return false;
  }
}

void Recipe::deleteRequested(void) {
  if (_data->used > 0) {
    QMessageBox::warning(this, "Illégal",
                         "Cette recette est référencée par d'autre. La"
                         " suppression n'est pas autorisée!",
                         QMessageBox::Ok);
    return;
  }

  if (QMessageBox::warning(this, "Supprimer?",
                           "Voulez vous definitivement supprimer '"
                           + _data->title + "' ?",
                           QMessageBox::Yes | QMessageBox::No)
      != QMessageBox::Yes)
    return;

  db::Book &book = db::Book::current();

  _data->updateUsageCounts();
  book.recipes.delRecipe(_data);

  emit deleted();

  reject();
}

} // end of namespace gui
