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
  auto *layout = new QVBoxLayout;
  layout->setSpacing(20);

#ifdef Q_OS_ANDROID
  setWindowFlag(Qt::Popup, true);
#endif

    auto *tlayout = new QHBoxLayout;
      auto *icon = new QLabel;
      icon->setPixmap(QIcon(":/icons/book.png").pixmap(10*db::iconQSize()));
      tlayout->addWidget(icon);

      auto *rdisplay = new QLabel;
      tlayout->addWidget(rdisplay);

    layout->addLayout(tlayout);

    auto *blayout = new QGridLayout;
      auto git_label = new QLabel("git");
      git_label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
      blayout->addWidget(git_label, 0, 0, Qt::AlignRight);

      auto *gdisplay = new QLabel;
      gdisplay->setWordWrap(true);
      gdisplay->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum);
      blayout->addWidget(gdisplay, 0, 1, Qt::AlignLeft | Qt::AlignTop);

      auto build_label = new QLabel("build");
      blayout->addWidget(build_label, 1, 0);

      auto *bdisplay = new QLabel;
      bdisplay->setWordWrap(true);
      bdisplay->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Maximum);
      blayout->addWidget(bdisplay, 1, 1);

      blayout->setColumnStretch(0, 0);
      blayout->setColumnStretch(1, 1);

    layout->addLayout(blayout);
  setLayout(layout);

  QString text;
  QTextStream qts(&text);
  qts << QApplication::applicationDisplayName() << "\n"
      << "Version " << QApplication::applicationVersion() << "\n"
      << "Fait avec amour!\n(pour mon amour)";
  rdisplay->setText(text);

  text = "";
  qts << GIT_HASH << "\n";
  if (QString(GIT_MSG) != ".")
    qts << GIT_MSG << "\n";
  qts << GIT_DATE;
  gdisplay->setText(text);

  text.clear();
  qts << BUILD_DATE << " with Qt " << QT_VERSION_STR;
  bdisplay->setText(text);

  auto *buttons = new QDialogButtonBox (QDialogButtonBox::Close);
  layout->addWidget(buttons);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

} // end of namespace gui
