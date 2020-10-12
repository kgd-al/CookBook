#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QDialog>

#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>

#include <QProcess>

namespace gui {

class UpdateManager : public QDialog {
public:
  UpdateManager(QWidget *parent);

private:
  QLineEdit *_path;
  QTextEdit *_output;

  struct {
    QLabel *pull, *compile, *deploy;
  } _labels;


  void doPull (void);
  void doCompile (void);
  void doDeploy (void);

  struct ProcessReport {
    bool qprocess = true;
    QProcess::ExitStatus exit_status = QProcess::NormalExit;
    int return_value = 0;

    bool ok (void) const {
      return qprocess
          && (exit_status == QProcess::NormalExit)
          && (return_value == 0);
    }
  };

  ProcessReport process (const QString &program, const QStringList &arguments,
                         const QString &relativeWorkPath = ".");
  ProcessReport process (const QString &program, const QString &arguments,
                         const QString &relativeWorkPath = ".") {
    return process(program, arguments.split(' ', QString::SkipEmptyParts),
                   relativeWorkPath);
  }

  void logOutput (void);
};

} // end of namespace gui

#endif // UPDATEMANAGER_H
