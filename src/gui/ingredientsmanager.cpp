#include <QVBoxLayout>
#include <QHeaderView>

#include <QMenu>
#include <QStandardItemModel>

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

#include "ingredientsmanager.h"
#include "listcontrols.h"
#include "common.h"

#include "../db/book.h"

namespace gui {

template <int C>
struct UsageStatsModel : public QAbstractTableModel {
  int rowCount(const QModelIndex& = QModelIndex()) const override {
    return _data.size();
  }

  int columnCount(const QModelIndex& = QModelIndex()) const override {
    return C;
  }

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override {
    if (role != Qt::DisplayRole)      return QVariant();
    if (orientation == Qt::Vertical)  return section;
    if (section >= _headers.size())   return section;
    return _headers.at(section);
  }

  QVariant data (const QModelIndex &index, int role) const {
    if (role != Qt::DisplayRole)  return QVariant();
    if (index.column() < 0)       return QVariant();
    if (index.column() >= C)    return QVariant();
    return _data.at(index.row()).at(index.column());
  }

  void clear (void) {
    beginResetModel();
    _data.clear();
    endResetModel();
  }

  void setHeaderLabels (const QStringList &headers) {
    Q_ASSERT(headers.size() == C);
    _headers = headers;
  }

  void appendRow (const QStringList &row) {
    Q_ASSERT(row.size() == C);
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    _data.append(row);
    endInsertRows();
  }

private:
  QStringList _headers;
  QList<QStringList> _data;
};

struct UsageStatsPopup : public QDialog {
  QLabel *_label;
  UsageStatsModel<3> _model;
  QSortFilterProxyModel _proxy;
  QTableView *_table;

  UsageStatsPopup (QWidget *parent) : QDialog(parent, Qt::Popup) {
    QLayout *layout = new QVBoxLayout;
    _label = new QLabel;
    layout->addWidget(_label);
    _table = new QTableView;
    _proxy.setSourceModel(&_model);
    _table->setModel(&_proxy);
    _table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    _table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _table->setSortingEnabled(true);
    _table->sortByColumn(0, Qt::DescendingOrder);
    layout->addWidget(_table);
    setLayout(layout);
  }

  UsageStatsModel<3>& resetModel (void) {
    _model.clear();
    return _model;
  }

  void exec (const QPoint &pos, const QString &unit) {
    _label->setText("Utilisation(s) de " + unit + ":");
    _table->sortByColumn(0, Qt::DescendingOrder);
    move(pos);
    adjustSize();
    QDialog::exec();
  }
};

IngredientsManager::IngredientsManager(QWidget *parent)
  : QDialog(parent) {

  setWindowTitle("Gestionnaire d'ingrédients");

  int r = 0, c = 0;

  QGridLayout *layout = new QGridLayout;
    layout->addWidget(new QLabel(tr("Ingrédients")),
                      r++, c, 1, 1, Qt::AlignCenter);

    _itable = new QTableView;
    layout->addWidget(_itable, r++, c);

    QHBoxLayout *linput = new QHBoxLayout;
      QLineEdit *ledit =  new QLineEdit;
      ledit->setPlaceholderText("Ingrédient");
      linput->addWidget(ledit);

      QComboBox *lbox = new QComboBox;
      lbox->setModel(db::getStaticModel<db::AlimentaryGroupData>());
      linput->addWidget(lbox);

      auto lcontrols = new ListControls(_itable);
      linput->addWidget(lcontrols);
    layout->addLayout(linput, r++, c);

    QComboBox *lcbox = new QComboBox;
    layout->addWidget(lcbox, r++, c);

  r = 0; c = 1;

    layout->addWidget(new QLabel(tr("Unités")),
                      r++, c, 1, 1, Qt::AlignCenter);

    _utable = new QTableView;
    layout->addWidget(_utable, r++, c);

    QHBoxLayout *rinput = new QHBoxLayout;
      QLineEdit *redit =  new QLineEdit;
      redit->setPlaceholderText("Unité");
      rinput->addWidget(redit);

      auto rcontrols = new ListControls(_utable);
      rinput->addWidget(rcontrols);
    layout->addLayout(rinput, r++, c);

    QComboBox *rcbox = new QComboBox;
    layout->addWidget(rcbox, r++, c);
  setLayout(layout);

  auto isorter = new QSortFilterProxyModel (this);
  isorter->setSourceModel(&db::Book::current().ingredients);
  _itable->setModel(isorter);
  _itable->setSelectionBehavior(QAbstractItemView::SelectRows);
  _itable->setSelectionMode(QAbstractItemView::SingleSelection);
  _itable->verticalHeader()->hide();
  _itable->setSortingEnabled(true);
  _itable->sortByColumn(1, Qt::DescendingOrder);
//  ltable->setIconSize(QSize(db::iconSize(), db::iconSize()));
  auto lheader = _itable->horizontalHeader();
  lheader->setSectionResizeMode(0, QHeaderView::Stretch);
  lheader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  lcontrols->setView(_itable);

  lcbox->setModel(&db::Book::current().ingredients);

  auto usorter = new QSortFilterProxyModel (this);
  usorter->setSourceModel(&db::Book::current().units);
  _utable->setModel(usorter);
  _utable->setSelectionBehavior(QAbstractItemView::SelectRows);
  _utable->setSelectionMode(QAbstractItemView::SingleSelection);
  _utable->verticalHeader()->hide();
  _utable->setSortingEnabled(true);
  _utable->sortByColumn(1, Qt::DescendingOrder);
  auto rheader = _utable->horizontalHeader();
  rheader->setSectionResizeMode(0, QHeaderView::Stretch);
  rheader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  rcontrols->setView(_utable);

  rcbox->setModel(&db::Book::current().units);

//  auto setModified = [] { db::Book::current().modified = true;  };
//  connect(isorter->sourceModel(), &QAbstractItemModel::dataChanged, setModified);
//  connect(usorter->sourceModel(), &QAbstractItemModel::dataChanged, setModified);

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
          [this, ledit, lbox] {
    db::Book::current().ingredients.update(
      db::ID(_itable->currentIndex().data(db::IDRole).toInt()),
      ledit->text(), db::ID(lbox->currentData(db::IDRole).toInt()));
  });

  connect(_itable->selectionModel(), &QItemSelectionModel::currentRowChanged,
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
          [this, redit] {
    db::Book::current().units.update(
      db::ID(_utable->currentIndex().data(db::IDRole).toInt()), redit->text());
  });

  connect(_utable->selectionModel(), &QItemSelectionModel::currentRowChanged,
          [rcontrols, redit] (const QModelIndex &current, const QModelIndex&) {
    using namespace db;
    auto u = current.data(PtrRole).value<const UnitData*>();
    bool unused = (u->used == 0);
    redit->setText(u->text);
    rcontrols->delButton()->setEnabled(unused);
  });

  _popup = new UsageStatsPopup (this);
  _utable->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(_utable, &QTableView::customContextMenuRequested,
          this, &IngredientsManager::unitsContextMenu);
}

void IngredientsManager::unitsContextMenu(const QPoint &pos) {
  if (!_utable->indexAt(pos).isValid()) return;

  QMenu menu (_utable);
  auto *a = menu.addAction("Utilisations");
  connect(a, &QAction::triggered, [this, pos] { unitsUsageStats(pos); });

  menu.exec(_utable->mapToGlobal(pos));
}

void IngredientsManager::unitsUsageStats(const QPoint &pos) {
  QModelIndex index = _utable->indexAt(pos);
  Q_ASSERT(index.isValid());

  auto q = qDebug().nospace();
  q << "Computing usage data for " << index.data() << ":\n";
  QString current = index.data().toString();
  auto &sim = _popup->resetModel();
  sim.setHeaderLabels({ "Recette", "Ingrédient", "Qualificatif(s)"});
  for (const auto &p: db::Book::current().recipes) {
    for (const db::IngredientListEntry::ptr &li: p.second.ingredients) {
      if (li->etype != db::EntryType::Ingredient) continue;

      const auto &i = *static_cast<db::IngredientEntry*>(li.data());
      if (i.unit->text == current)
        sim.appendRow({ p.second.title, i.idata->text, i.qualif });
    }
  }

  _popup->exec(_utable->mapToGlobal(pos), index.data().toString());
}

void IngredientsManager::closeEvent(QCloseEvent *e) {
  auto &settings = localSettings(this);
  settings.setValue("geometry", geometry());
  QDialog::closeEvent(e);
}

} // end of namespace gui
