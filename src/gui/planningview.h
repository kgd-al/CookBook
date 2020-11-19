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

  void showRecipe (const QModelIndex &index);
};

} // end of namespace gui

#endif // PLANNINGVIEW_H
