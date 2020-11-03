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

#include "../db/recipedata.h"

namespace gui {

static const QString windowName = "Gestionnaire d'update";

struct ProgressLabel : public QLabel {

};

UpdateManager::UpdateManager(QWidget *parent) : QDialog(parent) {
  QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *srclayout = new QHBoxLayout;
      srclayout->addWidget(new QLabel ("Dossier source: "));
      srclayout->addWidget(_path = new QLineEdit);
      srclayout->addWidget(new QToolButton);
    layout->addLayout(srclayout);

    _splitter = new QSplitter (Qt::Vertical);
      QWidget *lholder = new QWidget;
      QHBoxLayout *llayout = new QHBoxLayout;

        llayout->addWidget(new QWidget, 1);
        llayout->addWidget(_buttons.pull = new QPushButton("Pull"));
//        _buttons.pull->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        llayout->addWidget(_labels.pull = new QLabel);
        _labels.pull->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        llayout->addSpacing(10);

        llayout->addWidget(_buttons.compile = new QPushButton("Compiler"));
        llayout->addWidget(_labels.compile = new QLabel);
        _labels.compile->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        llayout->addSpacing(10);

        llayout->addWidget(_buttons.deploy = new QPushButton("Relancer"));
        llayout->addWidget(new QWidget, 1);

//        for (QLabel *l: {_labels.pull, _labels.compile, _labels.deploy})
//          l->setFixedSize(db::iconSize(), db::iconSize());

      lholder->setLayout(llayout);
      lholder->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
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

  connect(_buttons.pull, &QPushButton::clicked,
          this, &UpdateManager::doPull);
  connect(_buttons.compile, &QPushButton::clicked,
          this, &UpdateManager::doCompile);
  connect(_buttons.deploy, &QPushButton::clicked,
          this, &UpdateManager::doDeploy);

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

  setWindowTitle(windowName);
}

bool ok (int exitCode, QProcess::ExitStatus exitStatus) {
  return exitStatus == QProcess::NormalExit && exitCode == 0;
}

void setStatus (QLabel *l, int s) {
  static QMap<int, const QIcon*> codes {
//    {  0, Qt::gray  },
    {  0, &db::at<db::StatusData>(db::ID(1)).decoration  },
    {  1, &db::at<db::StatusData>(db::ID(3)).decoration  }
  };
  if (s == -1) l->setPixmap(QPixmap());
  else
    l->setPixmap(codes.value(s)->pixmap(l->size()));
}

void UpdateManager::doPull(void) {
  process(_labels.pull, "git", "pull", ".");
}

void UpdateManager::doCompile(void) {
  auto *p = process(_labels.compile, "qmake",
                    "CookBook.pro -spec linux-g++ CONFIG+=release -- -j 3");
  connect(p, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
          [this] (int exitCode, QProcess::ExitStatus exitStatus) {
    if (ok(exitCode, exitStatus))
      process(_labels.compile, "make", "", "build_debug");
  });
}

void UpdateManager::doDeploy(void) {
  if (parentWidget()->close()) {
    qApp->exit(RebootCode);
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
  }
}

void UpdateManager::logOutput (void) {
  QProcess *p = qobject_cast<QProcess*>(sender());
  auto output = p->readAllStandardOutput();
  qDebug() << output;
  _output->append(output);
}

QProcess*
UpdateManager::process(QLabel *monitor,
                       const QString &program, const QStringList &arguments,
                       const QString &relativeWorkPath) {

  static constexpr auto decoration =
    "*************************";

  QProcess *process = new QProcess(this);
  process->setProcessChannelMode(QProcess::MergedChannels);
  process->setWorkingDirectory(_path->text() + "/" + relativeWorkPath);
  connect(process, &QProcess::readyReadStandardOutput,
          this, &UpdateManager::logOutput);

  _output->append(decoration);
  _output->append("Executing " + program + " " + arguments.join(" "));
  _output->append("In " + process->workingDirectory());
  _output->append("\n");

  process->start(program, arguments);
  working(program);

  connect(process, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
          [this, monitor, process]
          (int exitCode, QProcess::ExitStatus exitStatus){

    QString s;
    QTextStream qss (&s);
    bool _ok = ok(exitCode, exitStatus);
    qss << "Exit status: " << exitStatus;
    _output->append(s);
    s.clear();
    qss << "  Exit code: " << exitCode;
    _output->append(s);
    s.clear();
    qss << "         OK? " << _ok;
    _output->append(s);
    s.clear();

    setStatus(monitor, _ok);

    _output->append(decoration);
    _output->append("");
  });
  connect(process, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
          process, &QProcess::deleteLater);

  connect(process, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
          this, &UpdateManager::stoppedWorking);

  return process;
}

void UpdateManager::closeEvent(QCloseEvent *e) {
  auto &settings = localSettings(this);
  gui::saveGeometry(this, settings);
  settings.setValue("splitter", _splitter->saveState());
  QDialog::closeEvent(e);
}

void UpdateManager::working(const QString &name) {
  setWindowTitle("Travail en cours: " + name);
  for (QPushButton *b: {_buttons.pull,_buttons.compile,_buttons.deploy})
    b->setEnabled(false);
}

void UpdateManager::stoppedWorking(void) {
  setWindowTitle(windowName);
  for (QPushButton *b: {_buttons.pull,_buttons.compile,_buttons.deploy})
    b->setEnabled(true);
}

} // end of namespace gui
