#ifndef PLANNINGMODEL_H
#define PLANNINGMODEL_H

#include <QAbstractTableModel>

#include "recipe.h"

namespace db {

class PlanningModel : public QAbstractTableModel {
public:
  static constexpr int ROWS = 3;
  static constexpr auto MergeAction = Qt::DropAction((Qt::ActionMask+1)>>1);
  static constexpr auto JsonRole = IDRole+1;

  static QIcon recipeLinkIcon (void);
  static QIcon rawTextIcon (void);

  int rowCount(const QModelIndex& = QModelIndex()) const override;
  int columnCount(const QModelIndex& = QModelIndex()) const override;

  QVariant headerData(int section, Qt::Orientation orientation,
                      int role) const override;
  QVariant data (const QModelIndex &index, int role) const override;

  Qt::DropActions supportedDragActions(void) const override {
    return Qt::MoveAction;
  }

  Qt::DropActions supportedDropActions(void) const override {
    return Qt::MoveAction | Qt::LinkAction;
  }

  QStringList mimeTypes(void) const override {
    return QStringList() << db::Recipe::MimeType;
  }

  Qt::ItemFlags flags (const QModelIndex &index) const override;
  QMimeData* mimeData(const QModelIndexList &indexes) const override;
  bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                    int row, int column, const QModelIndex &parent);

  bool setData(const QModelIndex &index, const QVariant &value,
               int role) override;

  void addItem (const QModelIndex &index, const QString &item);

  void clearOld (void);

  void fromJson (const QJsonArray &j);
  QJsonArray toJson (void) const;

private:
  struct Data;
  using Data_ptr = QSharedPointer<Data>;
  QList<Data_ptr> _data;
};

} // end of namespace db

#endif // PLANNINGMODEL_H
