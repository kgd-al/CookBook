#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QPushButton>

#include "ingrediententrydialog.h"
#include "common.h"

#include "../db/book.h"

#include <QTimeLine>
#include <QDebug>

namespace gui {

static const int NoIndex = -1;

IngredientDialog::IngredientDialog (QWidget *parent, const QString &title)
  : QDialog (parent) {
  setWindowTitle(title);

  db::Book &book = db::Book::current();

  QVBoxLayout *layout = new QVBoxLayout;

  QHBoxLayout *etsLayout = new QHBoxLayout;
  etsLayout->addWidget(new QLabel ("Type d'entrée: "));
  etsLayout->addWidget(entryTypeSelection = new QComboBox, 1);
  layout->addLayout(etsLayout);
  for (const auto &k: entryTypeDesc.keys())
      entryTypeSelection->addItem(entryTypeDesc.value(k), int(k));

  entryLayout = new QStackedLayout;
  layout->addLayout(entryLayout);

  // Ingredient
  QWidget *iholder = new QWidget;
  QGridLayout *ilayout = new QGridLayout;
  int r = 0, c = 0;
  const auto addCell =
    [ilayout, &r, &c] (QWidget *w) { ilayout->addWidget(w, r, c++); };

  addCell(new QLabel ("Quantité"));
  addCell(new QLabel ("Unité"));
  addCell(new QLabel ("Type"));
  addCell(new QLabel ("(Adjectif)"));
  addCell(new QLabel ("Groupe"));
  r++; c = 0;
  addCell(amount = new QLineEdit);
  addCell(unit = new AutoFilterComboBox);
  addCell(type = new AutoFilterComboBox);
  addCell(qualif = new QLineEdit);
  addCell(group = new AutoFilterComboBox (QComboBox::NoInsert));

  iholder->setLayout(ilayout);
  entryLayout->addWidget(iholder);

  auto unitModel = new QSortFilterProxyModel(this);
  unitModel->setSourceModel(&book.units);
  unitModel->sort(1, Qt::DescendingOrder);
  unit->setModel(unitModel);
  unit->setCurrentIndex(NoIndex);
  unit->setEmptyAlias(db::IngredientData::NoUnit);

  auto typeModel = new QSortFilterProxyModel(this);
  typeModel->setSourceModel(&book.ingredients);
  typeModel->sort(1, Qt::DescendingOrder);
  type->setModel(typeModel);
  type->setCurrentIndex(NoIndex);
  connect(type, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &IngredientDialog::typeChanged);

  group->setModel(db::getStaticModel<db::AlimentaryGroupData>());
  group->setCurrentIndex(NoIndex);

  // Sub-recipe
  QWidget *rholder = new QWidget;
  QVBoxLayout *rlayout = new QVBoxLayout;
  rlayout->addWidget(new QLabel ("Sélectionnez une recette"));
  rlayout->addWidget(recipe = new AutoFilterComboBox);
  recipe->setModel(&db::Book::current().recipes);
  recipe->setModelColumn(db::RecipesModel::titleColumn());
  recipe->setCurrentIndex(-1);
  rholder->setLayout(rlayout);
  entryLayout->addWidget(rholder);

  QWidget *dholder = new QWidget;
  QVBoxLayout *dlayout = new QVBoxLayout;
  dlayout->addWidget(new QLabel ("Saisissez du text"));
  dlayout->addWidget(decoration = new QLineEdit);
  dholder->setLayout(dlayout);
  entryLayout->addWidget(dholder);

  auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                     | QDialogButtonBox::Cancel);
  layout->addWidget(buttons);
  auto *decoy = buttons->addButton("", QDialogButtonBox::ActionRole);
  decoy->setDefault(true);
  decoy->hide();

  connect(buttons, &QDialogButtonBox::accepted,
          this, &IngredientDialog::validate);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

  validator.setBottom(0);
  validator.setDecimals(2);
  amount->setValidator(&validator);

  QVector<QWidget*> torder { type, amount, unit, group };
  for (int i=0; i<torder.size()-1; i++)
    setTabOrder(torder[i], torder[i+1]);

  setLayout(layout);

  connect(entryTypeSelection,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &IngredientDialog::updateLayout);

  gui::restoreGeometry(this);
}

void IngredientDialog::typeChanged(void) {
  qDebug() << "New type: " << type->currentText();
  auto &book = db::Book::current();
  auto id = db::ID(type->currentData(db::IDRole).toInt());
  db::IngredientData &d = book.ingredients.at(id);
  bool isNew = (d.group == nullptr);
  if (!isNew) {
    group->setCurrentIndex(group->findText(d.group->text));
    qDebug() << group->currentIndex() << "=" << group->findText(d.group->text)
             << "= find(" << d.group->text << ")";
  }
  group->setEnabled(isNew);
}

bool IngredientDialog::validate(void) {
  bool ok = true;
  const auto error = [&ok] (QWidget *w) {
    ok &= false;
    QTimeLine *timeline = new QTimeLine;
    timeline->setEasingCurve(QEasingCurve::Linear);
    timeline->setFrameRange(0, 6);
    connect(timeline, &QTimeLine::frameChanged,
            [timeline, w] (int i) {
      if (i%2)
        w->setStyleSheet("background: 1px solid red");
      else
        w->setStyleSheet("");
    });
    connect(timeline, &QTimeLine::finished,
            timeline, &QTimeLine::deleteLater);
    timeline->start();
  };

  auto q = qDebug().nospace();

  db::EntryType t = entryType();
  switch (t) {
  case db::EntryType::Ingredient:
    q << "Validating ingredient:\n"
      << amount->text().toDouble() << "\n"
      << "(" << unit->currentIndex() << ") " << unit->currentText() << "\n"
      << "(" << type->currentIndex() << ") " << type->currentText() << "\n"
      << "    " << qualif->text() << "\n"
      << "(" << group->currentIndex() << ") " << group->currentText() << "\n";

    db::Book::current().ingredients.validateTemporaryData({
      db::ID(type->currentData(db::IDRole).toInt())});

    if (amount->text().toDouble() <= 0) error(amount);
    // unit can be empty
    if (type->currentIndex() == NoIndex) error(type);
    // qualif can be empty
    if (group->currentIndex() == NoIndex)  error(group);
    break;

  case db::EntryType::SubRecipe:
    if (recipe->currentIndex() == NoIndex) error(recipe);
    break;

  case db::EntryType::Decoration:
    if (decoration->text().isEmpty()) error(decoration);
    break;
  }

  q << ">> ok: " << ok << "\n";

  if (ok) accept();
  return ok;
}

void IngredientDialog::setIngredient (const Ingredient_ptr &e) {
  qDebug() << "Setting current layout to " << int(e->etype);
  int index = int(e->etype);
  entryTypeSelection->setCurrentIndex(index);
  entryLayout->setCurrentIndex(index);
  qDebug() << ">>" << entryLayout->currentIndex();
  switch (e->etype) {
  case db::EntryType::Ingredient: {
    auto &i = dynamic_cast<const db::IngredientEntry&>(*e.data());
    amount->setText(QString::number(i.amount));
    unit->setCurrentText(i.unit->text);
    type->setCurrentIndex(type->findText(i.idata->text));
    qualif->setText(i.qualif);
    group->setCurrentText(i.idata->group->text);
    typeChanged();
    break;
  }
  case db::EntryType::SubRecipe: {
    auto &r = dynamic_cast<const db::SubRecipeEntry&>(*e.data());
    recipe->setCurrentText(r.recipe->title);
    break;
  }
  case db::EntryType::Decoration: {
    auto &d = dynamic_cast<const db::DecorationEntry&>(*e.data());
    decoration->setText(d.text);
    break;
  }
  }
}

IngredientDialog::Ingredient_ptr IngredientDialog::ingredient (void) const {
  db::EntryType t = entryType();
  Ingredient_ptr::pointer ptr = nullptr;
  switch (t) {
  case db::EntryType::Ingredient: {
    qDebug() << "Processing ingredient:\n"
             << type->currentIndex();

    db::Book &book = db::Book::current();
    static const auto id = [] (QComboBox *cb) {
      return db::ID(cb->currentData(db::IDRole).toInt());
    };

    db::IngredientData &d = book.ingredients.at(id(type));

    if (!d.group)
      d.group = &db::at<db::AlimentaryGroupData>(id(group));

    if (unit->currentIndex() == NoIndex)
      unit->setCurrentIndex(unit->findText(db::IngredientData::NoUnit));
    auto &u_data = book.units.at(id(unit));

    auto q = qDebug().nospace();
    q << " type: " << type->currentData(db::IDRole) << "\n"
      << "group: " << group->currentData(db::IDRole) << "\n"
      << " unit: " << unit->currentData(db::IDRole) << "\n";

    ptr = new db::IngredientEntry (amount->text().toDouble(), &u_data, &d,
                                   qualif->text());

    break;
  }

  case db::EntryType::SubRecipe:
    ptr = new db::SubRecipeEntry (
      &static_cast<db::RecipesModel*>(
        recipe->model())->at(db::ID(recipe->currentData(db::IDRole).toInt())));
    break;

  case db::EntryType::Decoration:
    ptr = new db::DecorationEntry (decoration->text());
    break;
  }
  return Ingredient_ptr(ptr);
}

db::EntryType IngredientDialog::entryType (void) const {
  auto i = entryTypeSelection->currentData().toInt();
  Q_ASSERT(0 <= i && i <= 2);
  return db::EntryType(i);
}

void IngredientDialog::updateLayout (void) {
  entryLayout->setCurrentIndex(int(entryType()));
}

void IngredientDialog::closeEvent(QCloseEvent *e) {
  gui::saveGeometry(this);
  QDialog::closeEvent(e);
}

const QMap<db::EntryType, QString>
IngredientDialog::entryTypeDesc {
{ db::EntryType::Ingredient, "Ingrédient"   },
{  db::EntryType::SubRecipe, "Sous-recette" },
{ db::EntryType::Decoration, "Décoration"   }
};

} // end of namespace gui
