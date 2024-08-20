#ifndef GUI_SETTINGS_H
#define GUI_SETTINGS_H

#include <QDialog>

namespace gui {

class SettingsView : public QDialog {
  Q_OBJECT
public:
  SettingsView(QWidget *parent);
};

} // end of namespace gui

#endif // GUI_SETTINGS_H
