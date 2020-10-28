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
struct LabelEdit : public QWidget {
  bool edit = false;
  QLabel *label;
  QLineEdit *ledit;
  LabelEdit (const QString &text, bool edit = false) {
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(label = new QLabel, 0, 0);
    layout->addWidget(ledit = new QLineEdit, 0, 0);
    setLayout(layout);
    setText(text);
    setEdit(edit);
  }

  void setFontRelativeSize (double r) {
    QFont f = font();
    auto q = qDebug();
    q << r << "*" << f.pointSizeF();
    f.setPointSizeF(r * f.pointSizeF());
    q << " >> " << f.pointSizeF();
    label->setFont(f);
    ledit->setFont(f);
  }

  void setAlignment (Qt::AlignmentFlag flag) {
    label->setAlignment(flag);
    ledit->setAlignment(flag);
  }

  void setEdit (bool e) {
    edit = e;
    label->setVisible(!edit);
    ledit->setVisible(edit);
    if (e)
      ledit->setText(label->text());
    else
      label->setText(ledit->text());
  }

  void setText (const QString &text) {
    if (edit)
      ledit->setText(text);
    else
      label->setText(text);
  }

  QString text (void) const {
    if (edit)
      return ledit->text();
    else
      return label->text();
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

Recipe::Recipe(QWidget *parent) : QDialog(parent) {
  static constexpr auto C = Qt::AlignCenter;
  QVBoxLayout *mainLayout = new QVBoxLayout;
  mainLayout->setSizeConstraint(QLayout::SetNoConstraint);

    _title = new LabelEdit ("Sans titre");
    _title->setAlignment(C);
    _title->setFontRelativeSize(1.5);
    mainLayout->addWidget(_title);

    _consult.holder = new QWidget;
    QGridLayout *clayout = new QGridLayout; {

      bool tightIcons = false;
      int r = 0, c = 0;
      if (tightIcons) {
        clayout->addWidget(new QWidget, r, c++);
        clayout->setColumnStretch(0, 1);
      }
      for (QLabel ** l: { &_consult.subrecipe, &_consult.basic,
                          &_consult.regimen, &_consult.status, &_consult.type,
                          &_consult.duration })
        clayout->addWidget(*l = new QLabel, r, c++, C);
      if (tightIcons) {
        clayout->addWidget(new QWidget, r, c);
        clayout->setColumnStretch(c, 1);
      }

    }
    _consult.holder->setLayout(clayout);
    mainLayout->addWidget(_consult.holder);

    _edit.holder = new QWidget;
    QGridLayout *elayout = new QGridLayout; {

      int r = 0, c = 0;
      elayout->addWidget(new QLabel (tr("Références")),
                         r, c++, 1, 1, C);
      elayout->addWidget(new QLabel (tr("Basique")),
                         r, c++, 1, 1, C);
      elayout->addWidget(new QLabel (tr("Régime")),
                         r, c++, 1, 1, C);
      elayout->addWidget(new QLabel (tr("Status")),
                         r, c++, 1, 1, C);
      elayout->addWidget(new QLabel (tr("Type")),
                         r, c++, 1, 1, C);
      elayout->addWidget(new QLabel (tr("Durée")),
                         r, c++, 1, 1, C);
      r++; c = 0;
      elayout->addWidget(_edit.subrecipe = new QLabel, r, c++, C);
      elayout->addWidget(_edit.basic = new QCheckBox, r, c++, C);
      elayout->addWidget(_edit.regimen = new QComboBox, r, c++, C);
      _edit.regimen->setModel(db::getStaticModel<db::RegimenData>());
      elayout->addWidget(_edit.status = new QComboBox, r, c++, C);
      _edit.status->setModel(db::getStaticModel<db::StatusData>());
      elayout->addWidget(_edit.type = new QComboBox, r, c++, C);
      _edit.type->setModel(db::getStaticModel<db::DishTypeData>());
      elayout->addWidget(_edit.duration = new QComboBox, r, c++, C);
      _edit.duration->setModel(db::getStaticModel<db::DurationData>());

      r++; c = 0;
      elayout->addWidget(spacer(), r, c, 1, 3);

    }
    _edit.holder->setLayout(elayout);
    mainLayout->addWidget(_edit.holder);

//    mainLayout->addWidget(spacer());
    _vsplitter = new QSplitter (Qt::Vertical);
      _hsplitter = new QSplitter;
        QWidget *iholder = new QWidget;
        QVBoxLayout *ilayout = new QVBoxLayout;
          ilayout->addWidget(new QLabel ("Ingrédients:"));
          QHBoxLayout *playout = new QHBoxLayout;
            _portions = new QDoubleSpinBox;
            _portions->setMinimum(0);
            _portions->setSingleStep(.25);
            playout->addWidget(_portions);
            _portions->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            playout->addWidget(_portionsLabel = new LabelEdit("?"));
          ilayout->addLayout(playout);
          _ingredients = new GUIList;
          _ingredients->setAcceptDrops(true);
          _ingredients->setDragDropMode(QAbstractItemView::InternalMove);
          _ingredients->setWordWrap(true);
          _ingredients->setMinimumWidth(100);
          ilayout->addWidget(_ingredients);
          ilayout->addWidget(_icontrols = new ListControls(_ingredients));
        iholder->setLayout(ilayout);
        _hsplitter->addWidget(iholder);

        QWidget *nholder = new QWidget;
        QVBoxLayout *nlayout = new QVBoxLayout;
          nlayout->addWidget(new QLabel ("Notes:"));
          _notes = new QTextEdit;
          _notes->setMinimumWidth(100);
          nlayout->addWidget(_notes);
        nholder->setLayout(nlayout);
        _hsplitter->addWidget(nholder);
      _vsplitter->addWidget(_hsplitter);

      mainLayout->addWidget(spacer());
      QWidget *sholder = new QWidget;
      QVBoxLayout *slayout = new QVBoxLayout;
        slayout->addWidget(new QLabel("Étapes:"));
        _steps = new GUIList;
        _steps->setAcceptDrops(true);
        _steps->setDragDropMode(QAbstractItemView::InternalMove);
        _steps->setMinimumWidth(100);
        _steps->setWordWrap(true);
        slayout->addWidget(_steps);
        slayout->addWidget(_scontrols = new ListControls(_steps));
      sholder->setLayout(slayout);
      _vsplitter->addWidget(sholder);
    mainLayout->addWidget(_vsplitter);

    QHBoxLayout *blayout = new QHBoxLayout;
      auto del = new QToolButton();
      del->setIcon(QIcon::fromTheme("edit-delete"));
      blayout->addWidget(del);
      connect(del, &QToolButton::clicked, this, &Recipe::deleteRequested);

      QDialogButtonBox *buttons = new QDialogButtonBox;

        _apply = buttons->addButton("Appliquer", QDialogButtonBox::AcceptRole);
        connect(_apply, &QPushButton::clicked, this, &Recipe::apply);

        auto close = buttons->addButton("Quitter", QDialogButtonBox::RejectRole);
        connect(close, &QPushButton::clicked, this, &Recipe::close);

        _toggle = buttons->addButton("", QDialogButtonBox::ActionRole);
        connect(_toggle, &QPushButton::clicked, this, &Recipe::toggleReadOnly);
      blayout->addWidget(buttons);
    mainLayout->addLayout(blayout);

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
  auto &settings = localSettings(this);
  _hsplitter->setSizes(settings.value("hsplitter").value<QList<int>>());
  _vsplitter->setSizes(settings.value("vsplitter").value<QList<int>>());
}

int Recipe::show (db::Recipe *recipe, bool readOnly, double ratio) {
  _data = recipe;

  _title->setText(_data->title);

  for (const auto &i: recipe->ingredients) addIngredient(i);
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

  _data->regimen = getData<db::RegimenData>(_edit.regimen);
  _data->status = getData<db::StatusData>(_edit.status);
  _data->type = getData<db::DishTypeData>(_edit.type);
  _data->duration = getData<db::DurationData>(_edit.duration);
  _data->basic = _edit.basic->isChecked();

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

void pixmap (QLabel *l, const QIcon &icon) {
  l->setPixmap(icon.pixmap(2*db::iconSize(), 2*db::iconSize()));
}

void maybePixmap (QLabel *l, bool yes, const QIcon &icon) {
  pixmap(l, yes ? icon : QIcon());
}

void Recipe::setReadOnly(bool ro) {
  _readOnly = ro;

  _title->setEdit(!ro);

  if (_data) {
    if (ro) { // copy from edit to consult
      maybePixmap(_consult.subrecipe, _data->used, db::MiscIcons::sub_recipe());
      maybePixmap(_consult.basic, _data->basic, db::MiscIcons::basic_recipe());
      pixmap(_consult.regimen, _data->regimen->decoration);
      pixmap(_consult.status, _data->status->decoration);
      pixmap(_consult.type, _data->type->decoration);
      pixmap(_consult.duration, _data->duration->decoration);

    } else {  // copy from consult to edit
      _edit.subrecipe->setText(QString::number(_data->used));
      _edit.basic->setChecked(_data->basic);
      _edit.regimen->setCurrentIndex(_data->regimen->id-1);
      _edit.status->setCurrentIndex(_data->status->id-1);
      _edit.type->setCurrentIndex(_data->type->id-1);
      _edit.duration->setCurrentIndex(_data->duration->id-1);
    }
  }

  _consult.holder->setVisible(ro);
  _edit.holder->setVisible(!ro);

  _notes->setReadOnly(ro);

  if (!ro) {
    _displayedPortions = _portions->value();
    if (_data)  _portions->setValue(_data->portions);
  } else {
    _portions->setValue(_displayedPortions);
  }
  _portionsLabel->setEdit(!ro);

  _ingredients->setDragEnabled(!ro);
  _icontrols->setVisible(!ro);
  _steps->setDragEnabled(!ro);
  _scontrols->setVisible(!ro);

  if (ro) _toggle->setText("Modifier");
  else    _toggle->setText("Valider");
  _apply->setVisible(!ro);

  setWindowTitle(ro ? "Consultation" : "Édition");
}

void Recipe::toggleReadOnly(void) {
  if (!isReadOnly() && !confirmed()) return;
  setReadOnly(!isReadOnly());
  updateDisplayedPortions(false);
}

void Recipe::apply(void) {
  if (confirmed())  accept();
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
  auto &settings = localSettings(this);
  settings.setValue("vsplitter", QVariant::fromValue(_vsplitter->sizes()));
  settings.setValue("hsplitter", QVariant::fromValue(_hsplitter->sizes()));
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
