#ifndef REPAIRSMANAGER_H
#define REPAIRSMANAGER_H

#include <QDialog>
#include <QTabWidget>

namespace gui {

enum struct Analysis {
  COUNT_RECIPE,      COUNT_INGREDIENT,      COUNT_UNIT,
                     HOMONYMOUS_INGREDIENT, HOMONYMOUS_UNIT,
                     UNUSED_INGREDIENT,     UNUSED_UNIT
};

struct CachedAnalysis;
struct Summary;

class RepairsManager : public QDialog {
public:
  RepairsManager(QWidget *parent);
  ~RepairsManager (void);

private:
  CachedAnalysis *_results;
  QMap<Analysis, Summary*> _summaries;

  QTabWidget *_resultsDisplayer;
  QWidget *_allGood;

  QPushButton *_apply;

  void checkAll (void);
  void correct (void);

  void postAction (void);
  void summaryChecked (Summary *s, bool checked);
};

} // end of namespace gui

#endif // REPAIRSMANAGER_H
