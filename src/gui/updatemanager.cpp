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
#include <QDir>

#include <QDebug>

#include "libmtp.h"

#include "updatemanager.h"
#include "common.h"

#include "../db/book.h"

namespace gui {

static const QString windowName = "Gestionnaire de mise à jour";
static const QString buildDirName = "autobuild";

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

struct PhoneDumper : public QWidget {
  PhoneDumper (void) {
    auto *layout = new QGridLayout;
    _cbox = new QComboBox;
    QPushButton *ok = new QPushButton("Send");
    QPushButton *refresh = new QPushButton("Refresh");
    layout->addWidget(_cbox, 0, 0, 1, -1);
    layout->addWidget(refresh, 1, 0);
    layout->addWidget(ok, 1, 1);
    setLayout(layout);

    connect(_cbox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [ok] (int index) {
      ok->setEnabled(index >= 0);
    });
    connect(refresh, &QPushButton::clicked, this, &PhoneDumper::findDevices);

    if (findDevices() != 0)
      qWarning("Failed to detect mtp devices");
    ok->setEnabled(_cbox->currentIndex() >= 0);
  }

private:
  struct Device {
    QString name;
  };
  QList<Device> _devices;

  QComboBox *_cbox;

  int findDevices (void) {
    static constexpr auto FC = QChar('0');
    LIBMTP_raw_device_t *rawdevices;
    int numrawdevices;
    LIBMTP_error_number_t err;

    LIBMTP_Init();

    _devices.clear();
    _cbox->clear();

    auto q = qDebug().nospace();
    q << "libmtp version: " << LIBMTP_VERSION_STRING << "\n"
      << "Listing raw devices:\n";
    err = LIBMTP_Detect_Raw_Devices(&rawdevices, &numrawdevices);
    switch(err) {
    case LIBMTP_ERROR_NO_DEVICE_ATTACHED:
      q << "No raw devices found.\n";
      return 0;
    case LIBMTP_ERROR_CONNECTING:
      q << "Detect: There has been an error connecting. Exiting\n";
      return 1;
    case LIBMTP_ERROR_MEMORY_ALLOCATION:
      q << "Detect: Encountered a Memory Allocation Error. Exiting\n";
      return 1;
    case LIBMTP_ERROR_NONE:
      q << "Found " << numrawdevices << " device(s):\n";
      for (int i = 0; i < numrawdevices; i++) {
        QString name;
        auto d = rawdevices[i];
        if (d.device_entry.vendor != NULL || d.device_entry.product != NULL)
          name = QString("%1: %2 (%3:%4) @ bus %5, dev %6")
              .arg(d.device_entry.vendor).arg(d.device_entry.product)
              .arg(d.device_entry.vendor_id, 4, 16, FC)
              .arg(d.device_entry.product_id, 4, 16, FC)
              .arg(d.bus_location).arg(d.devnum);
        else
          name = QString("%1:%2 @ bus %3, dev %4")
              .arg(d.device_entry.vendor_id, 4, 16, FC)
              .arg(d.device_entry.product_id, 4, 16, FC)
              .arg(d.bus_location).arg(d.devnum);
        q << "\t" << name;
        _devices.push_back({name});
        }
      break;
    case LIBMTP_ERROR_GENERAL:
    default:
      qDebug() << "Unknown connection error.\n";
      return 1;
    }

    _cbox->setCurrentIndex(-1);
    for (const Device &d: _devices)
      _cbox->addItem(d.name);

    return 0;
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
      scaleLayout->addWidget(new QLabel("Échelle: "));
      scaleLayout->addWidget(_scale = new QDoubleSpinBox);
      _scale->setMinimum(1);
      _scale->setSingleStep(.5);
    layout->addLayout(scaleLayout);

    QHBoxLayout *buildTypeLayout = new QHBoxLayout;
      buildTypeLayout->addWidget(new QLabel("Build type: "));
      buildTypeLayout->addWidget(_buildType = new QComboBox);
      _buildType->addItems({"debug"/*,"release"*/});
    layout->addLayout(buildTypeLayout);

    _splitter = new QSplitter (Qt::Vertical);
      QWidget *lholder = new QWidget;
      auto *llayout = new QGridLayout;

//        llayout->addWidget(new QWidget, 1);
        int r = 0, c = 0;
        llayout->addWidget(new QLabel("Actions"), r++, c, 1, 2);

        llayout->addWidget(_buttons.pull = new QPushButton("Pull"), r, c++);
        llayout->addWidget(_labels.pull = new ProgressLabel, r, c++);
//        llayout->addSpacing(10);

        r++; c = 0;
        llayout->addWidget(_buttons.compile = new QPushButton("Compiler"), r, c++);
        llayout->addWidget(_labels.compile = new ProgressLabel, r, c++);
//        llayout->addSpacing(10);

        r++; c = 0;
        llayout->addWidget(_buttons.deploy = new QPushButton("Relancer"), r, c++);
        llayout->addWidget(_labels.deploy = new ProgressLabel, r, c++);

        r++; c = 0;
        llayout->addWidget(_buttons.push = new QPushButton("Push"), r, c++);
        llayout->addWidget(_labels.push = new ProgressLabel, r, c++);

        llayout->addWidget(new QWidget, 0, 2, -1, 1);

        r = 0; c = 3;
        llayout->addWidget(new QLabel("Dossiers"), r++, c, 1, 2);
        llayout->addWidget(_buttons.data = new QPushButton("Local"), r++, c);
        llayout->addWidget(_buttons.phone = new QPushButton("Phone"), r++, c);
        llayout->addWidget(_phone = new PhoneDumper, r++, c);

        llayout->addWidget(new QWidget, 0, 4, -1, 1);

        for (QLabel *l: { _labels.pull, _labels.compile,
                          _labels.deploy, _labels.push  })
          l->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        for (QPushButton *b: {  _buttons.pull, _buttons.compile,
                                _buttons.deploy, _buttons.push,
                                _buttons.data, _buttons.phone })
          b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

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

  connect(_buttons.data, &QPushButton::clicked,
          [this] {
    process(nullptr, 1, "xdg-open", db::Book::monitoredDir());
  });

  connect(_buttons.phone, &QPushButton::clicked,
          [this] {
    process(nullptr, 1,
            "xdg-open", QStringList() << "mtp:/HT16/Internal storage/");
  });

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
  QDir buildDir (QString(BASE_DIR) + "/" + buildDirName);
  if (!buildDir.exists())
    QDir(BASE_DIR).mkdir(buildDirName);

  auto *p = process(_labels.compile, .5, "qmake",
                    QStringList() << "../CookBook.pro"
                      << "-spec" << "linux-g++"
                    << "CONFIG+=" + _buildType->currentText(),
                    buildDirName);

  connect(p, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
          [this] (int exitCode, QProcess::ExitStatus exitStatus) {
    if (ok(exitCode, exitStatus))
      process(_labels.compile, 1, "make", "-j 3", buildDirName);
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
        << " " <<BASE_DIR << "/" << buildDirName << "/CookBook\n"
      << "Icon=" << BASE_DIR << "/icons/book.png\n"
      << "MimeType=\n"
      << "Category=Qt;KDE;Utility\n";

  _output->append("Generated desktop entry " + dfile.fileName());
  _labels.deploy->setState(ProgressLabel::OK, .5);

  if (db::Book::current().close()) {
    qApp->quit();
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
                      << "--"  << db::Book::monitoredPath());
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

    if (monitor)
      monitor->setState(_ok ? ProgressLabel::OK
                            : ProgressLabel::ERROR,
                        progress);

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
  for (QPushButton *b: { _buttons.pull, _buttons.compile,
                         _buttons.deploy, _buttons.push })
    b->setEnabled(false);
}

void UpdateManager::stoppedWorking(void) {
  setWindowTitle(windowName);
  for (QPushButton *b: { _buttons.pull, _buttons.compile,
                         _buttons.deploy, _buttons.push })
    b->setEnabled(true);
}

} // end of namespace gui
