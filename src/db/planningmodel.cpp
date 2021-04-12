#include <QJsonArray>
#include <QMimeData>
#include <QApplication>
#include <QPalette>
#include <QStyle>
#include <QFontMetrics>

#include "planningmodel.h"
#include "book.h"
#include "settings.h"

#include <QDebug>

namespace db {

std::array<QString, PlanningModel::ROWS> hheaders {
  "Midi", "Goûter", "Soir"
};

QIcon PlanningModel::recipeLinkIcon(void) {
  return QApplication::style()->standardIcon(QStyle::SP_FileDialogInfoView);
}

QIcon PlanningModel::rawTextIcon(void) {
  return QApplication::style()->standardIcon(QStyle::SP_FileIcon);
}

struct PlanningModel::Data {
  QDate date;
  struct Item {
    using ptr_t = QSharedPointer<Item>;
    enum Type { RECIPE, STRING };

    virtual ~Item (void) {}

    virtual Type type (void) const = 0;
    virtual QString data (void) const = 0;

    virtual QIcon decoration (void) const = 0;
    static QIcon defaultDecoration (void) {
      return QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    }

    virtual QJsonValue toJson (void) const = 0;

    static ptr_t fromJson(const QJsonValue &j) {
      Item *ptr = nullptr;
      switch (j.type()) {
      case QJsonValue::Double:  ptr = new RecipeItem(ID(j.toInt()));  break;
      case QJsonValue::String:  ptr = new StringItem(j.toString());   break;
      default:  break;
      }
      Q_ASSERT(ptr);
      return ptr_t(ptr);
    }
  };
  struct RecipeItem : public Item {
    Recipe *recipe;
    RecipeItem (ID id) : recipe(&db::Book::current().recipes.at(id)) {}

    Type type (void) const override { return RECIPE; }
    QString data (void) const override {
      return recipe->title;
    }
    QIcon decoration (void) const override {
      return recipeLinkIcon();
    }

    QJsonValue toJson (void) const override {
      return recipe->id;
    }
  };
  struct StringItem : public Item {
    QString text;
    StringItem (const QString &s) : text(s) {}

    Type type (void) const override { return STRING; }
    QString data (void) const override {
      return text;
    }
    QIcon decoration (void) const override {
      return rawTextIcon();
    }

    QJsonValue toJson (void) const override {
      return text;
    }
  };
  struct CMP {
    static bool lower (const RecipeItem &lhs, const RecipeItem &rhs) {
      return lhs.recipe->title < rhs.recipe->title;
    }

    static bool lower (const StringItem &lhs, const StringItem &rhs) {
      return lhs.text < rhs.text;
    }

    bool operator() (const Item::ptr_t &lhs, const Item::ptr_t &rhs) {
      if (lhs->type() != rhs->type()) return lhs->type() < rhs->type();

      else if (lhs->type() == Item::RECIPE)
        return lower(static_cast<const RecipeItem&>(*lhs),
                     static_cast<const RecipeItem&>(*rhs));

      else if (lhs->type() == Item::STRING)
        return lower(static_cast<const StringItem&>(*lhs),
                     static_cast<const StringItem&>(*rhs));
      return false;
    }
  };
  using PSet = std::set<Item::ptr_t, CMP>;
  std::array<PSet, ROWS> data;

  Data (const QDate &d) : date(d) {}
  Data (const QJsonValue &j)
    : Data(QDate::fromString(j.toString(), Qt::ISODate)) {}

  bool empty (void) const {
    return std::all_of(data.begin(), data.end(),
                       [] (const PSet &s) { return s.empty(); });
  }

  QJsonArray toJson (void) const {
    QJsonArray j;
    j.append(date.toString(Qt::ISODate));
    for (const PSet &recipes: data) j.append(toJson(recipes));
    return j;
  }

  static Data_ptr fromJson (const QJsonArray &j) {
    auto d = Data_ptr::create(j.first());
    Q_ASSERT(j.size() == ROWS+1);
    for (int i=0; i<ROWS; i++)
      fromJson(j[i+1].toArray(), d->data[i]);
    return d;
  }

private:
  static QJsonArray toJson (const PSet &set) {
    QJsonArray j;
    for (const Item::ptr_t &i: set)  j.append(i->toJson());
    return j;
  }

  static void fromJson (const QJsonArray &j, PSet &r) {
    for (auto i: j) r.insert(Item::fromJson(i));
  }
};

QDate today (void) {
  return QDate::currentDate();
}

QDate fakeToday (void) {
  return today();//.addDays(4);
}

PlanningModel::PlanningModel (void) {
#ifndef Q_OS_ANDROID
  connect(Settings::instance(), &Settings::settingChanged,
          [this] (Settings::Type type, const QVariant&) {
    if (type == Settings::PLANNING_WINDOW)  populateModel();
  });
#endif
}

#ifndef Q_OS_ANDROID
int PlanningModel::rowCount(const QModelIndex&) const {
  return ROWS;
}

int PlanningModel::columnCount(const QModelIndex&) const {
  return _data.size();
}

#else

int PlanningModel::rowCount(const QModelIndex&) const {
  return (ROWS+2) * _data.size() - 1;
}

int PlanningModel::columnCount(const QModelIndex&) const {
  return 1;
}

#endif

QString formatDate (const QDate &date) {
  auto diff = fakeToday().daysTo(date);
  if (diff < 0)
    return QString::number(diff);
  else if (diff == 0)
    return QObject::tr("Aujourd'hui");
  else if (diff < 7)
    return date.toString("dddd");
  else
    return date.toString("dddd d");
}

#ifndef Q_OS_ANDROID
QVariant PlanningModel::headerData(int section, Qt::Orientation orientation,
                                   int role) const {

  if (role != Qt::DisplayRole)  return QVariant();
  if (orientation == Qt::Vertical)  return hheaders.at(section);
  return formatDate(_data.at(section)->date);
}
#endif

template <typename Set>
QString formatRecipeList (const Set &set,
                           const QString &separator) {
  QString text;
  if (set.size() > 0) {
    text += (*set.begin())->data();
    for (auto it=std::next(set.begin()); it!=set.end(); ++it)
      text += separator + (*it)->data();
  }
  return text;
}

template <typename Set>
QVariant decoration (const Set &set) {
  if (set.empty())  return QVariant();
  else if (set.size() == 1)
    return (*set.begin())->decoration();
  else
    return PlanningModel::Data::Item::defaultDecoration();
}

QVariant PlanningModel::data (const QModelIndex &i, int role) const {
#ifndef Q_OS_ANDROID
  auto dayData = [this] (const QModelIndex &i) {
    return _data.at(i.column());
  };
  auto cellData = [dayData] (const QModelIndex &i) {
    return dayData(i)->data.at(i.row());
  };
#else
  auto dayData = [this] (const QModelIndex &i) {
    return _data.at(i.row() / (ROWS+2));
  };
  auto cellData = [dayData] (const QModelIndex &i) {
    return dayData(i)->data.at(i.row() % (ROWS+2) - 1);
  };
  bool dateCell = ((i.row() % (ROWS+2)) == 0);
  bool separator = ((i.row() % (ROWS+2)) == ROWS+1);
#endif

  switch (role) {
#ifndef Q_OS_ANDROID
  case Qt::DisplayRole:
    return formatRecipeList(cellData(i), " & ");
  case Qt::ToolTipRole:
    return formatRecipeList(cellData(i), "\n");
  case Qt::DecorationRole:
    return decoration(cellData(i));
  case Qt::ForegroundRole:
    return QApplication::palette().color(
      dayData(i)->date < db::fakeToday() ? QPalette::Disabled
                                         : QPalette::Active,
      QPalette::WindowText);

#else
  case Qt::DisplayRole:
    if (separator)
      return QVariant();
    else if (dateCell)
      return formatDate(dayData(i)->date);
    else
      return formatRecipeList(cellData(i), "\n");

  case Qt::DecorationRole:
    return (separator || dateCell) ? QVariant() : decoration(cellData(i));

  case Qt::FontRole: {
    QFont f;
    if (dateCell) f.setBold(true);
    return f;
  }

  case Qt::TextAlignmentRole:
    return dateCell ? Qt::AlignCenter : Qt::AlignLeft;

  case Qt::ForegroundRole: {
    auto c = QApplication::palette().color(QPalette::Active,
                                           QPalette::WindowText);
    if (dayData(i)->date < db::fakeToday()) c = c.darker();
    return c;
  }
  case Qt::BackgroundRole: {
    auto c = dateCell? QApplication::palette().alternateBase()
                     : QApplication::palette().base();
    if (dayData(i)->date < db::fakeToday()) c = c.color().lighter();
    return c;
  }
  case Qt::SizeHintRole:
    if (!separator) return QVariant();
    return QSize(0, .5*QApplication::fontMetrics().height());
#endif

  case IDRole:
    Q_ASSERT(false);
    break;
  case JsonRole: {
    QJsonArray jarray;
    for (const auto &item: cellData(i)) jarray.append(item->toJson());
    return jarray;
  }


  default: break;
  }

  return QVariant();
}

#ifndef Q_OS_ANDROID
Qt::ItemFlags PlanningModel::flags (const QModelIndex &index) const {
  auto f = QAbstractItemModel::flags(index) | Qt::ItemIsDropEnabled;
  if (!index.data().toString().isEmpty()) f |= Qt::ItemIsDragEnabled;
  return f;
}

QMimeData* PlanningModel::mimeData(const QModelIndexList &indexes) const {
  QMimeData *d = QAbstractTableModel::mimeData(indexes);
  Q_ASSERT(indexes.size() == 1);
  d->setText(indexes.front().data().toString());

  QJsonArray jarray;
  for (const QModelIndex &i: indexes)
     for (const QJsonValue &v: data(i, JsonRole).value<QJsonArray>())
       jarray.append(v);

  d->setData(Recipe::MimeType, toByteArray(jarray));
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
  if (!parent.isValid()) {
    q << ">> Invalid index. Aborting\n";
    return false;
  }
  QJsonArray jarray = fromByteArray(data->data(Recipe::MimeType));
  q << "input: " << jarray << "\n";
  if (action & MergeAction) {
    q << "Try to merge with current data\n";
    for (const QJsonValue &v: parent.data(JsonRole).value<QJsonArray>())
      jarray.append(v);
  }
  auto r = setData(parent, jarray, JsonRole);
  if (!r) return false;
  if (action != Qt::MoveAction) return true;
  QModelIndex srcIndex = data->property("src").toModelIndex();
  return setData(srcIndex, QVariant(), JsonRole);
}

bool PlanningModel::setData (const QModelIndex &index, const QVariant &value,
                             int role) {
  if (role == JsonRole) {
    QJsonArray jarray = value.toJsonArray();
    Data::PSet &set = _data.at(index.column())->data.at(index.row());
    set.clear();
    for (const QJsonValue &v: jarray)
      set.insert(Data::Item::fromJson(v));
    if (jarray.size() != int(set.size())) return false;
    emit dataChanged(index, index, {role});
    return true;

  } else
    return QAbstractTableModel::setData(index, value, role);
}

void PlanningModel::addItem(const QModelIndex &index, const QString &item) {
  _data.at(index.column())->data.at(index.row()).insert(Data::Item::fromJson(item));
  emit dataChanged(index, index, {Qt::DisplayRole});
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
}
#endif

void PlanningModel::fromJson (const QJsonArray &j) {
  beginResetModel();

  auto q = qDebug().nospace();
  q << "Reading model from json\n";

  for (const QJsonValue &v: j)
    _data.push_back(Data::fromJson(v.toArray()));

  q << "Read " << _data.size() << " items\n";

  endResetModel();

#ifndef Q_OS_ANDROID
  populateModel();
#endif
}

#ifndef Q_OS_ANDROID
void PlanningModel::populateModel(void) {
  beginResetModel();
  auto prevsize = _data.size();
  QDate today = db::fakeToday();
  int range = Settings::value<int>(Settings::PLANNING_WINDOW);
  int offset = 0;
  while (offset < _data.size() && _data[offset]->date < today)  offset++;
  for (int i=0; i<range; i++) {
    int j = offset+i;
    QDate date = today.addDays(i);
    if (j >= _data.size()  // Not present
     || date < _data[j]->date)  // Next is in the future
      _data.insert(j, Data_ptr::create(date));
  }

  int i=_data.size()-1;
  while (i >= 0 && today.addDays(range) <= _data[i]->date) {
    if (_data[i]->empty()) _data.removeAt(i);
    i--;
  }

  qDebug() << "Model size after auto-populate: " << _data.size() << "(+"
           << (_data.size() - prevsize) << ")";
  endResetModel();
}

QJsonArray PlanningModel::toJson (void) const {
  QJsonArray j;
  for (const auto &d: _data)
    if (!d->empty())
      j.append(d->toJson());
  return j;
}
#endif

QModelIndex PlanningModel::todayOrLatter (void) const {
  for (int i=0; i<_data.size(); i++) {
    if (_data[i]->date == today()) {
#ifndef Q_OS_ANDROID
      return index(0, i);
#else
      return index(i*(ROWS+2), 0);
#endif
    }
  }
  return QModelIndex();
}

} // end of namespace db
