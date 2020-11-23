#ifndef PLANNINGVIEW_H
#define PLANNINGVIEW_H

#include <QDialog>
#include <QTableView>

namespace gui {

class PlanningView : public QDialog {
  Q_OBJECT
public:
  PlanningView(QWidget *parent = nullptr);

private:
  QTableView *_table;

  /// index is from the PlanningModel
  /// submodel_row indicates the selected recipe for the corresponding cell
  void showRecipe (const QModelIndex &index, int submodel_row);
  void showRecipe (const QModelIndex &index) {
    showRecipe(index, 0);
  }

  void showToday (void);
};

struct VerticallyConcentratedTable : public QTableView {
  void resizeEvent(QResizeEvent *e) override;
  void updateSize(void);
};

#ifndef Q_OS_ANDROID
class PlanningTableView : public VerticallyConcentratedTable {
  Q_OBJECT
public:
  enum Action { INVALID, ADD = 1, COMBINE, OVERWRITE, DELETE };
  PlanningTableView (void);

signals:
  /// Same arguments as PlanningView::showRecipe
  void showRecipe (const QModelIndex &index, int submodel_row);

private:
  QMenu *_contextMenu, *_contentsMenu;
  QMap<Action, QAction*> _actions;

  void wheelEvent(QWheelEvent *e) override;

  void dropEvent(QDropEvent *e) override;
  void contextMenuEvent(QContextMenuEvent *e) override;

  void populateContentsMenu (const QModelIndex &index, int threshold);
};
#endif

} // end of namespace gui

#endif // PLANNINGVIEW_H
