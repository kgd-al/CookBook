#include <QApplication>
#include <QSettings>
#include <QWindow>

#include <QJsonDocument>
#include <QDebug>
#include <QTimer>
#include <QTimeLine>

#include "gui/gui_book.h"

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
  QCoreApplication::postEvent(w, new QKeyEvent(QEvent::KeyPress, key,
                                               modifiers, kstr));
//    qDebug() << "Sent" << kstr << " RELEASE";
  QCoreApplication::postEvent(w, new QKeyEvent(QEvent::KeyRelease,
                                               key, modifiers));
//    qDebug() << "Done " << kstr;
  QDebug q = qDebug().nospace();
  q << kstr;
//  q << " (" << hex << key << " | " << modifiers << ")";
}

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

//  QLocale loc = QLocale::system(); // current locale
////  loc.set(QLocale::c().decimalPoint()); // borrow decimal char from the "C" locale
//  loc.setNumberOptions(QLocale::c().numberOptions()); // borrow number options from the "C" locale
//  QLocale::setDefault(loc); // set as default
  QLocale::setDefault(QLocale::c());

  QCoreApplication::setOrganizationName("almann");
  QCoreApplication::setApplicationName("cookbook");

  QSettings settings;
  QString lastBook = settings.value("lastBook").toString();
  QRect lastGeometry = settings.value("lastGeometry").toRect();

  QApplication::setFont(settings.value("font").value<QFont>());

  gui::Book w;
  if (!lastBook.isEmpty())  w.loadRecipes(lastBook);
  if (lastGeometry.isValid()) w.setGeometry(lastGeometry);
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

  QTimer::singleShot(100, [&timeline] {
    qDebug() << "Start";
    timeline.start();
  });

  return a.exec();
}
