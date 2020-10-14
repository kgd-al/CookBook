#ifndef INGREDIENTSMANAGER_H
#define INGREDIENTSMANAGER_H

#include <QDialog>

namespace gui {

class IngredientsManager : public QDialog {
  Q_OBJECT
public:
  IngredientsManager(QWidget *parent);
  ~IngredientsManager(void) {}

  void closeEvent(QCloseEvent *e);
};

} // end of namespace gui

#endif // INGREDIENTSMANAGER_H
