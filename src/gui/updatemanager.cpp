#include <QLayout>
#include <QSplitter>

#include <QLabel>
#include <QToolButton>
#include <QPushButton>
#include <QDialogButtonBox>

#include <QApplication>
#include <QProcess>

#include <QDebug>

#include "updatemanager.h"
#include "common.h"

namespace gui {

UpdateManager::UpdateManager(QWidget *parent) : QDialog(parent) {
  struct {
    QPushButton *pull, *compile, *deploy;
  } qpb;

  QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *srclayout = new QHBoxLayout;
      srclayout->addWidget(new QLabel ("Dossier source: "));
      srclayout->addWidget(_path = new QLineEdit);
      srclayout->addWidget(new QToolButton);
    layout->addLayout(srclayout);

    _splitter = new QSplitter (Qt::Vertical);
      QWidget *lholder = new QWidget;
      QGridLayout *llayout = new QGridLayout;
        int r = 0, c = 0;

        llayout->addWidget(qpb.pull = new QPushButton("Pull"), r, c++);
        llayout->addWidget(_labels.pull = new QLabel, r, c++);
        r++; c = 0;

        llayout->addWidget(qpb.compile = new QPushButton("Compiler"), r, c++);
        llayout->addWidget(_labels.compile = new QLabel, r, c++);
        r++; c = 0;

        llayout->addWidget(qpb.deploy = new QPushButton("Relancer"), r, c++);
        llayout->addWidget(_labels.deploy = new QLabel, r, c++);
        r++; c = 0;

      lholder->setLayout(llayout);
    _splitter->addWidget(lholder);

    _splitter->addWidget(_output = new QTextEdit);
    _splitter->setStretchFactor(1, 1);
  layout->addWidget(_splitter);

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

  auto &settings = localSettings(this);
  QString path = settings.value("src").toString();
  if (path.isEmpty()) path = BASE_DIR;
  _path->setText(path);
  connect(_path, &QLineEdit::textEdited, [this] (const QString &path) {
    qDebug() << "path edited. new value: " << path;
    auto &settings = localSettings(this);
    settings.setValue("src", path);
  });

  gui::restoreGeometry(this, settings);
  _splitter->restoreState(settings.value("splitter").toByteArray());
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
  qApp->exit(RebootCode);
//  qApp->quit();
  QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
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

void UpdateManager::closeEvent(QCloseEvent *e) {
  auto &settings = localSettings(this);
  gui::saveGeometry(this, settings);
  settings.setValue("splitter", _splitter->saveState());
  QDialog::closeEvent(e);
}

} // end of namespace gui
