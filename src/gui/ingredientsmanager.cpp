#include <QVBoxLayout>
#include <QTableView>
#include <QHeaderView>

#include <QComboBox>

#include "ingredientsmanager.h"
#include "../db/book.h"

namespace gui {

IngredientsManager::IngredientsManager(QWidget *parent)
  : QDialog(parent) {

  setWindowTitle("Gestionnaire d'ingrédients");

  QVBoxLayout *layout = new QVBoxLayout;
  QTableView *table = new QTableView;
  QComboBox *cbox = new QComboBox;

  layout->addWidget(table);
  layout->addWidget(cbox);
  setLayout(layout);

  table->setModel(&db::Book::current().ingredients);
  cbox->setModel(&db::Book::current().ingredients);

  table->verticalHeader()->hide();
}

} // end of namespace gui
