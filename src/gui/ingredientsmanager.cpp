#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>

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

  QGridLayout *layout = new QGridLayout;
    QTableView *ltable = new QTableView;
    layout->addWidget(ltable, 0, 0);

    QHBoxLayout *linput = new QHBoxLayout;
      QLineEdit *ledit =  new QLineEdit;
      ledit->setPlaceholderText("Ingrédient");
      linput->addWidget(ledit);

      QComboBox *lbox = new QComboBox;
      for (const db::AlimentaryGroupData &d: db::AlimentaryGroupData::database){
        QPixmap p (15,15);
        p.fill(d.color);
        lbox->addItem(p, d.text, d.id);
      }
      linput->addWidget(lbox);

      auto lcontrols = new ListControls(ltable);
      linput->addWidget(lcontrols);
    layout->addLayout(linput, 1, 0);

    QComboBox *lcbox = new QComboBox;
    layout->addWidget(lcbox, 2, 0);

    QTableView *rtable = new QTableView;
    layout->addWidget(rtable, 0, 1);

    QHBoxLayout *rinput = new QHBoxLayout;
      QLineEdit *redit =  new QLineEdit;
      redit->setPlaceholderText("Unité");
      rinput->addWidget(redit);

      auto rcontrols = new ListControls(rtable);
      rinput->addWidget(rcontrols);
    layout->addLayout(rinput, 1, 1);

    QComboBox *rcbox = new QComboBox;
    layout->addWidget(rcbox, 2, 1);
  setLayout(layout);

  auto isorter = new QSortFilterProxyModel (this);
  isorter->setSourceModel(&db::Book::current().ingredients);
  ltable->setModel(isorter);
  ltable->setSelectionBehavior(QAbstractItemView::SelectRows);
  ltable->setSelectionMode(QAbstractItemView::SingleSelection);
  ltable->verticalHeader()->hide();
  ltable->setSortingEnabled(true);
  ltable->sortByColumn(1, Qt::DescendingOrder);
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
    db::Book::current().ingredients.add(ledit->text(),
                                        db::ID(lbox->currentData().toInt()));
  });

  connect(lcontrols->editButton(), &QToolButton::clicked,
          [ltable, ledit, lbox] {
    using IM = db::IngredientsModel;
    db::Book::current().ingredients.update(
      db::ID(ltable->currentIndex().data(IM::IngredientRole).toInt()),
      ledit->text(), db::ID(lbox->currentData().toInt()));
  });

  connect(ltable->selectionModel(), &QItemSelectionModel::currentRowChanged,
          [lcontrols] (const QModelIndex &current, const QModelIndex&) {
    bool unused = (current.sibling(current.row(), 1).data() == 0);
    lcontrols->delButton()->setEnabled(unused);
  });
}

void IngredientsManager::closeEvent(QCloseEvent *e) {
  auto &settings = localSettings(this);
  settings.setValue("geometry", geometry());
  QDialog::closeEvent(e);
}

} // end of namespace gui
