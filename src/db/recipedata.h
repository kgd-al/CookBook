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
static constexpr int PtrRole = IDRole+42;

int iconSize (void);
void fontChanged (const QFont &font);

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
using cb_container = std::map<ID, T>;

template <typename T>
const T& at (const cb_container<T> &map, ID id) {
  if (id <= 0)
    throw std::invalid_argument("ID <= 0 is not a valid key!");
  auto it = map.find(id);
  if (it == map.end()) {
    std::ostringstream oss;
    oss << "Key " << int(id) << " was not found in the database";
    throw std::invalid_argument(oss.str());
  }
  return it->second;
}

template <typename T>
const T& at (ID id) {  return at(T::database(), id);  }

template <typename T>
const T& first (void) { return T::database().begin()->second; }

namespace _details { // (private)

template <typename T, typename = int>
struct HasDecoration : std::false_type {};

template <typename T>
struct HasDecoration <T, decltype((void) T::decoration, 0)> : std::true_type {};

} // end of namespace _details (private)

struct BasicModel : public QStandardItemModel {
  template <typename T>
  BasicModel (const cb_container<T> &database) {
    for (const auto &p: database)  appendRow(buildItemWithID(p.second));
  }

private:
  template <typename T>
  QStandardItem* buildItemWithID (const T &v) {
    QStandardItem *item = new QStandardItem(v.decoration, v.text);
    item->setData(v.id, IDRole);
    return item;
  }
};

template <typename T>
BasicModel* getStaticModel (void) {
  static BasicModel m (T::database());
  return &m;
}

struct MiscIcons {
  static const QIcon& sub_recipe (void);
  static const QIcon& basic_recipe (void);
};

struct UnitData {
  using ID = db::ID;
  ID id;
  QString text;
  int used;

  UnitData (ID i, const QString &t) : id(i), text(t), used(0) {}
  UnitData (void) : UnitData(ID::INVALID, "Invalid unit") {}

  static QJsonArray toJson (const UnitData &d);
  static UnitData fromJson (QJsonArray j);

  using Database = cb_container<UnitData>;
};
using UnitDatabase = UnitData::Database;

enum class StaticDataType : int {
  AlimentaryGroup, Regimen, DishType, Duration, Status
};

template <StaticDataType T>
struct StaticData {
  static constexpr auto TYPE = T;
  using ID = db::ID;
  ID id = ID::INVALID;
  QString text = "N/A";
  QIcon decoration;

  StaticData (ID i, const QString &t, const QIcon &d)
    : id(i), text(t), decoration(d) {}
  StaticData (void)
    : StaticData(ID::INVALID, "Invalid data", QIcon()) {}

  using Database = cb_container<StaticData<T>>;
  static const Database& database (void) {
    static const Database db = loadDatabase();
    return db;
  }

private:
  static Database loadDatabase (void);
};

using AlimentaryGroupData = StaticData<StaticDataType::AlimentaryGroup>;
using AlimentaryGroupsDatabase = AlimentaryGroupData::Database;

using RegimenData = StaticData<StaticDataType::Regimen>;
using DishTypeData = StaticData<StaticDataType::DishType>;
using DurationData = StaticData<StaticDataType::Duration>;
using StatusData = StaticData<StaticDataType::Status>;

struct IngredientData {
  using ID = db::ID;
  ID id = ID::INVALID;
  QString text = "N/A";
  const AlimentaryGroupData *group = nullptr;
  int used = 0;

  static QJsonArray toJson (const IngredientData &d);
  static IngredientData fromJson (QJsonArray j);

  static const QString NoUnit;
  using Database = cb_container<IngredientData>;
};
using IngredientsDatabase = IngredientData::Database;

} // end of namespace db

Q_DECLARE_METATYPE(const db::UnitData*)
Q_DECLARE_METATYPE(const db::IngredientData*)

#endif // INGREDIENTDATA_H
