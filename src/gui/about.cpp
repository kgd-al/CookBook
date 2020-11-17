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
  auto *tdisplay = new QLabel;
  layout->addWidget(tdisplay, 0, 1);
  setLayout(layout);

  auto *label = new QLabel;
  label->setPixmap(QIcon(":/icons/book.png").pixmap(10*db::iconQSize()));
  layout->addWidget(label, 0, 0);

  QString text;
  QTextStream qts(&text);
  qts << QApplication::applicationDisplayName() << "\n"
      << "Version " << QApplication::applicationVersion() << "\n"
      << "Fait avec amour!\n\n"
      << "  git: " << GIT_HASH << "\n";
  if (QString(GIT_MSG) != ".")
    qts << "       " << GIT_MSG << "\n";
  qts << "       " << GIT_DATE << "\n"
      << "build: " << BUILD_DATE;
  tdisplay->setText(text);

  auto *buttons = new QDialogButtonBox (QDialogButtonBox::Close);
  layout->addWidget(buttons, 1, 0, 1, 2);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

} // end of namespace gui
