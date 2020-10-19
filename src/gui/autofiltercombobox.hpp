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
      qDebug() << "\n@@@@\nInserting " << lineEdit()->text() << "in"
               << model();
      insertItem(index, lineEdit()->text());
      setCurrentIndex(index);

      QDebug q = qDebug().nospace();
      q << "Model " << model() << " of " << model()->rowCount() << " items\n";
      q << "Contents:\n";
      for (int i=0; i<count(); i++)
        q << itemText(i) << "\n";
      q << "\n";
      q << "Current index: " << currentIndex() << "\n"
        << " Current text: " << currentText() << "\n";
    }
  }
};

} // end of namespace gui

#endif // AUTOFILTERCOMBOBOX_H
