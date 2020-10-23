#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QDialog>

#include <QSplitter>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>

#include <QProcess>

namespace gui {

class UpdateManager : public QDialog {
  Q_OBJECT
public:
  static constexpr int RebootCode = 254;

  UpdateManager(QWidget *parent);
  ~UpdateManager (void) {}

private:
  QLineEdit *_path;
  QTextEdit *_output;
  QSplitter *_splitter;

  struct {
    QLabel *pull, *compile, *deploy;
  } _labels;

  struct {
    QPushButton *pull, *compile, *deploy;
  } _buttons;


  void doPull (void);
  void doCompile (void);
  void doDeploy (void);

  QProcess* process (QLabel *monitor,
                     const QString &program, const QStringList &arguments,
                     const QString &relativeWorkPath = ".");
  QProcess* process (QLabel *monitor,
                     const QString &program, const QString &arguments,
                     const QString &relativeWorkPath = ".") {
    return process(monitor,
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
