#include <QVBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QApplication>
#include <QTimer>
#include <QMenu>
#include <QMimeData>
#include <QToolButton>

#include "planningview.h"
#include "../db/book.h"

#include <QDebug>

namespace gui {

struct CustomTableView : public QTableView {
  QMenu *_contextMenu;

  enum Action { ADD, OVERWRITE, DELETE };
  QMap<Action, QAction*> actions;

  CustomTableView (void) {
    _contextMenu = new QMenu;

    const auto action = [this] (Action a, const QString &text) {
      QAction *qa = new QAction(text);
      qa->setData(a);
      actions[a] = qa;
      return qa;
    };
    action(ADD, "Combiner");
    action(OVERWRITE, "Remplacer");
    action(DELETE, "Supprimer");

    for (Action a: actions.keys())
      _contextMenu->addAction(actions.value(a));
  }

  void dropEvent(QDropEvent *e) override {
    QModelIndex index = indexAt(e->pos());
    bool notEmpty = !index.data().toString().isEmpty();
    if (notEmpty) {
      actions[ADD]->setEnabled(true);
      actions[OVERWRITE]->setEnabled(true);
      actions[DELETE]->setEnabled(false);

      QAction *action = _contextMenu->exec(mapToGlobal(e->pos()));
      if (action == nullptr)  return;

      switch (action->data().value<Action>()) {
      case ADD:
        e->setDropAction(Qt::DropAction(e->dropAction()
                                      + db::PlanningModel::MergeAction));
      case OVERWRITE:
        break;
      default:
        return;
      }
    }

    QTableView::dropEvent(e);
  }

  void contextMenuEvent(QContextMenuEvent *e) override {
    QModelIndex index = indexAt(e->pos());
    bool notEmpty = !index.data().toString().isEmpty();
    actions[ADD]->setEnabled(false);
    actions[OVERWRITE]->setEnabled(false);
    actions[DELETE]->setEnabled(notEmpty);

    QAction *action = _contextMenu->exec(e->globalPos());
    if (action == nullptr)  return;
    if (DELETE == action->data().value<Action>())
      model()->setData(index, QVariant(), db::IDRole);
//    QTableView::contextMenuEvent(e);
  }
};

PlanningView::PlanningView(QWidget *parent) : QDialog(parent) {
  QHBoxLayout *layout = new QHBoxLayout;
  layout->addWidget(_table = new CustomTableView);
  auto model = &db::Book::current().planning;
  _table->setModel(model);
  _table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  _table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  _table->setDragDropMode(QAbstractItemView::DragDrop);
  _table->setTextElideMode(Qt::ElideRight);
  _table->setWordWrap(false);

  QToolButton *clear = new QToolButton;
  clear->setIcon(QApplication::style()->standardPixmap(
                   QStyle::SP_LineEditClearButton));
  clear->setToolTip(tr("Supprimer les dates dans le passé"));
  connect(clear, &QToolButton::clicked, model, &db::PlanningModel::clearOld);
  layout->addWidget(clear);

//  connect(_table->model(), &QAbstractItemModel::modelReset,
//          this, &PlanningView::updateSize);

  layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
  setLayout(layout);
}

void PlanningView::resizeEvent(QResizeEvent *e) {
  QDialog::resizeEvent(e);
  QTimer::singleShot(0, this, &PlanningView::updateSize);
}

void PlanningView::updateSize(void) {
  int fitHeight = db::PlanningModel::ROWS * _table->rowHeight(0)
                   + _table->horizontalHeader()->height()
                   + 2 * _table->frameWidth();
  if (_table->horizontalScrollBar()->isVisible())
    fitHeight += _table->horizontalScrollBar()->height();

//  auto q = qDebug().nospace();
//  q << "Table's minimum height:\n"
//    << fitHeight << " =\n"

//    << "\t  " << db::PlanningModel::ROWS * _table->rowHeight(0)
//      << "\t(" << db::PlanningModel::ROWS << " * " << _table->rowHeight(0)
//      << ") row height" << "\n"
//    << "\t+ " << _table->horizontalHeader()->height()
//      << "\theader\n";
//  if (_table->horizontalScrollBar()->isVisible())
//    q << "\t+ " << _table->horizontalScrollBar()->height()
//      << "\tscrollbar\n";
//  q << "\t+ " << 2 * _table->frameWidth() << "\tframe\n";

  _table->setMinimumHeight(fitHeight);
  _table->setMaximumHeight(fitHeight);
}

} // end of namespace gui

Q_DECLARE_METATYPE(gui::CustomTableView::Action)
