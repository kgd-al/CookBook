#ifndef FILTERVIEW_H
#define FILTERVIEW_H

#include <QWidget>
#include <QCheckBox>
#include <QRadioButton>
#include <QComboBox>
#include <QLabel>
#include <QTableView>
#include <QListView>
#include <QGridLayout>
#include <QSortFilterProxyModel>

#include <QAbstractItemView>

#include "listcontrols.h"

namespace gui {

struct RecipeFilter;
struct YesNoGroupBox;

class FilterView : public QWidget {
  Q_OBJECT

  RecipeFilter *_filter;

  template <typename T>
  struct Entry {
    QCheckBox *cb;
    T *widget;

    Entry (const QString &label, QGridLayout *layout);
  };
  Entry<QLineEdit> *title;
  Entry<YesNoGroupBox> *basic, *subrecipe;
  Entry<QComboBox> *regimen, *status, *type, *duration;

#ifndef Q_OS_ANDROID
  Entry<QTableView> *ingredients;
  Entry<QListView> *subrecipes;
  ListControls *icontrols, *scontrols;
#endif

public:
  FilterView (QWidget *parent);

  QSortFilterProxyModel* proxyModel (void);

signals:
  void filterChanged(void);

private:
  template <typename T, typename... SRC>
  void connectMany (Entry<T> *entry, SRC... members);

  void processFilterChanges (void);
  void clear (void);
  void random (void);
};

} // end of namespace gui

#endif // FILTERVIEW_H
