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
    insertionOnFocusOut();
    QComboBox::focusOutEvent(e);
  }

  void insertionOnFocusOut (void) {
    if (QComboBox::NoInsert == insertPolicy()) {
      setCurrentText(completer()->currentCompletion());
      return;
    }

//    QDebug q = qDebug().nospace();
//    q << completer()->currentCompletion() << "\n";
//    q << lineEdit()->text() << "\n";
//    q << completer()->currentRow() << "\n";
//    q << findText(completer()->currentCompletion()) << "\n";
//    q << findText(lineEdit()->text()) << "\n";

    if (!completer()->currentCompletion().isEmpty()) {
      setCurrentIndex(findText(completer()->currentCompletion()));

    } else {
      QString text = lineEdit()->text();
      int index;
      if (!duplicatesEnabled()) {
        index = findText(text);
        if (index != -1) {
          setCurrentIndex(index);
          return;
        }
      }

      index = count();
      insertItem(index, lineEdit()->text());
      setCurrentIndex(index);
    }
  }
};

} // end of namespace gui

#endif // AUTOFILTERCOMBOBOX_H
