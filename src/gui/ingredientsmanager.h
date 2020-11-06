#ifndef INGREDIENTSMANAGER_H
#define INGREDIENTSMANAGER_H

#include <QDialog>
#include <QTableView>

namespace gui {

struct UsageStatsPopup;
class IngredientsManager : public QDialog {
  Q_OBJECT
public:
  IngredientsManager(QWidget *parent);
  ~IngredientsManager(void) {}

  void closeEvent(QCloseEvent *e);

private:
  QTableView *_itable, *_utable;
  UsageStatsPopup *_popup;

  void unitsContextMenu (const QPoint &pos);
  void unitsUsageStats (const QPoint &pos);
};

} // end of namespace gui

#endif // INGREDIENTSMANAGER_H
