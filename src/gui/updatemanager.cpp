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

    int r = 0, c = 0;
    layout->addWidget(new QLabel(tr("Gestion de périphériques")),
                      r++, c, 1, -1);

    layout->addWidget(_deviceCBox = new QComboBox, r++, c, 1, -1);
    layout->addWidget(_storageCBox = new QComboBox, r++, c, 1, -1);

    layout->addWidget(new QLabel("Distant:"), r, c++);
    layout->addWidget(_rlabel = new QLabel, r, c++);
    r++; c = 0;

    layout->addWidget(new QLabel("  Local:"), r, c++);
    layout->addWidget(_llabel = new QLabel, r, c++);
    r++; c = 0;

    layout->addWidget(_log = new QLabel, r++, 0, 1, -1, Qt::AlignCenter);
    _log->setStyleSheet("color: red");

    QHBoxLayout *blayout =  new QHBoxLayout;
    blayout->addStretch(1);
    blayout->addWidget(_refresh = new QPushButton("Refresh"));
    blayout->addWidget(_send = new QPushButton("Send"));
    blayout->addStretch(1);
    layout->addLayout(blayout, r++, 0, 1, -1);

    layout->addWidget(new QWidget, 0, 2, -1, 1);

    setLayout(layout);

    _deviceCBox->setMinimumContentsLength(1);
    _deviceCBox->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
    _deviceCBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    _storageCBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

    _llabel->setText(db::Book::monitoredName() + " ("
                     + localModification().toString() + ")");

    connect(_deviceCBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this] {  updateState(_deviceCBox); });
    connect(_storageCBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            [this] {  updateState(_storageCBox);  });
    connect(_send, &QPushButton::clicked, this, &PhoneDumper::sendToPhone);
    connect(_refresh, &QPushButton::clicked, this, &PhoneDumper::findDevices);

    if (findDevices() != 0)
      qWarning("Failed to detect mtp devices");
//    send->setEnabled(_cbox->currentIndex() >= 0);
  }

  ~PhoneDumper(void) {
    for (Device &d: _devices)
      LIBMTP_Release_Device(d.device);
  }

private:
  struct Storage {
    QString name = "No name";
    LIBMTP_devicestorage_t *storage = NULL;
    LIBMTP_file_t *target = NULL;

    bool found (void) const {
      return target != NULL;
    }
  };

  struct Device {
    QString name;
    LIBMTP_mtpdevice_t *device;
    QList<Storage> storages;

    static Device makeFrom (LIBMTP_raw_device_t &d) {
      static constexpr auto FC = QChar('0');
      QString name;
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
      auto device = LIBMTP_Open_Raw_Device_Uncached(&d);
      if (device == NULL)
        qWarning("Unable to open %s\n", name.toStdString().c_str());
      return Device(name, device);
    }

    explicit operator bool (void) const {
      return device != NULL;
    }


  private:
    Device (const QString &n, LIBMTP_mtpdevice_t *d) : name(n), device(d) {}
  };
  QList<Device> _devices;

  QComboBox *_deviceCBox, *_storageCBox;
  QLabel *_rlabel, *_llabel, *_log;
  QPushButton *_refresh, *_send;

  int findDevices (void) {
    LIBMTP_raw_device_t *rawdevices;
    int numrawdevices;
    LIBMTP_error_number_t err;

    LIBMTP_Init();

    _devices.clear();
    _deviceCBox->clear();
    _storageCBox->clear();
    _rlabel->clear();

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
      for (int i = 0; i < numrawdevices; i++)
        if (Device d = Device::makeFrom(rawdevices[i]))
          _devices.push_back(d);
      break;
    case LIBMTP_ERROR_GENERAL:
    default:
      qDebug() << "Unknown connection error.\n";
      return 1;
    }

    _deviceCBox->setCurrentIndex(-1);
    for (Device &d: _devices) {
      q << "Accessing files of " << d.name << "\n";
      for (auto storage = d.device->storage; storage; storage = storage->next) {
        Storage s;
        s.name = storage->StorageDescription;
        s.storage = storage;
        LIBMTP_file_t *files
          = LIBMTP_Get_Files_And_Folders(d.device,
                                         storage->id,
                                         LIBMTP_FILES_AND_FOLDERS_ROOT);
        if (files != NULL) {
          LIBMTP_file_t *file = files;
          while (file != NULL && !s.found()) {
            if (file->filename == db::Book::monitoredName())
              s.target = file;
            file = file->next;
          }
        }

        if (s.found())
          q << ">> Found target: " << s.target->filename << ":\n"
            << "\tiid: " << s.target->item_id << "\n"
            << "\tpid: " << s.target->parent_id << "\n"
            << "\tsid: " << s.target->storage_id << "\n"
            << "\ttype: " << s.target->filetype << "\n"
            << "\n\tssid: " << s.storage->id << "\n";

        d.storages.push_back(s);
      }

      q << "--\n";
      _deviceCBox->addItem(d.name);
    }

    free(rawdevices);
    return 0;
  }

  bool sendToPhone (void) {
    auto q = qDebug().nospace();
    q << "Trying to send " << db::Book::monitoredPath()
      << " to device " << device()->name << " " << storage()->name << "\n";
    auto source = db::Book::monitoredPath();
    QFileInfo sourceInfo (source);
    LIBMTP_file_t *file = target();
    if (!file) {
      file = LIBMTP_new_file_t();
      file->filesize = sourceInfo.size();
      file->filename = strdup(sourceInfo.fileName().toStdString().c_str());
      file->filetype = LIBMTP_FILETYPE_UNKNOWN;
      file->parent_id = 0;
      file->storage_id = storage()->storage->id;
      q << ">> Creating new file from extrapolated metadata\n";

    } else {
      LIBMTP_Delete_Object(device()->device, file->item_id);
      q << ">> Deleting existing target file and using its metadata\n";
    }

    auto r = LIBMTP_Send_File_From_File(device()->device,
                                        source.toStdString().c_str(),
                                        file, NULL, NULL);
    if (r != 0) {
      _log->setText("Error sending file");
      LIBMTP_Dump_Errorstack(device()->device);
      LIBMTP_Clear_Errorstack(device()->device);

    } else {
      _log->setText("File successfully sent");
      updateState(nullptr);
    }

    if (file != target()) LIBMTP_destroy_file_t(file);
    return true;
  }

  void updateState (QWidget *source) {
    if (source == _deviceCBox) {
      _storageCBox->clear();
      int i = _deviceCBox->currentIndex();
      if (i >= 0)
        for (const Storage &s: _devices.at(i).storages)
          _storageCBox->addItem(s.name);
    }

    QString error;
    if (_devices.empty())
      error = "No devices detected. Is the phone plugged in?";
    else if (device() == nullptr)
      error = "No devices selected";
    else if (device()->storages.empty())
      error = "No storage detected. Is file sharing activated on the phone?";
    else if (storage() == nullptr)
      error = "No storage selected";
    else {
      auto file = target();
      if (!file)
        _rlabel->setText("No database found");
      else {
        QString filedata;
        QTextStream qts (&filedata);
        qts << file->filename << " ("
            << remoteModification().toString()
            << ")";
        _rlabel->setText(filedata);

        auto lm = localModification(), rm = remoteModification();
        if (rm < lm)
          _log->setText("Phone version is outdated");
        else if (rm == lm)
          error = "No update necessary";
        else
          error = "Phone version is more recent!";
      }
    }

    if (!error.isEmpty())  _log->setText(error);

    _send->setEnabled(error.isEmpty());
  }

  QDateTime localModification (void) const {
    return QFileInfo(db::Book::monitoredPath()).lastModified();
  }

  QDateTime remoteModification (void) {
    if (!target())  return QDateTime();
    return QDateTime::fromSecsSinceEpoch(target()->modificationdate);
  }

  Device* device (void) {
    int i = _deviceCBox->currentIndex();
    if (i < 0)  return nullptr;
    return &_devices[i];
  }

  Storage* storage (void) {
    Device *d = device();
    if (d == nullptr) return nullptr;
    int i = _storageCBox->currentIndex();
    if (i < 0)  return nullptr;
    return &d->storages[i];
  }

  LIBMTP_file_t* target (void) {
    Storage *s = storage();
    if (s == nullptr) return nullptr;
    return s->target;
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
        int lr = 0, lc = 0;
        llayout->addWidget(new QLabel("Actions"), lr++, lc, 1, 2);

        llayout->addWidget(_buttons.pull = new QPushButton("Pull"), lr, lc++);
        llayout->addWidget(_labels.pull = new ProgressLabel, lr, lc++);
//        llayout->addSpacing(10);

        lr++; lc = 0;
        llayout->addWidget(_buttons.compile = new QPushButton("Compiler"), lr, lc++);
        llayout->addWidget(_labels.compile = new ProgressLabel, lr, lc++);
//        llayout->addSpacing(10);

        lr++; lc = 0;
        llayout->addWidget(_buttons.deploy = new QPushButton("Relancer"), lr, lc++);
        llayout->addWidget(_labels.deploy = new ProgressLabel, lr, lc++);

        lr++; lc = 0;
        llayout->addWidget(_buttons.push = new QPushButton("Push"), lr, lc++);
        llayout->addWidget(_labels.push = new ProgressLabel, lr, lc++);

//        llayout->addWidget(new QWidget, 0, 2, -1, 1);

        int rr = 0, rc = 3;
        llayout->addWidget(new QLabel("Dossiers"), rr++, rc, 1, 1);
        llayout->addWidget(_buttons.data = new QPushButton("Local"), rr++, rc);
        llayout->addWidget(_buttons.phone = new QPushButton("Phone"), rr++, rc);


        llayout->addWidget(_phone = new PhoneDumper,
                           std::max(lr, rr)+1, 0, 1, -1);

//        llayout->addWidget(new QWidget, 0, 4, -1, 1);

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
