#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

#include "ingredientsmanager.h"
#include "listcontrols.h"
#include "common.h"

#include "../db/book.h"

namespace gui {

IngredientsManager::IngredientsManager(QWidget *parent)
  : QDialog(parent) {

  setWindowTitle("Gestionnaire d'ingrédients");

  int r = 0, c = 0;

  QGridLayout *layout = new QGridLayout;
    layout->addWidget(new QLabel(tr("Ingrédients")),
                      r++, c, 1, 1, Qt::AlignCenter);

    QTableView *ltable = new QTableView;
    layout->addWidget(ltable, r++, c);

    QHBoxLayout *linput = new QHBoxLayout;
      QLineEdit *ledit =  new QLineEdit;
      ledit->setPlaceholderText("Ingrédient");
      linput->addWidget(ledit);

      QComboBox *lbox = new QComboBox;
      lbox->setModel(db::getStaticModel<db::AlimentaryGroupData>());
      linput->addWidget(lbox);

      auto lcontrols = new ListControls(ltable);
      linput->addWidget(lcontrols);
    layout->addLayout(linput, r++, c);

    QComboBox *lcbox = new QComboBox;
    layout->addWidget(lcbox, r++, c);

  r = 0; c = 1;

    layout->addWidget(new QLabel(tr("Unités")),
                      r++, c, 1, 1, Qt::AlignCenter);

    QTableView *rtable = new QTableView;
    layout->addWidget(rtable, r++, c);

    QHBoxLayout *rinput = new QHBoxLayout;
      QLineEdit *redit =  new QLineEdit;
      redit->setPlaceholderText("Unité");
      rinput->addWidget(redit);

      auto rcontrols = new ListControls(rtable);
      rinput->addWidget(rcontrols);
    layout->addLayout(rinput, r++, c);

    QComboBox *rcbox = new QComboBox;
    layout->addWidget(rcbox, r++, c);
  setLayout(layout);

  auto isorter = new QSortFilterProxyModel (this);
  isorter->setSourceModel(&db::Book::current().ingredients);
  ltable->setModel(isorter);
  ltable->setSelectionBehavior(QAbstractItemView::SelectRows);
  ltable->setSelectionMode(QAbstractItemView::SingleSelection);
  ltable->verticalHeader()->hide();
  ltable->setSortingEnabled(true);
  ltable->sortByColumn(1, Qt::DescendingOrder);
//  ltable->setIconSize(QSize(db::iconSize(), db::iconSize()));
  auto lheader = ltable->horizontalHeader();
  lheader->setSectionResizeMode(0, QHeaderView::Stretch);
  lheader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  lcontrols->setView(ltable);

  lcbox->setModel(&db::Book::current().ingredients);

  auto usorter = new QSortFilterProxyModel (this);
  usorter->setSourceModel(&db::Book::current().units);
  rtable->setModel(usorter);
  rtable->setSelectionBehavior(QAbstractItemView::SelectRows);
  rtable->setSelectionMode(QAbstractItemView::SingleSelection);
  rtable->verticalHeader()->hide();
  rtable->setSortingEnabled(true);
  rtable->sortByColumn(1, Qt::DescendingOrder);
  auto rheader = rtable->horizontalHeader();
  rheader->setSectionResizeMode(0, QHeaderView::Stretch);
  rheader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  rcontrols->setView(rtable);

  rcbox->setModel(&db::Book::current().units);

  auto &settings = localSettings(this);
  setGeometry(settings.value("geometry").toRect());

  connect(lcontrols->addButton(), &QToolButton::clicked,
          [ledit, lbox] {
    if (ledit->text().isEmpty())  return;
    db::Book::current()
      .ingredients
      .add(ledit->text(), db::ID(lbox->currentData(db::IDRole).toInt()));
  });

  connect(lcontrols->editButton(), &QToolButton::clicked,
          [ltable, ledit, lbox] {
    db::Book::current().ingredients.update(
      db::ID(ltable->currentIndex().data(db::IDRole).toInt()),
      ledit->text(), db::ID(lbox->currentData(db::IDRole).toInt()));
  });

  connect(ltable->selectionModel(), &QItemSelectionModel::currentRowChanged,
          [lcontrols,ledit,lbox]
          (const QModelIndex &current, const QModelIndex&) {
    using namespace db;
    auto i = current.data(PtrRole).value<const IngredientData*>();
    bool unused = (i->used == 0);
    ledit->setText(i->text);
    lbox->setCurrentText(i->group->text);
    lcontrols->delButton()->setEnabled(unused);
  });

  connect(rcontrols->addButton(), &QToolButton::clicked,
          [redit] {
    if (redit->text().isEmpty())  return;
    db::Book::current().units.add(redit->text());
  });

  connect(rcontrols->editButton(), &QToolButton::clicked,
          [rtable, redit] {
    db::Book::current().units.update(
      db::ID(rtable->currentIndex().data(db::IDRole).toInt()), redit->text());
  });

  connect(rtable->selectionModel(), &QItemSelectionModel::currentRowChanged,
          [rcontrols, redit] (const QModelIndex &current, const QModelIndex&) {
    using namespace db;
    auto u = current.data(PtrRole).value<const UnitData*>();
    bool unused = (u->used == 0);
    redit->setText(u->text);
    rcontrols->delButton()->setEnabled(unused);
  });
}

void IngredientsManager::closeEvent(QCloseEvent *e) {
  auto &settings = localSettings(this);
  settings.setValue("geometry", geometry());
  QDialog::closeEvent(e);
}

} // end of namespace gui
