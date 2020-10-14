#ifndef AUTOFILTERCOMBOBOX_H
#define AUTOFILTERCOMBOBOX_H

#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>

#include <QDebug>

namespace gui {

struct AutoFilterComboBox : public QComboBox {
  AutoFilterComboBox (QComboBox::InsertPolicy policy
                      = QComboBox::InsertAtBottom) {
    setEditable(true);
    setInsertPolicy(policy);

    QCompleter *c = completer();
    c->setCompletionMode(QCompleter::PopupCompletion);
    c->setCompletionRole(Qt::DisplayRole);
    c->setFilterMode(Qt::MatchContains);
  }

  void focusOutEvent(QFocusEvent *e) {
    if (QComboBox::NoInsert != insertPolicy()) {
      int index = this->count();
      insertItem(index, lineEdit()->text());
      setCurrentIndex(index);

    } else
      setCurrentText(completer()->currentCompletion());

    QComboBox::focusOutEvent(e);
  }
};

} // end of namespace gui

#endif // AUTOFILTERCOMBOBOX_H
