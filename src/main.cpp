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

QFile logfile ("log");
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

  if (logfile.isOpen()) {
    QTextStream qts (&logfile);
    qts << "##" << typeStr << " (" << context.file << ":"
        << context.line << ", " << context.function << "):\n"
        << message << "\n";
  }
  std::cerr << "##" << typeStr.toStdString() << " (" << context.file << ":"
            << context.line << ", " << context.function << "):\n"
            << message.toStdString() << "\n";
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
  QApplication a(argc, argv);

  if (!logfile.open(QIODevice::WriteOnly))
    qWarning("Failed to open log file");
  qInstallMessageHandler(logger);

  QCoreApplication::setOrganizationName("almann");
  QCoreApplication::setApplicationName("cookbook");

  // Register icons
  db::AlimentaryGroupData::loadDatabase();
  db::RegimenData::loadDatabase();
  db::StatusData::loadDatabase();
  db::DishTypeData::loadDatabase();
  db::DurationData::loadDatabase();

  QSettings settings;
  {
    auto q = qDebug();
    q << "Current settings:\n";
    for (QString k: settings.allKeys())
      q << "\t" << k << ": " << settings.value(k) << "\n";
    q << "************************\n";
  }

  QApplication::setFont(settings.value("font").value<QFont>());

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
    a.installTranslator(&qtTranslator);
  }

  QTranslator qtBaseTranslator;
  qDebug() << "qtBaseTranslator (" << "qtbase_" << fr_locale.name()
           << ")";
  if (qtBaseTranslator.load("qtbase_" + fr_locale.name(),
              QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
    qDebug() << ">> ok";
    qDebug() << "Installed qtbase translator:"
             << a.installTranslator(&qtBaseTranslator);
  }
  qDebug() << "************************\n";

  //  QLocale loc = QLocale::system(); // current locale
  ////  loc.set(QLocale::c().decimalPoint()); // borrow decimal char from the "C" locale
  //  loc.setNumberOptions(QLocale::c().numberOptions()); // borrow number options from the "C" locale
  //  QLocale::setDefault(loc); // set as default
    QLocale::setDefault(QLocale::c());

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

  return a.exec();
}
