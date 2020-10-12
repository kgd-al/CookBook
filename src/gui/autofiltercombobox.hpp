#ifndef AUTOFILTERCOMBOBOX_H
#define AUTOFILTERCOMBOBOX_H

#include <QComboBox>
#include <QCompleter>

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
};

} // end of namespace gui

#endif // AUTOFILTERCOMBOBOX_H
