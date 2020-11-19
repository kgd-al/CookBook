#include <QVBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QApplication>
#include <QTimer>
#include <QMenu>
#include <QMimeData>
#include <QToolButton>
#include <QInputDialog>
#include <QJsonArray>

#include "planningview.h"
#include "gui_recipe.h"
#include "../db/book.h"

#include <QDebug>

namespace gui {

struct VerticallyConcentratedTable : public QTableView {

  void resizeEvent(QResizeEvent *e) {
    QTableView::resizeEvent(e);
    QTimer::singleShot(0, this, &VerticallyConcentratedTable::updateSize);
  }

  void updateSize(void) {
    int fitHeight = db::PlanningModel::ROWS * rowHeight(0)
                     + horizontalHeader()->height()
                     + 2 * frameWidth();
    if (horizontalScrollBar()->isVisible())
      fitHeight += horizontalScrollBar()->height();

  //  auto q = qDebug().nospace();
  //  q << "Table's minimum height:\n"
  //    << fitHeight << " =\n"

  //    << "\t  " << db::PlanningModel::ROWS * rowHeight(0)
  //      << "\t(" << db::PlanningModel::ROWS << " * " << rowHeight(0)
  //      << ") row height" << "\n"
  //    << "\t+ " << horizontalHeader()->height()
  //      << "\theader\n";
  //  if (horizontalScrollBar()->isVisible())
  //    q << "\t+ " << horizontalScrollBar()->height()
  //      << "\tscrollbar\n";
  //  q << "\t+ " << 2 * frameWidth() << "\tframe\n";

    setMinimumHeight(fitHeight);
    setMaximumHeight(fitHeight);
  }

};

struct CustomTableView : public VerticallyConcentratedTable {
  QMenu *_contextMenu;

  enum Action { ADD, COMBINE, OVERWRITE, DELETE };
  QMap<Action, QAction*> actions;

  CustomTableView (void) {
    _contextMenu = new QMenu;

    const auto action = [this] (Action a, const QString &text) {
      QAction *qa = new QAction(text);
      qa->setData(a);
      actions[a] = qa;
      return qa;
    };
    action(ADD, "Ajouter");
    action(COMBINE, "Combiner");
    action(OVERWRITE, "Remplacer");
    action(DELETE, "Supprimer");

    for (Action a: actions.keys())
      _contextMenu->addAction(actions.value(a));
  }

  void dropEvent(QDropEvent *e) override {
    QModelIndex index = indexAt(e->pos());
    bool notEmpty = !index.data().toString().isEmpty();
    if (notEmpty) {
      actions[ADD]->setEnabled(false);
      actions[COMBINE]->setEnabled(true);
      actions[OVERWRITE]->setEnabled(true);
      actions[DELETE]->setEnabled(false);

      QAction *action = _contextMenu->exec(mapToGlobal(e->pos()));
      if (action == nullptr)  return;

      switch (action->data().value<Action>()) {
      case COMBINE:
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
    actions[ADD]->setEnabled(true);
    actions[COMBINE]->setEnabled(false);
    actions[OVERWRITE]->setEnabled(false);
    actions[DELETE]->setEnabled(notEmpty);

    QAction *action = _contextMenu->exec(e->globalPos());
    if (action == nullptr)  return;
    Action eaction = action->data().value<Action>();
    auto model = &db::Book::current().planning;
    if (DELETE == eaction)
      model->setData(index, QVariant(), db::IDRole);

    else if (ADD == eaction) {
      QString item = QInputDialog::getText(this, "", "");
      if (!item.isEmpty())  model->addItem(index, item);
    }
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

  connect(_table, &QTableView::activated, this, &PlanningView::showRecipe);

  layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
  setLayout(layout);
}

struct RecipeSublistModel : public QAbstractListModel {
  RecipeSublistModel (const QJsonArray &jarray) {
    auto &book = db::Book::current();
    for (const QJsonValue &v: jarray) {
      if (v.isDouble())
        _recipes.push_back(&book.recipes.at(db::ID(v.toDouble())));
      else
        _texts.push_back(v.toString());
    }
  }

  int rowCount(const QModelIndex& = QModelIndex()) const override {
    return _recipes.size() + _texts.size();
  }

  QVariant data (const QModelIndex &index, int role) const override {
    int row = index.row();
    if (row < _recipes.size()) {
      db::Recipe *r = _recipes.at(row);
      if (role == Qt::DisplayRole)  return r->title;
      else if (role == db::IDRole)  return r->id;
      else if (role == Qt::DecorationRole)
        return db::PlanningModel::recipeLinkIcon();

    } else {
      row -= _recipes.size();
      if (role == Qt::DisplayRole)  return _texts.at(row);
      else if (role == Qt::DecorationRole)
        return db::PlanningModel::rawTextIcon();
      else if (role == Qt::ForegroundRole)
        return QApplication::palette().color(QPalette::Disabled,
                                             QPalette::WindowText);
    }

    return QVariant();
  }

private:
  QList<db::Recipe*> _recipes;
  QStringList _texts;
};

void PlanningView::showRecipe(const QModelIndex &index) {
  static const auto recipe = [] (db::ID id) {
    return &db::Book::current().recipes.at(id);
  };
  const auto show = [this] (const QJsonValue &v, const QModelIndex &index) {
    if (!v.isDouble())  return;
    db::ID id = db::ID(v.toDouble());
    gui::Recipe (this).show(recipe(id), true, index);
  };
  auto jarray = index.data(db::PlanningModel::JsonRole).value<QJsonArray>();
  if (jarray.empty())
    return;

  else if (jarray.size() == 1) {
    show(jarray.first(), QModelIndex());

  } else {
    QDialog dialog (this, Qt::Popup);
    QHBoxLayout *layout = new QHBoxLayout;
    auto *list = new VerticallyConcentratedTable;

    RecipeSublistModel model (jarray);
    list->setModel(&model);
    list->setEditTriggers(QAbstractItemView::NoEditTriggers);

    list->verticalHeader()->hide();
    auto hheader = list->horizontalHeader();
    hheader->setSectionResizeMode(QHeaderView::ResizeToContents);
    hheader->setStretchLastSection(true);
    hheader->hide();

    connect(list, &QTableView::activated,
            [show] (const QModelIndex &index) {
      QVariant data = index.data(db::IDRole);
      qDebug() << index << " (" << index.data() << index.data(db::IDRole) << ")";
      if (!data.isValid())  return;
      show(db::ID(data.toInt()), index);
    });

    layout->addWidget(list);
    layout->setSizeConstraint(QLayout::SetFixedSize);
    dialog.setLayout(layout);
    dialog.exec();
    qDebug() << "Destroying dialog";
  }
}

} // end of namespace gui

Q_DECLARE_METATYPE(gui::CustomTableView::Action)
