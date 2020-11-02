#include <iostream>

#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QWindow>

#include <QJsonDocument>
#include <QDebug>
#include <QTimer>
#include <QTimeLine>

#include "gui/gui_book.h"
#include "gui/common.h"

#ifdef Q_OS_ANDROID
#include <android/log.h>
#endif

static constexpr auto appName = "cookbook";

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
                      appName,
                      formattedMessage.toStdString().c_str());
#endif
}

void simulateKey (const QString &kstr) {
  const QKeySequence kseq = kstr;
//    qDebug() << "Key sequence: " << kseq;
  int key = kseq[0] & (~Qt::KeyboardModifierMask);
  auto modifiers = Qt::KeyboardModifiers(kseq[0] & Qt::KeyboardModifierMask);
//    qDebug().nospace() << hex
//      << "0x" << kseq[0] << " -> 0x" << key << " | 0x" << modifiers;

//    qDebug() << "Requesting " << kstr;
  auto *w = QGuiApplication::focusWindow();

//    qDebug() << "Focus is on " << FOCUSOBJ;
//    qDebug() << "Sent" << kstr << " PRESS";

  for (QEvent::Type t: {QEvent::KeyPress, QEvent::KeyRelease}) {
    auto e = new QKeyEvent(t, key, modifiers, kstr);
//    if(!handleShortcutEvent(e, w))
    QCoreApplication::postEvent(w, e);
  }
//    qDebug() << "Done " << kstr;
  QDebug q = qDebug().nospace();
  q << kstr;
//  q << " (" << hex << key << " | " << modifiers << ")";
}

int main(int argc, char *argv[]) {
  QApplication app (argc, argv);

  QCoreApplication::setOrganizationName("almann");
  QCoreApplication::setApplicationName(appName);
  QSettings settings;

  QString logFilePath;
  QTextStream qss (&logFilePath);
  qss << settings.fileName().left(settings.fileName().lastIndexOf('.')) << "."
      << QDateTime::currentDateTime().toString("yyyy-MM-dd.hh-mm-ss")
      << ".log";
  qDebug() << "logfilepath: " << logFilePath;
  logfile.setFileName(logFilePath);
  if (!logfile.open(QIODevice::WriteOnly))
    qWarning("Failed to open log file");
  qInstallMessageHandler(logger);

  {
    auto q = qDebug();
    q << "Current settings (@" << settings.fileName() << "):\n";
    for (QString k: settings.allKeys())
      q << "\t" << k << ": " << settings.value(k) << "\n";
    q << "************************\n";
  }

  QApplication::setFont(settings.value("font").value<QFont>());
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

  //  QLocale loc = QLocale::system(); // current locale
  ////  loc.set(QLocale::c().decimalPoint()); // borrow decimal char from the "C" locale
  //  loc.setNumberOptions(QLocale::c().numberOptions()); // borrow number options from the "C" locale
  //  QLocale::setDefault(loc); // set as default
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

  app.setStyleSheet("QCheckBox::indicator {width: 91px; height: 91px; } ");
#endif

  gui::Book w;
  w.show();

  QList<QString> data {

// Test that invalid ingredients show appropriate errors
//    "Down", "Enter",
//    "Tab", "Tab", "Tab",
//    "Enter",
//    "Tab", "Tab", "Tab", "Tab", "Tab", "Tab",
//    "Space",
//    "Enter"

// Test add ingredient (one new, one existing and edit existing)
//    "Down", "Enter",
//    "Tab", "Tab", "Tab",
//    "Enter",
//    "Tab", "Tab", "Tab", "Tab", "Tab", "Tab",
//    "Space",
//    "Tab",

//    "Rhum", "Enter", "Tab",
//    "120", "Enter", "Tab",
//    "L", "Enter", "Tab",
//    "Liq", "Down", "Enter", "Tab",
//    "Space",

//    "Space", "Tab",
//    "Eau", "Down", "Enter", "Tab",
//    "1", "Tab",
//    "cL", "Enter", "Tab", "Space",

//    "Shift+Tab", "Down", "Tab", "Tab", "Space"

// Menu shortcuts don't work...
//    "Ctrl+I",
//    "Ctrl+N",
//    "Ctrl+U"
  };

  int duration = 5;
  QTimeLine timeline (duration*1000);
  timeline.setEasingCurve(QEasingCurve::Linear);
  timeline.setFrameRange(0, data.size());

  QTimeLine::connect(&timeline, &QTimeLine::frameChanged,
                     [&data,&timeline] {
    if (data.isEmpty()) {
      timeline.stop();
      return;
    }

    QString k = data.takeFirst();
    if (k != "-") simulateKey(k);
  });

  if (!data.empty())
    QTimer::singleShot(100, [&timeline] {
      qDebug() << "Start";
      timeline.start();
    });

  return app .exec();
}
