#ifndef INGREDIENTDATA_H
#define INGREDIENTDATA_H

#include <set>

#include <QString>
#include <QColor>
#include <QStringListModel>
#include <QAbstractListModel>

namespace db {

enum ID : int { INVALID = -1 };

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
  QColor color;

  using AlimentaryGroupsDatabase = transparent_set<AlimentaryGroupData>;
  static const AlimentaryGroupsDatabase database;
};
using AlimentaryGroupsDatabase = AlimentaryGroupData::AlimentaryGroupsDatabase;

struct IngredientData {
  using ID = db::ID;
  ID id = ID::INVALID;
  QString text = "N/A";
  AlimentaryGroupData const *group = nullptr;
  int used = 0;

  static QJsonArray toJson (const IngredientData &d);
  static IngredientData fromJson (QJsonArray j);

  static const QString NoUnit;
  using Database = transparent_set<IngredientData>;
};
using IngredientsDatabase = IngredientData::Database;

} // end of namespace db

#endif // INGREDIENTDATA_H
