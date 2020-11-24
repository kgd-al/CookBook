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
#include <QScroller>

#include "planningview.h"
#include "gui_recipe.h"
#include "androidspecifics.hpp"
#include "../db/book.h"

#include <QDebug>

namespace gui {

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

  bool isRecipe (int row) const {
    return row < _recipes.size();
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

// =============================================================================

void VerticallyConcentratedTable::resizeEvent(QResizeEvent *e) {
  QTableView::resizeEvent(e);
  QTimer::singleShot(0, this, &VerticallyConcentratedTable::updateSize);
}

void VerticallyConcentratedTable::updateSize(void) {
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

// =============================================================================

#ifndef Q_OS_ANDROID
PlanningTableView::PlanningTableView (void) {
  _contextMenu = new QMenu;

  const auto action = [this] (Action a, const QString &text) {
    QAction *qa = new QAction(text);
    qa->setData(a);
    _actions[a] = qa;
    return qa;
  };
  action(ADD, "Ajouter");
  action(COMBINE, "Combiner");
  action(OVERWRITE, "Remplacer");
  action(DELETE, "Supprimer");

  _contentsMenu = _contextMenu->addMenu("Contenus");

  for (Action a: _actions.keys())
    _contextMenu->addAction(_actions.value(a));
}

void PlanningTableView::dropEvent(QDropEvent *e) {
  QModelIndex index = indexAt(e->pos());
  bool notEmpty = !index.data().toString().isEmpty();
  if (notEmpty) {
    _actions[ADD]->setEnabled(false);
    _actions[COMBINE]->setEnabled(true);
    _actions[OVERWRITE]->setEnabled(true);
    _actions[DELETE]->setEnabled(false);

    populateContentsMenu(index, 0);

    QAction *action = _contextMenu->exec(mapToGlobal(e->pos()));
    _contentsMenu->clear();

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

void PlanningTableView::contextMenuEvent(QContextMenuEvent *e) {
  QModelIndex index = indexAt(e->pos());
  bool notEmpty = !index.data().toString().isEmpty();
  _actions[ADD]->setEnabled(true);
  _actions[COMBINE]->setEnabled(false);
  _actions[OVERWRITE]->setEnabled(false);
  _actions[DELETE]->setEnabled(notEmpty);

  populateContentsMenu(index, 1);

  QAction *action = _contextMenu->exec(e->globalPos());
  _contentsMenu->clear();

  if (action == nullptr)  return;
  Action eaction = action->data().value<Action>();
  qDebug() << eaction;
  auto model = &db::Book::current().planning;
  if (DELETE == eaction)
    model->setData(index, QVariant(), db::PlanningModel::JsonRole);

  else if (ADD == eaction) {
    QString item = QInputDialog::getText(this, "", "");
    if (!item.isEmpty())  model->addItem(index, item);
  }

}

void PlanningTableView::populateContentsMenu (const QModelIndex &index,
                                              int threshold) {
  static const auto JsonRole = db::PlanningModel::JsonRole;
  RecipeSublistModel m (index.data(JsonRole).value<QJsonArray>());
  qDebug() << m.rowCount();
  if (m.rowCount() <= threshold)
    _contentsMenu->menuAction()->setVisible(false);

  else {
    _contentsMenu->menuAction()->setVisible(true);
    for (int i=0; i<m.rowCount(); i++) {
      auto sub_index = m.index(i);
      auto text = m.data(sub_index, Qt::DisplayRole).toString();
      auto icon = m.data(sub_index, Qt::DecorationRole).value<QIcon>();
      qDebug() << sub_index << text << icon;
      QAction *a = _contentsMenu->addAction(icon, text, [this, index, i] {
        showRecipe(index, i);
      });
      a->setEnabled(m.isRecipe(i));
    }
  }
}

void PlanningTableView::wheelEvent(QWheelEvent *e) {
//  qDebug() << verticalScrollBar()->isVisible();
  if (!verticalScrollBar()->isVisible()) {
    // transform vertical scroll into horizontal
    QPoint invertedDelta (e->angleDelta().y(), e->angleDelta().x());
    QWheelEvent e2 (e->posF(), e->globalPosF(),
                    e->pixelDelta(), invertedDelta,
                    e->delta(), Qt::Horizontal,
                    e->buttons(), e->modifiers(), e->phase(),
                    e->source(), e->inverted());
//    qDebug() << e << "\n>> " << &e2;
    return VerticallyConcentratedTable::wheelEvent(&e2);
  } else {
//    qDebug() << e;
    return VerticallyConcentratedTable::wheelEvent(e);
  }
}

#endif

PlanningView::PlanningView(QWidget *parent) : QDialog(parent) {
  QHBoxLayout *layout = new QHBoxLayout;
#ifndef Q_OS_ANDROID
  using TableView = PlanningTableView;
#else
  using TableView = QTableView;
#endif
  layout->addWidget(_table = new TableView);
  auto model = &db::Book::current().planning;
  _table->setModel(model);
  _table->setSelectionMode(TableView::SingleSelection);
  _table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  _table->setEditTriggers(QAbstractItemView::NoEditTriggers);

#ifndef Q_OS_ANDROID
  _table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  _table->setDragDropMode(QAbstractItemView::DragDrop);
  _table->setWordWrap(false);
  _table->setTextElideMode(Qt::ElideRight);

  connect(static_cast<TableView*>(_table), &TableView::showRecipe,
          this, QOverload<const QModelIndex&,int>::of(
                  &PlanningView::showRecipe));

  QVBoxLayout *blayout = new QVBoxLayout;
    QToolButton *clear = new QToolButton;
    clear->setIcon(QApplication::style()->standardPixmap(
                     QStyle::SP_LineEditClearButton));
    clear->setToolTip(tr("Supprimer les dates dans le passé"));
    connect(clear, &QToolButton::clicked, model, &db::PlanningModel::clearOld);
    blayout->addWidget(clear);

    QToolButton *today = new QToolButton;
    today->setToolTip(tr("Afficher aujourd'hui"));
    connect(today, &QToolButton::clicked, this, &PlanningView::showToday);
    blayout->addWidget(today);
  layout->addLayout(blayout);

#else
  _table->setWordWrap(true);
  _table->horizontalHeader()->setStretchLastSection(true);
  _table->horizontalHeader()->hide();
  _table->verticalHeader()->hide();

  android::enableTouchScrolling(_table);

  QFont f = _table->font();
  f.setPixelSize(1.5*f.pixelSize());
  _table->setFont(f);
#endif

  connect(model, &db::PlanningModel::modelReset,
          [this] { QTimer::singleShot(500, this, &PlanningView::showToday); });

  connect(_table, &QTableView::activated,
          this, QOverload<const QModelIndex&>::of(&PlanningView::showRecipe));

  layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
  setLayout(layout);
}

void PlanningView::showRecipe(const QModelIndex &index, int submodel_row) {
  static const auto recipe = [] (db::ID id) {
    return &db::Book::current().recipes.at(id);
  };
  const auto show = [this] (const QJsonValue &v, const QModelIndex &index) {
    if (!v.isDouble())  return;
    db::ID id = db::ID(v.toDouble());
    if (id <= 0)  return;
    gui::Recipe (this).show(recipe(id), true, index);
  };
  auto jarray = index.data(db::PlanningModel::JsonRole).value<QJsonArray>();
  qDebug() << index << jarray;
  if (jarray.empty())
    return;

  else if (jarray.size() == 1)
    show(jarray.first(), QModelIndex());

  else {
    RecipeSublistModel model (jarray);
    auto index = model.index(submodel_row);
    show(db::ID(index.data(db::IDRole).toInt()), index);
  }
}

void PlanningView::showToday(void) {
  auto model = static_cast<db::PlanningModel*>(_table->model());
  auto index = model->todayOrLatter();
  qDebug() << "Auto scrolling to: " << index;
#ifndef Q_OS_ANDROID
  // manual horizontal scrolling
  QScrollBar *hscroll = _table->horizontalScrollBar();
  hscroll->setValue(hscroll->maximum());
  _table->scrollTo(index, QAbstractItemView::PositionAtTop);
#else
  // vertical scrolling works fine
  _table->scrollTo(index, QAbstractItemView::PositionAtTop);
#endif
}

} // end of namespace gui

#ifndef Q_OS_ANDROID
Q_DECLARE_METATYPE(gui::PlanningTableView::Action)
#endif
