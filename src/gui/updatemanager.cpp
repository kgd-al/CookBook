#include <QLayout>
#include <QSplitter>

#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QDialogButtonBox>

#include <QApplication>
#include <QStandardPaths>
#include <QFile>
#include <QProcess>
#include <QPainter>

#include <QDebug>

#include "updatemanager.h"
#include "common.h"

#include "../db/recipedata.h"

namespace gui {

static const QString windowName = "Gestionnaire de mise à jour";

struct ProgressLabel : public QLabel {
  double progress;
  enum State { PENDING = 0, OK = 1, ERROR = 2 };
  State state;

  ProgressLabel (QWidget *parent = nullptr)
    : QLabel(parent), progress(1), state(PENDING) {}

  void setState (State state, float progress = 1) {
    this->state = state;
    this->progress = progress;
    update();
  }

  QSize sizeHint(void) const override {
    return db::iconQSize();
  }

  void paintEvent(QPaintEvent*) {
    static const QMap<State, QColor> c {
      { PENDING, Qt::gray   },
      {      OK, Qt::green  },
      {   ERROR, Qt::red    },
    };

    QPainter p (this);
    QRectF r = rect();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setBrush(c.value(state));
    p.setPen(Qt::NoPen);
    p.drawPie(r, 0, progress*360*16);
  }
};

UpdateManager::UpdateManager(QWidget *parent) : QDialog(parent) {
  QVBoxLayout *layout = new QVBoxLayout;
    QHBoxLayout *srclayout = new QHBoxLayout;
      srclayout->addWidget(new QLabel ("Dossier source: "));
      auto path = new QLineEdit (BASE_DIR);
      srclayout->addWidget(path);
      path->setReadOnly(true);
    layout->addLayout(srclayout);

    QHBoxLayout *scaleLayout = new QHBoxLayout;
      scaleLayout->addWidget(new QLabel("Scale"));
      scaleLayout->addWidget(_scale = new QDoubleSpinBox);
      _scale->setMinimum(1);
      _scale->setSingleStep(.5);
    layout->addLayout(scaleLayout);

    _splitter = new QSplitter (Qt::Vertical);
      QWidget *lholder = new QWidget;
      QHBoxLayout *llayout = new QHBoxLayout;

        llayout->addWidget(new QWidget, 1);
        llayout->addWidget(_buttons.pull = new QPushButton("Pull"));
//        _buttons.pull->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        llayout->addWidget(_labels.pull = new ProgressLabel);
        _labels.pull->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        llayout->addSpacing(10);

        llayout->addWidget(_buttons.compile = new QPushButton("Compiler"));
        llayout->addWidget(_labels.compile = new ProgressLabel);
        _labels.compile->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        llayout->addSpacing(10);

        llayout->addWidget(_buttons.deploy = new QPushButton("Relancer"));
        llayout->addWidget(_labels.deploy = new ProgressLabel);
        _labels.deploy->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        llayout->addWidget(_buttons.push = new QPushButton("Push"));
        llayout->addWidget(_labels.push = new ProgressLabel);
        _labels.push->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

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
  connect(_buttons.push, &QPushButton::clicked,
          this, &UpdateManager::doPush);

  auto &settings = localSettings(this);
  gui::restoreGeometry(this, settings);
  _splitter->restoreState(settings.value("splitter").toByteArray());
  _scale->setValue(settings.value("scale", 1).toFloat());

  connect(_scale, &QDoubleSpinBox::editingFinished,
          [this] {
    auto &settings = localSettings(this);
    settings.setValue("scale", _scale->value());
  });

  setWindowTitle(windowName);
}

bool ok (int exitCode, QProcess::ExitStatus exitStatus) {
  return exitStatus == QProcess::NormalExit && exitCode == 0;
}

void UpdateManager::doPull(void) {
  process(_labels.pull, 1, "git", "pull", ".");
}

void UpdateManager::doCompile(void) {
  auto *p = process(_labels.compile, .5, "qmake",
                    "CookBook.pro -spec linux-g++ CONFIG+=release -- -j 3");
  connect(p, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
          [this] (int exitCode, QProcess::ExitStatus exitStatus) {
    if (ok(exitCode, exitStatus))
      process(_labels.compile, 1, "make", "", "build_debug");
  });
}

void UpdateManager::doDeploy(void) {
  if (!parentWidget()->close()) {
    _labels.deploy->setState(ProgressLabel::ERROR, 1);
    return;
  }

  QApplication *app = qApp;
#ifdef Q_OS_LINUX
  QString l = QStandardPaths::writableLocation(
                QStandardPaths::ApplicationsLocation);
  if (l.isEmpty()) {
    qInfo("No standard location for writing .desktop file. Autolaunch aborted");
    return;
  }

  QFile dfile (l + "/" + app->desktopFileName());
  dfile.open(QIODevice::WriteOnly);
  if (!dfile.isWritable()) {
    _output->append("Failed to write to " + dfile.fileName());
    _labels.deploy->setState(ProgressLabel::ERROR, .5);
    return;
  }

  QTextStream qts (&dfile);
  qts << "[Desktop Entry]\n"
         "Type=Application\n"
      << "Name=" << app->applicationDisplayName() << "\n"
      << "Version=" << app->applicationVersion() << "\n"
      << "Comment=Malenda's recipes (version " << app->applicationVersion()
        << ")\n"
      << "Exec=QT_SCALE_FACTOR=" << QString::number(_scale->value(), 'f', 2)
        << " " << BASE_BUILD_DIR << "/CookBook\n"
      << "Icon=" << BASE_DIR << "/icons/book.png\n"
      << "MimeType=\n"
      << "Category=Qt;KDE;Utility\n";

  _output->append("Generated desktop entry " + dfile.fileName());
  _labels.deploy->setState(ProgressLabel::OK, .5);

  if (parentWidget()->close()) {
    QProcess::startDetached(
      "kioclient5", QStringList() << "exec" << dfile.fileName());
  }
#else
  app->exit(RebootCode);
  qDebug() << "Launching " << app->arguments()[0] << app->arguments();
  QProcess::startDetached(app->arguments()[0], app->arguments());
  _labels.deploy->setState(ProgressLabel::OK, 1);
#endif
}

void UpdateManager::doPush(void) {
  auto *p = process(_labels.push, .5, "git",
                    QStringList() << "commit"
                      << "-m" << "Updated recipes database"
                      << "--"  << "main.rbk");
  connect(p, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
          [this] (int exitCode, QProcess::ExitStatus exitStatus) {
    if (ok(exitCode, exitStatus))
      process(_labels.push, 1, "git", "push");
  });
}

void UpdateManager::logOutput (void) {
  QProcess *p = qobject_cast<QProcess*>(sender());
  auto output = p->readAllStandardOutput();
  qDebug() << output;
  _output->append(output);
}

QProcess*
UpdateManager::process(ProgressLabel *monitor, float progress,
                       const QString &program, const QStringList &arguments,
                       const QString &relativeWorkPath) {

  static constexpr auto decoration =
    "*************************";

  QProcess *process = new QProcess(this);
  process->setProcessChannelMode(QProcess::MergedChannels);
  process->setWorkingDirectory(QString(BASE_DIR) + "/" + relativeWorkPath);
  connect(process, &QProcess::readyReadStandardOutput,
          this, &UpdateManager::logOutput);

  _output->append(decoration);
  _output->append("Executing " + program + " " + arguments.join(" "));
  _output->append("In " + process->workingDirectory());
  _output->append("\n");

  process->start(program, arguments);
  working(program);

  connect(process, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
          [this, monitor, progress, process]
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

    monitor->setState(_ok ? ProgressLabel::OK : ProgressLabel::ERROR, progress);

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
