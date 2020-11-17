#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QDialog>

#include <QSplitter>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QTextEdit>
#include <QComboBox>

#include <QProcess>

namespace gui {

struct ProgressLabel;
struct PhoneDumper;

class UpdateManager : public QDialog {
  Q_OBJECT
public:
  static constexpr int RebootCode = 254;

  UpdateManager(QWidget *parent);
  ~UpdateManager (void) {}

private:
  QDoubleSpinBox *_scale;
  QComboBox *_buildType;

  QTextEdit *_output;
  QSplitter *_splitter;

  struct {
    ProgressLabel *pull, *compile, *deploy, *push;
  } _labels;

  struct {
    QPushButton *pull, *compile, *deploy, *push, *data, *phone;
  } _buttons;

  PhoneDumper *_phone;

  void doPull (void);
  void doCompile (void);
  void doDeploy (void);
  void doPush (void);

  QProcess* process (ProgressLabel *monitor, float progress,
                     const QString &program, const QStringList &arguments,
                     const QString &relativeWorkPath = ".");
  QProcess* process (ProgressLabel *monitor, float progress,
                     const QString &program, const QString &arguments,
                     const QString &relativeWorkPath = ".") {
    return process(monitor, progress,
                   program, arguments.split(' ', QString::SkipEmptyParts),
                   relativeWorkPath);
  }

  void logOutput (void);

  void closeEvent(QCloseEvent *e);

private:
  void working (const QString &name);
  void stoppedWorking (void);
};

} // end of namespace gui

#endif // UPDATEMANAGER_H
