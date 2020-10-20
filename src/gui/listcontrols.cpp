#include <set>

#include <QHBoxLayout>
#include <QMessageBox>

#include "listcontrols.h"

#include <QDebug>

namespace gui {

ListControls::ListControls (QAbstractItemView *view) : _view(nullptr) {
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

  connect(_del, &QToolButton::clicked, this, &ListControls::deleteSelection);

  if (view)  setView(view);

  setState();
}

void ListControls::setView (QAbstractItemView *view) {
  if (_view && _view->selectionModel())
    _view->selectionModel()->disconnect(this);
  _view = view;
  if (_view && _view->selectionModel())
    connect(_view->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &ListControls::setState);
}

void ListControls::setState(void) {
  bool hasSelection = _view->currentIndex().isValid();
  _edit->setEnabled(hasSelection);
  _del->setEnabled(hasSelection);
}

void ListControls::deleteSelection(void) {
  if (QMessageBox::question(this, "Confirmez", "Supprimer?")
      != QMessageBox::Yes)  return;

//  qDeleteAll(_view->selectedItems());
  std::set<int> selectedRows;
  for (const auto &i: _view->selectionModel()->selectedIndexes())
    selectedRows.insert(i.row());

  qDebug() << selectedRows.size() << "selected rows to delete";
  for (auto i: selectedRows) {
    qDebug() << "Please delete row" << i << "in" << _view->model();
    _view->model()->removeRow(i);
  }
}

} // end of namespace gui
