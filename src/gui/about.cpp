#include <QApplication>
#include <QTextStream>
#include <QGridLayout>
#include <QLabel>
#include <QIcon>
#include <QDialogButtonBox>

#include "about.h"
#include "../db/recipedata.h"
#include "about_metadata.h"

namespace gui {

About::About(QWidget *parent) : QDialog(parent) {
  auto *layout = new QGridLayout;

#ifdef Q_OS_ANDROID
  setWindowFlag(Qt::Popup, true);
#endif

  auto *label = new QLabel;
  label->setPixmap(QIcon(":/icons/book.png").pixmap(10*db::iconQSize()));
  layout->addWidget(label, 0, 0);

  auto *rdisplay = new QLabel;
  layout->addWidget(rdisplay, 0, 1);

  auto *bdisplay = new QLabel;
  layout->addWidget(bdisplay, 1, 0, 1, 2);

  setLayout(layout);

  QString text;
  QTextStream qts(&text);
  qts << QApplication::applicationDisplayName() << "\n"
      << "Version " << QApplication::applicationVersion() << "\n"
      << "Fait avec amour!\n(pour mon amour)";
  rdisplay->setText(text);

  text = "";
  qts << "  git: " << GIT_HASH << "\n";
  if (QString(GIT_MSG) != ".")
    qts << "       " << GIT_MSG << "\n";
  qts << "       " << GIT_DATE << "\n"
      << "build: " << BUILD_DATE;
  bdisplay->setText(text);

  auto *buttons = new QDialogButtonBox (QDialogButtonBox::Close);
  layout->addWidget(buttons, 2, 0, 1, 2);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

} // end of namespace gui
