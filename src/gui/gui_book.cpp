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
#include "planningview.h"
#include "autofiltercombobox.hpp"
#include "ingredientsmanager.h"
#include "updatemanager.h"
#include "repairsmanager.h"
#include "gui_settings.h"
#include "about.h"

#include "../db/settings.h"
#include "../db/recipesmodel.h"


namespace gui {

Book::Book(QWidget *parent) : QMainWindow(parent) {
  _filter = new FilterView (this);
  auto *proxy = _filter->proxyModel();

  _recipes = new QTableView;
  _recipes->setEditTriggers(QAbstractItemView::NoEditTriggers);
  _recipes->setModel(proxy);
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
  _recipes->setSortingEnabled(true);
  _recipes->setDragDropMode(QAbstractItemView::DragOnly);
  proxy->setSortRole(db::RecipesModel::SortRole);
  proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
  proxy->setDynamicSortFilter(true);
  _recipes->sortByColumn(db::RecipesModel::titleColumn(),
                           Qt::AscendingOrder);

  connect(_recipes, &QTableView::activated, this, &Book::showRecipe);

#ifndef Q_OS_ANDROID
  _planning = new PlanningView;
#endif

  buildLayout();

  connect(&db::Book::current(), &db::Book::modified,
          this, &Book::setWindowModified);
  connect(_filter, &FilterView::filterChanged, this, &Book::setAutoTitle);

  QMenuBar *bar = menuBar();
    QMenu *m_book = bar->addMenu("Book");
//      m_book->addAction(QIcon::fromTheme(""), "Load",
//                        this, QOverload<>::of(&Book::loadRecipes),
//                        QKeySequence("Ctrl+O"));

#ifndef Q_OS_ANDROID
      m_book->addAction(QIcon::fromTheme(""), "&Save",
                        [this] { overwriteRecipes(false); },
                        QKeySequence("Ctrl+S"));

//      m_book->addAction(QIcon::fromTheme(""), "Save As",
//                        [this] { saveRecipes(); },
//                        QKeySequence("Ctrl+Shift+S"));
#endif

      m_book->addAction(QIcon::fromTheme(""), "&Filtrer",
                        this, &Book::toggleFilterArea,
                        QKeySequence("Ctrl+F"));

      m_book->addAction(QIcon::fromTheme(""), "P&lanning",
                        this, &Book::togglePlanningArea,
                        QKeySequence("Ctrl+L"));

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
  gui::restore(settings, "vsplitter", _vsplitter);
  gui::restore(settings, "hsplitter", _hsplitter);
  _hsplitter->setVisible(true);
  _recipes->setVisible(true);
#else
  _splitter->setSizes({M, M});
  _filter->hide();
#endif

  gui::restoreGeometry(this, settings);
}

void Book::buildLayout(void) {
#ifndef Q_OS_ANDROID
  _vsplitter = new QSplitter (Qt::Vertical);
  _vsplitter->setChildrenCollapsible(false);
    _hsplitter = new QSplitter (Qt::Horizontal);
    _hsplitter->setChildrenCollapsible(false);
      _hsplitter->addWidget(_recipes);
      _hsplitter->addWidget(_filter);
    _vsplitter->addWidget(_hsplitter);
    _vsplitter->addWidget(_planning);
  setCentralWidget(_vsplitter);
#else
  _splitter = new QSplitter (Qt::Vertical);
    _splitter->addWidget(_recipes);
    _splitter->addWidget(_filter);
  setCentralWidget(_splitter);
#endif
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
//    setModified(true);
  });
  drecipe.show(&recipe, false);

  if (validated)  overwriteRecipes();
}
#endif

void Book::showRecipe(const QModelIndex &index) {
  Recipe recipe (this);
//  connect(&recipe, &Recipe::validated, [this] { setModified(true); });
  recipe.show(&db::Book::current()
              .recipes.at(db::ID(index.data(db::IDRole).toInt())),
              true, index);
#ifndef Q_OS_ANDROID
  overwriteRecipes();
#endif
}

#ifndef Q_OS_ANDROID
bool Book::overwriteRecipes(bool spontaneous) {
  return db::Book::current().autosave(spontaneous);
}
#endif

bool Book::loadDefaultBook(void) {
  auto &book = db::Book::current();
  auto ret = book.load();
  if (ret) {
    _filter->proxyModel()->setSourceModel(&book.recipes);
    setAutoTitle();
  }
  return ret;
}

#ifndef Q_OS_ANDROID
template <typename T>
void maybeShowModal (Book *b, db::Settings::Type t) {
  if (db::Settings::value<bool>(t)) {
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
  maybeShowModal<IngredientsManager>(this, db::Settings::MODAL_IMANAGER);
}

void Book::showUpdateManager(void) {
  UpdateManager (this).exec();
}

void Book::showRepairUtility(void) {
  maybeShowModal<RepairsManager>(this, db::Settings::MODAL_REPAIRS);
}

void Book::showSettings(void) {
  maybeShowModal<SettingsView>(this, db::Settings::MODAL_SETTINGS);
}

#endif

void Book::showAbout(void) {
  About (this).exec();
}

void Book::setAutoTitle(void) {
  QString name;
  QTextStream qts (&name);
  auto *proxy = _filter->proxyModel();
  qts << proxy->rowCount() << " recettes";
  if (proxy->rowCount() != proxy->sourceModel()->rowCount())
    qts << " sur " << proxy->sourceModel()->rowCount();
  qts << " [*]";

  setWindowTitle(name);
}

//void Book::setModified(bool m) {
//  db::Book::current().modified = m;
//  setAutoTitle();
//}

void Book::closeEvent(QCloseEvent *e) {
  auto &book = db::Book::current();

#ifndef Q_OS_ANDROID
  if (!book.close(this)) {
    e->ignore();
    return;
  } else
    e->accept();
#else
  const bool confirm = true;
  const QString msg = "Quitter?";
  auto b2 = QMessageBox::NoButton;
  auto ret = QMessageBox::warning(this, "Confirmez", "Quitter?",
                                  QMessageBox::Yes, QMessageBox::No);
  switch (ret) {
  case QMessageBox::Yes:
    e->accept();
    break;
  case QMessageBox::No:
  default:
    e->ignore();
    return;
  }
#endif

  auto &settings = localSettings(this);
#ifndef Q_OS_ANDROID
  gui::save(settings, "vsplitter", _vsplitter);
  gui::save(settings, "hsplitter", _hsplitter);
#endif
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
  _filter->setVisible(!_filter->isVisible());
  q << " >> " << _filter->geometry().isValid();
}

void Book::togglePlanningArea(void) {
  auto q = qDebug().nospace();
  q << "planning visible? " << _planning->geometry().isValid();
  _planning->setVisible(!_planning->isVisible());
  q << " >> " << _planning->geometry().isValid();
}

} // end of namespace gui
