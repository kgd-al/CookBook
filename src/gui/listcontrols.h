#ifndef LISTCONTROLS_H
#define LISTCONTROLS_H

#include <QAbstractItemView>
#include <QBoxLayout>
#include <QToolButton>

namespace gui {

class ListControls : public QWidget {
public:
  ListControls (QAbstractItemView *view = nullptr,
                QBoxLayout::Direction direction = QBoxLayout::LeftToRight);

  QToolButton* addButton (void)   { return _add;  }
  QToolButton* editButton (void)  { return _edit; }
  QToolButton* delButton (void)   { return _del;  }

  void setView (QAbstractItemView *view);

  void setNeedsConfirmation (bool c) {
    _needsConfirmation = c;
  }

private:
  const QAbstractItemView *_view;
  QToolButton *_add, *_edit, *_del;
  bool _needsConfirmation;

  void setState (void);
  void deleteSelection (void);
};

} // end of namespace gui

#endif // LISTCONTROLS_H
