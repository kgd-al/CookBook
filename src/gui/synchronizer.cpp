#include <QTabWidget>
#include <QPushButton>
#include <QFormLayout>
#include <QLabel>

#include <QFileInfo>

#include <QBluetoothServer>

#include "synchronizer.h"
#include "../db/book.h"

#include <iostream>

namespace gui {

enum TABS {
  CLIENT = 0,
  SERVER = 1
};

struct Synchronizer::Data {
  QLabel *status, *localDate, *remoteDate;
  QTabWidget *tabs;

  QBluetoothAddress address;
  QBluetoothServer *server;
};

struct Synchronizer::Worker : public QObject {
  Synchronizer &parent;
  Data &data;

  Worker(Synchronizer *parent) : parent(*parent), data(*parent->_data) {}

  void panelChanged() {
    std::cerr << "Panel changed" << std::endl;
    auto i = data.tabs->currentIndex();
    data.status->setText(i == TABS::CLIENT ? "Client mode" : "Server mode");
  }

  void startServer() {
    data.server = new QBluetoothServer(QBluetoothServiceInfo::RfcommProtocol, &parent);
    connect(data.server, &QBluetoothServer::newConnection, this, &Worker::newConnection);
  }

  void newConnection(void) {
    data.status->setText("New connection");
  }
};

Synchronizer::Synchronizer(QWidget *parent) : QDialog(parent) {
  _data = new Data();
  _worker = new Worker(this);

  _data->address = QBluetoothAddress();

  _data->status = new QLabel();
  _data->localDate = new QLabel();
  _data->remoteDate = new QLabel();

  auto layout = new QVBoxLayout();

  auto tabWidget = _data->tabs = new QTabWidget();
  layout->addWidget(tabWidget);

  auto serverWidget = new QWidget();
  tabWidget->insertTab(TABS::SERVER, serverWidget, "Server");

  auto clientWidget = new QWidget();
  tabWidget->insertTab(TABS::CLIENT, clientWidget, "Client");

  connect(tabWidget, &QTabWidget::currentChanged, _worker, &Worker::panelChanged);

  auto statusLayout = new QFormLayout();
  layout->addLayout(statusLayout);

  statusLayout->addRow("Status", _data->status);
  statusLayout->addRow("Local", _data->localDate);
  statusLayout->addRow("Remote", _data->remoteDate);

  auto close = new QPushButton("Close");
  layout->addWidget(close, 0, Qt::AlignRight);
  connect(close, &QPushButton::clicked, this, &QDialog::accept);

  setLayout(layout);
  setWindowTitle("Synchronisation");

  _worker->panelChanged();

  adjustSize();
  update();
}

void Synchronizer::update(void) {
  _data->localDate->setText(QFileInfo(db::Book::monitoredPath()).lastModified().toString());
}

} // end of namespace gui


// struct DevicesManager : public QWidget {
//   DevicesManager (void) {
//     auto *layout = new QGridLayout;

//     int r = 0, c = 0;
//     layout->addWidget(new QLabel(tr("Gestion de périphériques")),
//                       r++, c, 1, -1);

//     layout->addWidget(_deviceCBox = new QComboBox, r++, c, 1, -1);
//     layout->addWidget(_storageCBox = new QComboBox, r++, c, 1, -1);

//     layout->addWidget(new QLabel("Distant:"), r, c++);
//     layout->addWidget(_rlabel = new QLabel, r, c++);
//     r++; c = 0;

//     layout->addWidget(new QLabel("  Local:"), r, c++);
//     layout->addWidget(_llabel = new QLabel, r, c++);
//     r++; c = 0;

//     layout->addWidget(_log = new QLabel, r++, 0, 1, -1, Qt::AlignCenter);
//     _log->setStyleSheet("color: red");

//     QHBoxLayout *blayout =  new QHBoxLayout;
//     blayout->addStretch(1);
//     blayout->addWidget(_refresh = new QPushButton("Refresh"));
//     blayout->addWidget(_send = new QPushButton("Send"));
//     blayout->addStretch(1);
//     layout->addLayout(blayout, r++, 0, 1, -1);

//     layout->addWidget(new QWidget, 0, 2, -1, 1);

//     setLayout(layout);

//     _deviceCBox->setMinimumContentsLength(1);
//     _deviceCBox->setSizeAdjustPolicy(QComboBox::AdjustToContentsOnFirstShow);
//     _deviceCBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
//     _storageCBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

//     _llabel->setText(db::Book::monitoredName() + " ("
//                      + localModification().toString() + ")");

//     connect(_deviceCBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
//             [this] {  updateState(_deviceCBox); });
//     connect(_storageCBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
//             [this] {  updateState(_storageCBox);  });
//     // connect(_send, &QPushButton::clicked, this, &DevicesManager::sendToPhone);
//     connect(_refresh, &QPushButton::clicked, this, &DevicesManager::findDevices);

//     if (findDevices() != 0)
//       qWarning("Failed to detect mtp devices");
//   }

//   ~DevicesManager(void) {
//   }

//   private:
//   // QList<Device> _devices;
//   // QList<QBluetoothDeviceInfo> _bluetoothDevices;

//   QComboBox *_deviceCBox, *_storageCBox;
//   QLabel *_rlabel, *_llabel, *_log;
//   QPushButton *_refresh, *_send;

//   // void devicesDiscovered(const QBluetoothDeviceDiscoveryAgent *agent) {
//   //   _bluetoothDevices = agent->discoveredDevices();
//   //   qDebug() << "Discovered" << _bluetoothDevices.size() << "devices";
//   //   for (const auto &device: _bluetoothDevices)
//   //     qDebug() << ">" << device.name() << '(' << device.address().toString() << ')';
//   // }

//   int findDevices (void) {
//     // auto agent = new QBluetoothDeviceDiscoveryAgent(this);
//     // connect(agent, &QBluetoothDeviceDiscoveryAgent::finished,
//     //         [this, agent] { devicesDiscovered(agent); });
//     // connect(agent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
//     //         [agent] (QBluetoothDeviceDiscoveryAgent::Error) {
//     //   qDebug() << "Error discovering bluetooth devices:" << agent->errorString();
//     // });
//     // qDebug() << "Looking for bluetooth devices";
//     // agent->start();

//     auto thisDevice = new QBluetoothLocalDevice(this);
//     qDebug() << "connected devices:" << thisDevice->connectedDevices();

//     updateState(nullptr);
//     return 0;
//   }

//   //   bool sendToPhone (void) {
//   //     qDebug() << "Trying to send " << db::Book::monitoredPath()
//   //              << " to device " << device()->name << " " << storage()->name;
//   //     auto source = db::Book::monitoredPath();
//   //     QFileInfo sourceInfo (source);
//   //     LIBMTP_file_t *file = target();
//   //     if (!file) {
//   //       file = LIBMTP_new_file_t();
//   //       file->filesize = sourceInfo.size();
//   //       file->filename = strdup(sourceInfo.fileName().toStdString().c_str());
//   //       file->filetype = LIBMTP_FILETYPE_UNKNOWN;
//   //       file->parent_id = 0;
//   //       file->storage_id = storage()->storage->id;
//   //       qDebug() << ">> Creating new file from extrapolated metadata";

//   //     } else {
//   //       qDebug() << ">> Deleting existing target file and using its metadata";
//   //       LIBMTP_Delete_Object(device()->device, file->item_id);
//   //       file->filesize = localFileInfo().size();
//   //     }

//   //     auto r = LIBMTP_Send_File_From_File(device()->device,
//   //                                         source.toStdString().c_str(),
//   //                                         file, NULL, NULL);
//   //     if (r != 0) {
//   //       _log->setText("Error sending file");
//   //       LIBMTP_Dump_Errorstack(device()->device);
//   //       LIBMTP_Clear_Errorstack(device()->device);

//   //     } else {
//   //       _log->setText("File successfully sent");
//   //       storage()->target =
//   //         LIBMTP_Get_Filemetadata(device()->device, file->item_id);
//   //       auto newSize = target()->filesize;
//   //       decltype(newSize) expectedSize = localFileInfo().size();
//   //       Q_ASSERT(newSize == expectedSize);
//   //       updateState(nullptr);
//   //     }

//   // //    if (file != target()) LIBMTP_destroy_file_t(file);
//   //     return true;
//   //   }

//   void updateState (QWidget *source) {
//     // qDebug() << "Updating state";
//     // if (source == _deviceCBox) {
//     //   _storageCBox->clear();
//     //   int i = _deviceCBox->currentIndex();
//     //   if (i >= 0)
//     //     for (const Storage &s: _devices.at(i).storages)
//     //       _storageCBox->addItem(s.name);
//     // }

//     // QString error;
//     // if (_devices.empty())
//     //   error = "No devices detected. Is the phone plugged in?";
//     // else if (device() == nullptr)
//     //   error = "No devices selected";
//     // else if (device()->storages.empty())
//     //   error = "No storage detected. Is file sharing activated on the phone?";
//     // else if (storage() == nullptr)
//     //   error = "No storage selected";
//     // else {
//     //   auto file = target();
//     //   if (!file)
//     //     _rlabel->setText("No database found");
//     //   else {
//     //     QString filedata;
//     //     QTextStream qts (&filedata);
//     //     qts << file->filename << " ("
//     //         << remoteModification().toString()
//     //         << ")";
//     //     _rlabel->setText(filedata);
//     //     qDebug() << "Remote: " << filedata;

//     //     auto lm = localModification(), rm = remoteModification();
//     //     if (rm < lm)
//     //       _log->setText("Phone version is outdated");
//     //     else if (rm == lm)
//     //       error = "No update necessary";
//     //     else
//     //       error = "Phone version is more recent!";
//     //   }
//     // }

//     // _log->setText(error);

//     // _send->setEnabled(error.isEmpty());
//     // qDebug() << "Send enabled?" << _send->isEnabled();
//   }

//   QFileInfo localFileInfo (void) const {
//     return QFileInfo(db::Book::monitoredPath());
//   }

//   QDateTime localModification (void) const {
//     return localFileInfo().lastModified();
//   }

//   // QDateTime remoteModification (void) {
//   //   if (!target())  return QDateTime();
//   //   return QDateTime::fromSecsSinceEpoch(target()->modificationdate);
//   // }

//   // Device* device (void) {
//   //   int i = _deviceCBox->currentIndex();
//   //   if (i < 0)  return nullptr;
//   //   return &_devices[i];
//   // }

//   // Storage* storage (void) {
//   //   Device *d = device();
//   //   if (d == nullptr) return nullptr;
//   //   int i = _storageCBox->currentIndex();
//   //   if (i < 0)  return nullptr;
//   //   return &d->storages[i];
//   // }

//   // LIBMTP_file_t* target (void) {
//   //   Storage *s = storage();
//   //   if (s == nullptr) return nullptr;
//   //   return s->target;
//   // }
// };
