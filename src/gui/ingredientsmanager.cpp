#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QPainter>

#include <QComboBox>
//#include <QDebug>

#include "ingredientsmanager.h"
#include "../db/book.h"
#include "common.h"

namespace gui {

IngredientsManager::IngredientsManager(QWidget *parent)
  : QDialog(parent) {

  setWindowTitle("Gestionnaire d'ingrédients");

  QGridLayout *layout = new QGridLayout;
    QTableView *ltable = new QTableView;
    layout->addWidget(ltable, 0, 0);

    QComboBox *lcbox = new QComboBox;
    layout->addWidget(lcbox, 1, 0);

    QTableView *rtable = new QTableView;
    layout->addWidget(rtable, 0, 1);

    QComboBox *rcbox = new QComboBox;
    layout->addWidget(rcbox, 1, 1);
  setLayout(layout);

  auto isorter = new QSortFilterProxyModel (this);
  isorter->setSourceModel(&db::Book::current().ingredients);
  ltable->setModel(isorter);
  ltable->verticalHeader()->hide();
  ltable->setSortingEnabled(true);
  ltable->sortByColumn(1, Qt::DescendingOrder);
  auto lheader = ltable->horizontalHeader();
  lheader->setSectionResizeMode(0, QHeaderView::Stretch);
  lheader->setSectionResizeMode(1, QHeaderView::ResizeToContents);

  lcbox->setModel(&db::Book::current().ingredients);

  auto usorter = new QSortFilterProxyModel (this);
  usorter->setSourceModel(&db::Book::current().units);
  rtable->setModel(usorter);
  rtable->verticalHeader()->hide();
  rtable->setSortingEnabled(true);
  rtable->sortByColumn(1, Qt::DescendingOrder);
  auto rheader = rtable->horizontalHeader();
  rheader->setSectionResizeMode(0, QHeaderView::Stretch);
  rheader->setSectionResizeMode(1, QHeaderView::ResizeToContents);

  rcbox->setModel(&db::Book::current().units);

  auto &settings = localSettings(this);
  setGeometry(settings.value("geometry").toRect());
}

void IngredientsManager::closeEvent(QCloseEvent *e) {
  auto &settings = localSettings(this);
  settings.setValue("geometry", geometry());
  QDialog::closeEvent(e);
}

} // end of namespace gui
