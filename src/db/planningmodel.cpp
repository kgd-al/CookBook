#include <QJsonArray>
#include <QMimeData>
#include <QApplication>
#include <QPalette>

#include "planningmodel.h"
#include "book.h"

#include <QDebug>

namespace db {

static constexpr int DAYS_RANGE = 7;

std::array<QString, PlanningModel::ROWS> hheaders {
  "Midi", "Goûter", "Soir"
};

struct PlanningModel::Data {
  QDate date;
  struct CMP {
    bool operator() (const Recipe *lhs, const Recipe *rhs) {
      return lhs->title < rhs->title;
    }
  };
  using RSet = std::set<Recipe*, CMP>;
  std::array<RSet, ROWS> data;

  Data (const QDate &d) : date(d) {}

  bool empty (void) const {
    return std::all_of(data.begin(), data.end(),
                       [] (const RSet &s) { return s.empty(); });
  }

  QJsonArray toJson (void) const {
    QJsonArray j;
    j.append(date.toString());
    for (const RSet &recipes: data) j.append(toJson(recipes));
    return j;
  }

  static Data_ptr fromJson (const QJsonArray &j) {
    auto d = Data_ptr::create(QDate::fromString(j.first().toString()));
    Q_ASSERT(j.size() == ROWS+1);
    for (int i=0; i<ROWS; i++)
      fromJson(j[i+1].toArray(), d->data[i]);
    return d;
  }

private:
  static QJsonArray toJson (const RSet &set) {
    QJsonArray j;
    for (const Recipe *r: set)  j.append(r->id);
    return j;
  }

  static void fromJson (const QJsonArray &j, RSet &r) {
    for (auto id: j)
      r.insert(&Book::current().recipes.at(ID(id.toInt())));
  }
};

QDate today (void) {
  return QDate::currentDate();
}

QDate fakeToday (void) {
  return today();//.addDays(4);
}

int PlanningModel::rowCount(const QModelIndex&) const {
  return ROWS;
}

int PlanningModel::columnCount(const QModelIndex&) const {
  return _data.size();
}

QVariant PlanningModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const {
  if (role != Qt::DisplayRole)  return QVariant();
  if (orientation == Qt::Vertical)  return hheaders.at(section);
  auto date = _data.at(section)->date;
  auto diff = fakeToday().daysTo(date);
  if (0 <= diff && diff < DAYS_RANGE)
    return date.toString("dddd");
  else
    return ((diff > 0) ? "+" : "") + QString::number(diff);
}

template <typename Set>
QVariant formatRecipeList (const Set &set,
                           const QString &separator) {
  QString text;
  if (set.size() == 0)
    return QVariant();
  else if (set.size() > 1 && separator != "\n")
    text += "(+) ";
  text += (*set.begin())->title;
  for (auto it=std::next(set.begin()); it!=set.end(); ++it)
    text += separator + (*it)->title;
  return text;
}

QVariant PlanningModel::data (const QModelIndex &i, int role) const {
  auto cellData = [this] (const QModelIndex &i) {
    return _data.at(i.column())->data.at(i.row());
  };
  switch (role) {
  case Qt::DisplayRole:
    return formatRecipeList(cellData(i), " & ");
  case Qt::ToolTipRole:
    return formatRecipeList(cellData(i), "\n");
  case Qt::ForegroundRole:
    return QApplication::palette().color(
      _data.at(i.column())->date < db::fakeToday() ? QPalette::Disabled
                                                   : QPalette::Active,
      QPalette::WindowText);
  case IDRole: {
    IDSet ids;
    for (Recipe *r: cellData(i)) ids.insert(r->id);
    return QVariant::fromValue(ids);
  }
  default:
    return QVariant();
  }
}

Qt::ItemFlags PlanningModel::flags (const QModelIndex &index) const {
  auto f = QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled;
  if (!index.data().toString().isEmpty()) f |= Qt::ItemIsDragEnabled;
  return f;
}

QMimeData* PlanningModel::mimeData(const QModelIndexList &indexes) const {
  QMimeData *d = QAbstractTableModel::mimeData(indexes);
  Q_ASSERT(indexes.size() == 1);
  d->setText(indexes.front().data().toString());

  IDSet ids;
  for (const QModelIndex &i: indexes)
    ids += data(i, IDRole).value<IDSet>();

  d->setData(Recipe::MimeType, toByteArray(ids));
  qDebug() << "dragMimeData" << d->text() << d->data(Recipe::MimeType);
  d->setProperty("src", indexes.front());
  return d;
}

bool PlanningModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                 int row, int column,
                                 const QModelIndex &parent) {
  auto q = qDebug().nospace();
  q << "dropMimeData(" << data << " " << action << " " << row << " " << column
    << " " << parent << ")\n";
  IDSet ids = fromByteArray(data->data(Recipe::MimeType));
  q << "input: [";
  for (ID id: ids)  q << " " << id;
  q << " ]\n";
  if (action & MergeAction) {
    q << "Try to merge with current data\n";
    ids.unite(parent.data(IDRole).value<IDSet>());
  }
  auto r = setData(parent, QVariant::fromValue(ids), IDRole);
  if (!r) return false;
  if (action != Qt::MoveAction) return false;
  QModelIndex srcIndex = data->property("src").toModelIndex();
  return setData(srcIndex, QVariant(), IDRole);
}

bool PlanningModel::setData (const QModelIndex &index, const QVariant &value,
                             int role) {
  if (role == IDRole) {
    IDSet ids = value.value<IDSet>();
    Data::RSet &recipes = _data.at(index.column())->data.at(index.row());
    recipes.clear();
    for (ID id: ids)  recipes.insert(&Book::current().recipes.at(id));
    if (ids.size() != int(recipes.size())) return false;
    emit dataChanged(index, index, {role});
    return true;

  } else
    return QAbstractTableModel::setData(index, value, role);
}

void PlanningModel::clearOld(void) {
  int old=0;
  while (_data[old]->date < db::fakeToday())  old++;
  qDebug() << "Want to clean" << old << "old planning days";
  if (old > 0) {
    beginRemoveColumns(QModelIndex(), 0, old-1);
    for (int i=0; i<old; i++) _data.removeFirst();
    endRemoveColumns();
    emit dataChanged(index(0, 0), index(ROWS, old-1));
  }
  Q_ASSERT(DAYS_RANGE == _data.size());
}

void PlanningModel::fromJson (const QJsonArray &j) {
  beginResetModel();

  auto q = qDebug().nospace();
  q << "Reading model from json\n";

  for (const QJsonValue &v: j)
    _data.push_back(Data::fromJson(v.toArray()));

  q << "Read " << _data.size() << " items\n";

  QDate today = db::fakeToday();
  int offset = 0;
  while (offset < _data.size() && _data[offset]->date < today)  offset++;
  for (int i=0; i<DAYS_RANGE; i++) {
    int j = offset+i;
    QDate date = today.addDays(i);
    if (j >= _data.size()  // Not present
     || date < _data[j]->date)  // Next is in the future
      _data.insert(j, Data_ptr::create(date));
  }

  q << "Final size: " << _data.size() << "\n";
  endResetModel();
}

QJsonArray PlanningModel::toJson (void) const {
  QJsonArray j;
  for (const auto &d: _data)
    if (!d->empty())
      j.append(d->toJson());
  return j;
}

} // end of namespace db
