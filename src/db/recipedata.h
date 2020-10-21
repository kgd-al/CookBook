#ifndef INGREDIENTDATA_H
#define INGREDIENTDATA_H

#include <set>
#include <sstream>

#include <QString>
#include <QColor>
#include <QStandardItemModel>

namespace db {

enum ID : int { INVALID = -1 };
static constexpr int IDRole = Qt::UserRole+42;

extern const int ICON_SIZE;

template <typename T>
struct TransparentID_CMP {
  using is_transparent = void;

  static bool compare (ID lhs_id, ID rhs_id) {
    return lhs_id < rhs_id;
  }

  bool operator() (const T &lhs, const T &rhs) const {
    return compare(lhs.id, rhs.id);
  }

  bool operator() (const T &lhs, ID rhs) const {
    return compare(lhs.id, rhs);
  }

  bool operator() (ID lhs, const T &rhs) const {
    return compare(lhs, rhs.id);
  }
};

template <typename T>
using transparent_set = std::set<T, TransparentID_CMP<T>>;

template <typename T>
const T& at (const transparent_set<T> &set, ID id) {
  auto it = set.find(id);
  if (it == set.end()) {
    std::ostringstream oss;
    oss << "Key " << int(id) << " was not found in the database";
    throw std::invalid_argument(oss.str());
  }
  return *it;
}

template <typename T>
const auto& at (ID id) {  return at(T::database, id);  }

namespace _details { // (private)

template <typename T, typename = int>
struct HasDecoration : std::false_type {};

template <typename T>
struct HasDecoration <T, decltype((void) T::decoration, 0)> : std::true_type {};

} // end of namespace _details (private)

struct BasicModel : public QStandardItemModel {
  template <typename T>
  BasicModel (const transparent_set<T> &database) {
    for (const T& value: database)  appendRow(buildItemWithID(value));
  }

private:
  template <typename T>
  QStandardItem* buildItemWithID (const T &v) {
    QStandardItem *item = buildItem(v);
    item->setData(v.id, IDRole);
    return item;
  }

  template <typename T>
  std::enable_if_t<_details::HasDecoration<T>::value, QStandardItem*>
  buildItem (const T &v) {
    return new QStandardItem(T::iconFromDecoration(v.decoration), v.text);
  }

  template <typename T>
  std::enable_if_t<!_details::HasDecoration<T>::value, QStandardItem*>
  buildItem (const T &v) {
    return new QStandardItem(v.text);
  }
};

template <typename T>
BasicModel* getStaticModel (void) {
  static BasicModel m (T::database);
  return &m;
}

struct UnitData {
  using ID = db::ID;
  ID id = ID::INVALID;
  QString text = "N/A";
  int used = 0;

  static QJsonArray toJson (const UnitData &d);
  static UnitData fromJson (QJsonArray j);

  using Database = transparent_set<UnitData>;
};
using UnitDatabase = UnitData::Database;

struct AlimentaryGroupData {
  using ID = db::ID;
  ID id = ID::INVALID;
  QString text = "N/A";
  QColor decoration;

  static QPixmap iconFromDecoration (const QColor &d);

  using Database = transparent_set<AlimentaryGroupData>;
  static const Database database;
};
using AlimentaryGroupsDatabase = AlimentaryGroupData::Database;

struct RegimenData {
  using ID = db::ID;
  ID id = ID::INVALID;
  QString text = "N/A", decoration;

  static QPixmap iconFromDecoration (const QString &d);

  using Database = transparent_set<RegimenData>;
  static const Database database;
};

struct StatusData {
  using ID = db::ID;
  ID id = ID::INVALID;
  QString text = "N/A";
  QColor decoration;

  static QPixmap iconFromDecoration (const QColor &d);

  using Database = transparent_set<StatusData>;
  static const Database database;
};

struct DishTypeData {
  using ID = db::ID;
  ID id = ID::INVALID;
  QString text = "N/A", decoration;

  static QPixmap iconFromDecoration (const QString &d);

  using Database = transparent_set<DishTypeData>;
  static const Database database;
};

struct DurationData {
  using ID = db::ID;
  ID id = ID::INVALID;
  QString text = "N/A", decoration;

  static QPixmap iconFromDecoration (const QString &d);

  using Database = transparent_set<DurationData>;
  static const Database database;
};

struct IngredientData {
  using ID = db::ID;
  ID id = ID::INVALID;
  QString text = "N/A";
  const AlimentaryGroupData *group = nullptr;
  int used = 0;

  static QJsonArray toJson (const IngredientData &d);
  static IngredientData fromJson (QJsonArray j);

  static const QString NoUnit;
  using Database = transparent_set<IngredientData>;
};
using IngredientsDatabase = IngredientData::Database;

} // end of namespace db

#endif // INGREDIENTDATA_H
