#ifndef SYNCHRONIZER_H
#define SYNCHRONIZER_H

#include <QDialog>

namespace gui {

class Synchronizer : public QDialog {
public:
  Synchronizer(QWidget *parent);
  ~Synchronizer (void) = default;

private:
  struct Data;
  Data *_data;

  void update(void);
};

} // end of namespace gui

#endif // SYNCHRONIZER_H
