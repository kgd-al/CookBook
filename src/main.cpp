#include <iostream>

#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QWindow>

#include <QJsonDocument>
#include <QDebug>
#include <QTimer>
#include <QTimeLine>

#include <QDir>

#include "gui/gui_book.h"
#include "gui/common.h"
#include "gui/settings.h"

#ifdef Q_OS_ANDROID
#include <android/log.h>
#endif

QFile logfile;
void logger (QtMsgType type, const QMessageLogContext &context,
             const QString &message) {

  static const QMap<QtMsgType, QString> types {
    {    QtDebugMsg, "## Debug" },
    {     QtInfoMsg, "### Info" },
    {  QtWarningMsg, "# Warning" },
    { QtCriticalMsg, " Critical" },
    {    QtFatalMsg, "### Fatal" },
    {   QtSystemMsg, "## System" }
  };

  QString typeStr = types.value(type);

  QString formattedMessage;
  QTextStream qts (&formattedMessage);
  qts << "##" << typeStr << " (" << context.file << ":"
      << context.line << ", " << context.function << "):\n"
      << message << "\n";

  if (logfile.isOpen()) {
    QTextStream qts (&logfile);
    qts << formattedMessage;
  }

#ifndef Q_OS_ANDROID
  std::cerr << formattedMessage.toStdString();
#else
  static const QMap<QtMsgType, android_LogPriority> qt2androidMsgType {
    {    QtDebugMsg, ANDROID_LOG_DEBUG  },
    {     QtInfoMsg, ANDROID_LOG_INFO   },
    {  QtWarningMsg, ANDROID_LOG_WARN   },
    { QtCriticalMsg, ANDROID_LOG_ERROR  },
    {    QtFatalMsg, ANDROID_LOG_FATAL  },
  };
  __android_log_write(qt2androidMsgType.value(type),
                      QCoreApplication::applicationName().toStdString().c_str(),
                      formattedMessage.toStdString().c_str());
#endif
}

int main(int argc, char *argv[]) {
  QApplication app (argc, argv);

  QApplication::setOrganizationName("almann");
  QApplication::setApplicationName("cookbook");
  QApplication::setApplicationDisplayName("CookBook");
  QApplication::setApplicationVersion("1.0.0");
  QApplication::setDesktopFileName(
    QApplication::organizationName() + "-" + QApplication::applicationName()
    + ".desktop");

  QString logFilePath;
  QTextStream qss (&logFilePath);
  qss << QDir::tempPath() << "/" << QApplication::organizationName()
      << "/" << QApplication::applicationName();
  QDir logFileDir (logFilePath);
  if (!logFileDir.exists()) {
    qDebug() << "Creating tmp directory " << logFileDir.path();
    if (!logFileDir.mkpath(logFileDir.path()))
      qWarning("Failed to create temporary folder %s",
               logFileDir.path().toStdString().c_str());
  }
  qss << "/"
      << QDateTime::currentDateTime().toString("yyyy-MM-dd.hh-mm-ss")
      << ".log";
  qDebug() << "logfilepath: " << logFilePath;
  logfile.setFileName(logFilePath);
  if (!logfile.open(QIODevice::WriteOnly))
    qWarning("Failed to open log file");
  qInstallMessageHandler(logger);

  QSettings settings;
  {
    auto q = qDebug();
    q << "Current settings (@" << settings.fileName() << "):\n";
    for (QString k: settings.allKeys())
      q << "\t" << k << ": " << settings.value(k) << "\n";
    q << "************************\n";
  }

#ifndef Q_OS_ANDROID
  QApplication::setFont(gui::Settings::value<QFont>(gui::Settings::FONT));
#endif
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

  QLocale fr_locale (QLocale::French);
//  QLocale fr_locale = QLocale::system();

  /// FIXME Does not work (on some buttons)
  QTranslator qtTranslator;
  qDebug() << "qtTranslator.load(" << fr_locale
           << QLibraryInfo::location(QLibraryInfo::TranslationsPath) << ")";
  if (qtTranslator.load(fr_locale,
              "qt", "_",
              QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
    qDebug() << ">> ok";
    app .installTranslator(&qtTranslator);
  }

  QTranslator qtBaseTranslator;
  qDebug() << "qtBaseTranslator (" << "qtbase_" << fr_locale.name()
           << ")";
  if (qtBaseTranslator.load("qtbase_" + fr_locale.name(),
              QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
    qDebug() << ">> ok";
    qDebug() << "Installed qtbase translator:"
             << app .installTranslator(&qtBaseTranslator);
  }
  qDebug() << "************************\n";

  // For decimal point (and lots of other changes I suppose)
  QLocale::setDefault(QLocale::c());

#ifdef Q_OS_ANDROID
  // Force the style
  app.setStyle("Fusion");

  // Now use a palette to switch to dark colors:
  auto palette = QPalette();
  palette.setColor(QPalette::Window, QColor(53, 53, 53));
  palette.setColor(QPalette::WindowText, Qt::white);
  palette.setColor(QPalette::Base, QColor(25, 25, 25));
  palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
  palette.setColor(QPalette::ToolTipBase, Qt::black);
  palette.setColor(QPalette::ToolTipText, Qt::white);
  palette.setColor(QPalette::Text, Qt::white);
  palette.setColor(QPalette::Button, QColor(53, 53, 53));
  palette.setColor(QPalette::ButtonText, Qt::white);
  palette.setColor(QPalette::BrightText, Qt::red);
  palette.setColor(QPalette::Link, QColor(42, 130, 218));
  palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
  palette.setColor(QPalette::HighlightedText, Qt::black);
  app.setPalette(palette);

  app.setStyleSheet(
    "QCheckBox::indicator { width: 64px; height: 64px; } "
    "QRadioButton::indicator { width: 48px; height: 48px; } "
  );
#endif

  gui::Book w;
  w.setWindowIcon(QIcon(":/icons/book.png"));
  w.show();

  return app.exec();
}
