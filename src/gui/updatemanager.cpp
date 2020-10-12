#include <QLayout>
#include <QSplitter>

#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QDialogButtonBox>

#include <QSettings>
#include <QProcess>
#include <QDebug>

#include "updatemanager.h"

namespace gui {

UpdateManager::UpdateManager(QWidget *parent) : QDialog(parent) {
  struct {
    QPushButton *pull, *compile, *deploy;
  } qpb;

  QVBoxLayout *layout = new QVBoxLayout;
    QSplitter *splitter = new QSplitter;
      QWidget *lholder = new QWidget;
      QGridLayout *llayout = new QGridLayout;
        int r = 0, c = 0;
        llayout->addWidget(new QLabel ("Source path: "), r, c, Qt::AlignLeft);
        r++; c = 0;

        llayout->addWidget(new QWidget);
        r++; c = 0;

        llayout->addWidget(_path = new QLineEdit, r, c++);
        llayout->addWidget(new QToolButton, r, c++);
        r++; c = 0;

        llayout->addWidget(qpb.pull = new QPushButton("Pull"), r, c++);
        llayout->addWidget(_labels.pull = new QLabel, r, c++);
        r++; c = 0;

        llayout->addWidget(qpb.compile = new QPushButton("Compile"), r, c++);
        llayout->addWidget(_labels.compile = new QLabel, r, c++);
        r++; c = 0;

        llayout->addWidget(qpb.deploy = new QPushButton("Deploy"), r, c++);
        llayout->addWidget(_labels.deploy = new QLabel, r, c++);
        r++; c = 0;

      lholder->setLayout(llayout);
    splitter->addWidget(lholder);

    splitter->addWidget(_output = new QTextEdit);
    splitter->setStretchFactor(1, 1);
  layout->addWidget(splitter);

  QDialogButtonBox *buttons = new QDialogButtonBox;
//    auto update = buttons->addButton("Update", QDialogButtonBox::ActionRole);
//    connect(update, &QPushButton::clicked, this, &UpdateManager::process);
    auto close = buttons->addButton("Close", QDialogButtonBox::AcceptRole);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
  layout->addWidget(buttons);

  setLayout(layout);

  connect(qpb.pull, &QPushButton::clicked, this, &UpdateManager::doPull);
  connect(qpb.compile, &QPushButton::clicked, this, &UpdateManager::doCompile);
  connect(qpb.deploy, &QPushButton::clicked, this, &UpdateManager::doDeploy);

  QSettings settings;
  _path->setText(settings.value("srcPath").toString());
  connect(_path, &QLineEdit::textEdited, [] (const QString &path) {
    qDebug() << "path edited. new value: " << path;
    QSettings settings;
    settings.setValue("srcPath", path);
  });
}

void setStatus (QLabel *l, int s) {
  QMap<int, QString> codes {
//    {  0, Qt::gray  },
    {  0, "red"   },
    {  1, "green" }
  };
  if (s == -1) l->setStyleSheet("");
  else
    l->setStyleSheet("QLabel { background-color : " + codes.value(s) + "; }");
}

void UpdateManager::doPull(void) {
  auto report = process("git", "pull", ".");
  setStatus(_labels.pull, report.ok());
}

void UpdateManager::doCompile(void) {
  auto report = process("qmake",
                        "CookBook.pro -spec linux-g++ CONFIG+=release -- -j 3");
  if (!report.ok()) {
    setStatus(_labels.compile, 0);
    return;
  }
  report = process("make", "", "build_debug");
  setStatus(_labels.compile, report.ok());
}

void UpdateManager::doDeploy(void) {

}

void UpdateManager::logOutput (void) {
  QProcess *p = qobject_cast<QProcess*>(sender());
  _output->append(p->readAllStandardOutput());
}

UpdateManager::ProcessReport
UpdateManager::process(const QString &program, const QStringList &arguments,
                       const QString &relativeWorkPath) {

  static constexpr auto decoration =
    "*************************";

  QProcess process;
  process.setProcessChannelMode(QProcess::MergedChannels);
  process.setWorkingDirectory(_path->text() + "/" + relativeWorkPath);
  connect(&process, &QProcess::readyReadStandardOutput,
          this, &UpdateManager::logOutput);

  _output->append(decoration);
  _output->append("Executing " + program + arguments.join(" "));
  _output->append("In " + process.workingDirectory());
  _output->append("\n");

  process.start(program, arguments);

  ProcessReport r;
  r.qprocess = process.waitForFinished();
  r.exit_status = process.exitStatus();
  r.return_value = process.exitCode();

  QString s;
  QTextStream qss (&s);
  qss << "QProcess Ok? " << r.qprocess;
  _output->append(s);
  s.clear();
  qss << "Exit status: " << r.exit_status;
  _output->append(s);
  s.clear();
  qss << "  Exit code: " << r.return_value;
  _output->append(s);
  s.clear();
  qss << "         OK? " << r.ok();
  _output->append(s);
  s.clear();

  _output->append(decoration);
  _output->append("");

  return r;
}

} // end of namespace gui
