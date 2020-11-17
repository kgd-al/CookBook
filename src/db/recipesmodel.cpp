#include <QJsonArray>
#include <QPainter>
#include <QMimeData>

#include "recipesmodel.h"

#include <QDebug>

QDataStream& operator>> (QDataStream &stream, db::ID &id) {
  return stream >> (std::underlying_type<db::ID>::type&)id;
}

namespace db {

QByteArray toByteArray (const QSet<ID> &set) {
  QByteArray array;
  QDataStream stream (&array, QIODevice::WriteOnly);
  stream << set;
  return array;
}

QSet<ID> fromByteArray(const QByteArray &array) {
  QSet<db::ID> set;
  QDataStream stream (array);
  stream >> set;
  return set;
}

RecipesModel::RecipesModel(void) {}

QModelIndex RecipesModel::addRecipe(Recipe &&r) {
  int i = rowCount();
  beginInsertRows(QModelIndex(), i, i);
  r.id = nextID();
  auto p = _data.emplace(r.id, r);
  endInsertRows();

  Q_ASSERT(p.second);
  valueModified(r.id);
  return index(std::distance(_data.begin(), p.first), 0);
}

void RecipesModel::delRecipe(Recipe *r) {
  valueModified(r->id);
  removeItem(r->id);
}

int RecipesModel::columnCount(void) {
  return 7;
}

int RecipesModel::titleColumn(void) {
  return columnCount()-1;
}

int RecipesModel::columnCount(const QModelIndex&) const {
  return RecipesModel::columnCount();
}

QVariant RecipesModel::headerData(int section, Qt::Orientation orientation,
                                  int role) const {
  if (role != Qt::SizeHintRole) return QVariant();
  if (section == titleColumn() && orientation == Qt::Horizontal)
    return QVariant();
  return QSize(iconSize(), .25*iconSize());
}

QVariant RecipesModel::data (const QModelIndex &index, int role) const {
  if (!index.isValid()) return QVariant();
  const Recipe &r = atIndex(index.row());
  switch (role) {
  case Qt::DisplayRole:
  case Qt::EditRole:
    return (index.column() == titleColumn()) ? r.title : QVariant();

  case Qt::DecorationRole:
    switch (index.column()) {
    case 0:   return r.basicIcon();
    case 1:   return r.subrecipeIcon();
    case 2:   return r.regimen->decoration;
    case 3:   return r.type->decoration;
    case 4:   return r.duration->decoration;
    case 5:   return r.status->decoration;
    default:  return QVariant();
    }

  case Qt::SizeHintRole:
    return (index.column() < titleColumn()) ? QSize(0, 0)
                                            : QVariant();

  case IDRole:
    return r.id;

  case PtrRole:
    return QVariant::fromValue(&r);

  case SortRole:
    switch (index.column()) {
    case 0: return r.basic;
    case 1: return r.used;
    case 2: return r.regimen->id;
    case 3: return r.type->id;
    case 4: return r.duration->id;
    case 5: return r.status->id;
    case 6: return r.title;
    default:  return QVariant();
    }
  }
  return QVariant();
}

Qt::ItemFlags RecipesModel::flags(const QModelIndex &index) const {
  auto f = base_t::flags(index);
  if (index.column() == titleColumn())  f |= Qt::ItemIsDragEnabled;
  return f;
}

QMimeData* RecipesModel::mimeData(const QModelIndexList &indexes) const {
  QMimeData *d = base_t::mimeData(indexes);
  Q_ASSERT(indexes.size() == 1);
  d->setText(indexes.front().data().toString());

  QSet<ID> ids;
  for (const QModelIndex &i: indexes) ids.insert(ID(i.data(IDRole).toInt()));
#ifndef NDEBUG
  auto idsBA = toByteArray(ids);
  auto ids2 = fromByteArray(idsBA);
  Q_ASSERT(ids == ids2);
#endif
  qDebug() << "MimeData for " << indexes << " = " << ids;
  d->setData(Recipe::MimeType, toByteArray(ids));
  return d;
}

void RecipesModel::valueModified(ID id) {
  int index = indexOf(id);
  emit dataChanged(createIndex(index, 0), createIndex(index, columnCount()));
}

void RecipesModel::fromJson(const QJsonArray &a) {
  for (const QJsonValue &v: a) {
    Recipe r = Recipe::fromJson(v);
    _data.insert({r.id, r});
    _nextID = std::max(r.id, _nextID);
  }

  for (const auto &p: _data)
    for (auto &i: p.second.ingredients)
      if (i->etype == EntryType::SubRecipe)
        static_cast<SubRecipeEntry*>(i.data())->setRecipeFromHackedPointer();

  nextID();
}

QJsonArray RecipesModel::toJson(void) {
  QJsonArray a;
  for (const auto &p: _data)
    a.append(Recipe::toJson(p.second));
  return a;
}

} // end of namespace db
