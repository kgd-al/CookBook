#include <QApplication>
#include <QFileDialog>
#include <QFontDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QShortcut>
#include <QScreen>

#include <QStyledItemDelegate>
#include <QPainter>

#include <QMenuBar>
#include <QMenu>
#include <QStatusBar>

#include <QGridLayout>
#include <QTableView>
#include <QHeaderView>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QDialogButtonBox>

#ifdef Q_OS_ANDROID
#include <QScroller>
#include "androidspecifics.hpp"
#endif

#include "common.h"
#include "gui_book.h"
#include "filterview.h"
#include "autofiltercombobox.hpp"
#include "ingredientsmanager.h"
#include "updatemanager.h"
#include "repairsmanager.h"
#include "settings.h"

#include "../db/recipesmodel.h"


namespace gui {

static const QString saveFormat = ".rbk";
static const QString fileFilter = "Recipe Book (*" + saveFormat + ")";

Book::Book(QWidget *parent) : QMainWindow(parent) {
#ifndef Q_OS_ANDROID
  auto sorientation = Qt::Horizontal;
#else
  auto sorientation = Qt::Vertical;
#endif
    _splitter = new QSplitter (sorientation);
    _filter = new FilterView (this);
    auto *proxy = _filter->proxyModel();

      _recipes = new QTableView;
      _recipes->setEditTriggers(QAbstractItemView::NoEditTriggers);
      _recipes->setModel(proxy);
      _splitter->addWidget(_recipes);
      auto rheader = _recipes->horizontalHeader();
      Q_ASSERT(rheader);
#ifdef Q_OS_ANDROID
      rheader->setMinimumSectionSize(0);//.5*db::iconSize());
#else
      rheader->setMinimumSectionSize(1.5*db::iconSize());
#endif
      rheader->setSectionResizeMode(QHeaderView::ResizeToContents);
      rheader->setStretchLastSection(true);
      _recipes->verticalHeader()->hide();
      _recipes->setSelectionBehavior(QTableView::SelectRows);
      _recipes->setSelectionMode(QTableView::SingleSelection);
      _recipes->setShowGrid(false);
//      rheader->setSectionResizeMode(_recipes->model()->columnCount()-1,
//                                    QHeaderView::Stretch);
//      _recipes->setItemDelegate(new RecipeListDelegate (this));
      _recipes->setSortingEnabled(true);
      proxy->setSortRole(db::RecipesModel::SortRole);
      proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
      proxy->setDynamicSortFilter(true);
      _recipes->sortByColumn(db::RecipesModel::titleColumn(),
                             Qt::AscendingOrder);

      _splitter->addWidget(_filter);

  setCentralWidget(_splitter);

  connect(_recipes, &QTableView::activated, this, &Book::showRecipe);

  QMenuBar *bar = menuBar();
    QMenu *m_book = bar->addMenu("Book");
//      m_book->addAction(QIcon::fromTheme(""), "Load",
//                        this, QOverload<>::of(&Book::loadRecipes),
//                        QKeySequence("Ctrl+O"));

#ifndef Q_OS_ANDROID
      m_book->addAction(QIcon::fromTheme(""), "Save",
                        [this] { overwriteRecipes(false); },
                        QKeySequence("Ctrl+S"));

//      m_book->addAction(QIcon::fromTheme(""), "Save As",
//                        [this] { saveRecipes(); },
//                        QKeySequence("Ctrl+Shift+S"));
#endif

      m_book->addAction(QIcon::fromTheme(""), "Filtrer",
                        this, &Book::toggleFilterArea,
                        QKeySequence("Ctrl+F"));

#ifndef Q_OS_ANDROID
    QMenu *m_recipes = bar->addMenu("Recipes");
      m_recipes->addAction(QIcon::fromTheme(""), "Add",
                           this, &Book::addRecipe,
                           QKeySequence("Ctrl+N"));
    QMenu *m_ingredients = bar->addMenu("Ingredients");
    m_ingredients->addAction(QIcon::fromTheme(""), "Manage",
                             this, &Book::showIngredientsManager,
                             QKeySequence("Ctrl+I"));

    QMenu *m_other = bar->addMenu("Misc");
      m_other->addAction("Mise à jour", this, &Book::showUpdateManager,
                         QKeySequence("Ctrl+U"));
      m_other->addAction("Réparer", this, &Book::showRepairUtility,
                         QKeySequence("Ctrl+R"));
      m_other->addAction("Configuration", this, &Book::showSettings,
                         QKeySequence("Ctrl+C"));
      m_other->addAction("Bug tracker", [] {
        QDesktopServices::openUrl(QUrl("https://github.com/kgd-al/CookBook/issues"));
      });
      m_other->addAction("About", this, &Book::showAbout);

#else
//  grabGesture(android::SingleFingerSwipeRecognizer::type());
  QScroller::grabGesture(_recipes, QScroller::LeftMouseButtonGesture);
#endif

  statusBar();  // Invoke now to make space

  auto &settings = localSettings(this);

//  QString lastBook = settings.value("lastBook").toString();
//  if (!lastBook.isEmpty())  loadRecipes(lastBook);
  if (!loadDefaultBook())
    QMessageBox::warning(this, "Erreur",
                         "Impossible de charger le livre de recette '"
                         + db::Book::monitoredPath() + "'");

#ifndef Q_OS_ANDROID
  QVariant defaultSizes = QVariant::fromValue(QList<int>{100,100});
  _splitter->setSizes(
    settings.value("splitter", defaultSizes).value<QList<int>>());
#else
  int maxHeight = QGuiApplication::primaryScreen()->size().height();
  _splitter->setSizes({maxHeight,maxHeight});
  _filter->hide();
#endif

  gui::restoreGeometry(this, settings);
}

Book::~Book(void) {}

#ifndef Q_OS_ANDROID
void Book::addRecipe(void) {
  Recipe drecipe (this);
  db::Recipe recipe;
  bool validated = false;
  connect(&drecipe, &Recipe::validated,
          [this,&validated,&drecipe,&recipe] {
    validated = true;
    qDebug() << "Inserting: " << db::Recipe::toJson(recipe);
    QModelIndex source_index = db::Book::current().addRecipe(std::move(recipe));
    drecipe.setIndex(_filter->proxyModel()->mapFromSource(source_index));
    setModified(true);
  });
  drecipe.show(&recipe, false);

  if (validated)  overwriteRecipes();
}
#endif

void Book::showRecipe(const QModelIndex &index) {
  Recipe recipe (this);
  connect(&recipe, &Recipe::validated, [this] { setModified(true); });
  recipe.show(&db::Book::current()
              .recipes.at(db::ID(index.data(db::IDRole).toInt())),
              true, index);
#ifndef Q_OS_ANDROID
  overwriteRecipes();
#endif
}

#ifndef Q_OS_ANDROID
bool Book::saveRecipes(void) {
  QString path = QFileDialog::getSaveFileName(
    this, "Define where to save the book", ".", fileFilter);
  if (!path.isEmpty())  return saveRecipes(path);
  return false;
}

bool Book::saveRecipes(const QString &path) {
  if (path.isEmpty()) return saveRecipes();
  auto ok = db::Book::current().save(path);
  if (ok) setModified(false);
  return ok;
}

bool Book::overwriteRecipes(bool spontaneous) {
  if (!db::Book::current().modified)  return false;
  if (spontaneous && !Settings::value<bool>(Settings::AUTOSAVE)) return false;
  return saveRecipes(db::Book::current().path);
}
#endif

bool Book::loadRecipes(void) {
  return loadRecipes(
          QFileDialog::getOpenFileName(
            this, "Select recipe book", ".", fileFilter));
}

bool Book::loadRecipes(const QString &path) {
  auto &book = db::Book::current();
  auto ret = book.load(path);
  if (ret) {
    _filter->proxyModel()->setSourceModel(&book.recipes);
    setModified(false);
  }
  return ret;
}

bool Book::loadDefaultBook(void) {
  auto &book = db::Book::current();
  auto ret = book.load();
  if (ret) {
    _filter->proxyModel()->setSourceModel(&book.recipes);
    setModified(false);
  }
  return ret;
}

#ifndef Q_OS_ANDROID
template <typename T>
void maybeShowModal (Book *b, Settings::Type t) {
  if (Settings::value<bool>(t)) {
    T manager (b);
    manager.exec();
    b->overwriteRecipes();

  } else {
    auto manager = new T (b);
    Book::connect(manager, &T::destroyed,
                  b, &Book::overwriteRecipes);
    manager->show();
  }
}

void Book::showIngredientsManager(void) {
  maybeShowModal<IngredientsManager>(this, Settings::MODAL_IMANAGER);
}

void Book::showUpdateManager(void) {
  UpdateManager (this).exec();
}

void Book::showRepairUtility(void) {
  maybeShowModal<RepairsManager>(this, Settings::MODAL_REPAIRS);
}

void Book::showSettings(void) {
  maybeShowModal<Settings>(this, Settings::MODAL_SETTINGS);
}

#endif

void Book::showAbout(void) {
  QDialog d (this);

  auto *layout = new QGridLayout;
  auto *tdisplay = new QLabel;
  layout->addWidget(tdisplay, 0, 1);
  d.setLayout(layout);

  auto *label = new QLabel;
  label->setPixmap(QIcon(":/icons/book.png").pixmap(10*db::iconQSize()));
  layout->addWidget(label, 0, 0);

  QString text;
  QTextStream qts(&text);
  qts << QApplication::applicationDisplayName() << "\n"
      << "Version " << QApplication::applicationVersion() << "\n";
  tdisplay->setText(text);

  auto *buttons = new QDialogButtonBox (QDialogButtonBox::Close);
  layout->addWidget(buttons, 1, 0, 1, 2);
  connect(buttons, &QDialogButtonBox::rejected, &d, &QDialog::reject);

  d.exec();
}

void Book::setAutoTitle(void) {
  db::Book &book = db::Book::current();

  QString name = book.path;
  if (name == db::Book::monitoredPath())
    name = "(monitored)";

  else {
    name = name.mid(name.lastIndexOf('/')+1);
    name = name.mid(0, name.lastIndexOf('.')) + "(unmonitored)";
  }

  if (book.modified)  name += " *";

  setWindowTitle(name);
}

void Book::setModified(bool m) {
  db::Book::current().modified = m;
  setAutoTitle();
}

void Book::closeEvent(QCloseEvent *e) {
  auto &book = db::Book::current();

#ifndef Q_OS_ANDROID
  const bool confirm = book.modified;
  const QString msg = "Sauvegarder les changements?";
  auto b2 = QMessageBox::No;
#else
  const bool confirm = true;
  const QString msg = "Quitter?";
  auto b2 = QMessageBox::NoButton;
#endif

  if (confirm) {
    auto ret = QMessageBox::warning(this, "Confirmez", msg,
                                    QMessageBox::Yes, b2, QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Yes:
#ifndef Q_OS_ANDROID
      overwriteRecipes(false);
#endif
      break;
    case QMessageBox::No:
      e->accept();
      break;
    case QMessageBox::Cancel:
    default:
      e->ignore();
    }
  }

  auto &settings = localSettings(this);
  settings.setValue("lastBook", book.path);
  settings.setValue("splitter", QVariant::fromValue(_splitter->sizes()));
  gui::saveGeometry(this, settings);
}

#ifdef Q_OS_ANDROID
bool Book::event(QEvent *event) {
  if (event->type() == QEvent::Gesture) {
    auto *ge = dynamic_cast<QGestureEvent*>(event);
    auto q = qDebug().nospace();
    q << ge << "\n";
    if (auto *g = ge->gesture(android::SingleFingerSwipeRecognizer::type())) {
      auto sg = static_cast<android::ExtendedSwipeGesture*>(g);
      q << "\tExtendedSwipeGesture: " << sg->dx() << ", " << sg->dy() << "\n";
      if (sg->state() == Qt::GestureFinished) {
        if (sg->dy() > 100)
          showFilterArea(false);
        else if (sg->dy() < -100)
          showFilterArea(true);
      }
    }
    return true;

  } else
    return QMainWindow::event(event);
}
#endif

void Book::toggleFilterArea(void) {
  auto q = qDebug().nospace();
  q << "filter visible? " << _filter->geometry().isValid();
  showFilterArea(!_filter->geometry().isValid());
  q << " >> " << _filter->geometry().isValid();
}

void Book::showFilterArea(bool show) {
//  int maxHeight = QGuiApplication::primaryScreen()->size().height();
//  _splitter->setSizes(QList<int>({maxHeight, maxHeight*show}));
  if (show)
    _filter->show();
  else
    _filter->hide();
  qDebug() << _filter << _filter->geometry() << _filter->isVisible();
}

} // end of namespace gui
