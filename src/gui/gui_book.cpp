#include <QApplication>
#include <QFileDialog>
#include <QFontDialog>
#include <QMessageBox>
#include <QDesktopServices>

#include <QMenuBar>
#include <QMenu>

#include "gui_book.h"
#include "ingredientsmanager.h"
#include "updatemanager.h"
#include "common.h"
#include "../db/recipesmodel.h"


namespace gui {

static const QString saveFormat = ".rbk";
static const QString fileFilter = "Recipe Book (*" + saveFormat + ")";

struct RecipeListItem : public QListWidgetItem {
  db::Recipe &recipe;
  RecipeListItem (db::Recipe &r) : recipe(r) {}

  QVariant data (int role) const override {
    if (role == Qt::DisplayRole)
      return recipe.title;
    else
      return QListWidgetItem::data(role);
  }
};

Book::Book(QWidget *parent) : QMainWindow(parent) {
  _recipes = new QListView;
  _recipes->setEditTriggers(QAbstractItemView::NoEditTriggers);
//  _recipes->setEditTriggers(QAbstractItemView::DoubleClicked
//                          | QAbstractItemView::SelectedClicked
//                          | QAbstractItemView::EditKeyPressed);
//  _recipes->set
  setCentralWidget(_recipes);

  connect(_recipes, &QListView::activated,
          this, &Book::showRecipe);

  _recipes->setContextMenuPolicy(Qt::ActionsContextMenu);

//  QAction *a_del = new QAction("delete");
//  connect(a_del, &QAction::triggered, [this] {
//    auto index = _recipes->currentIndex();
//    if (!index.isValid()) return;
//    _recipes->re
//  });
//  _recipes->addAction(a_del);

  QMenuBar *bar = menuBar();
    QMenu *m_book = bar->addMenu("Book");
      m_book->addAction(QIcon::fromTheme(""), "New",
                        [this] {
                          db::Book::current().clear();
                          setModified(false);
                        },
                        QKeySequence("Ctrl+Shift+N"));

      m_book->addAction(QIcon::fromTheme(""), "Load",
                        this, qOverload<>(&Book::loadRecipes),
                        QKeySequence("Ctrl+O"));
      m_book->addAction(QIcon::fromTheme(""), "Save",
                        [this] { overwriteRecipes(); },
                        QKeySequence("Ctrl+S"));
      m_book->addAction(QIcon::fromTheme(""), "Save As",
                        [this] { saveRecipes(); },
                        QKeySequence("Ctrl+Shift+S"));
    QMenu *m_recipes = bar->addMenu("Recipes");
      m_recipes->addAction(QIcon::fromTheme(""), "Add",
                           this, &Book::addRecipe,
                           QKeySequence("Ctrl+N"));
    QMenu *m_ingredients = bar->addMenu("Ingredients");
    m_ingredients->addAction(QIcon::fromTheme(""), "Manage",
                             this, &Book::showIngredientsManager,
                             QKeySequence("Ctrl+I"));

    QMenu *m_other = bar->addMenu("Misc");
      m_other->addAction("Font", [this] {
        bool ok;
        QFont font = QApplication::font();
        font = QFontDialog::getFont(&ok, font);
        if (ok) {
          QApplication::setFont(font);
          QSettings settings;
          settings.setValue("font", font);
        }

      }, QKeySequence("Ctrl+Shift+F"));
      m_other->addAction("Update", this, &Book::showUpdateManager,
                         QKeySequence("Ctrl+U"));
      m_other->addAction("Clear settings", [] { QSettings().clear(); });
      m_other->addAction("Bug tracker", [] {
        QDesktopServices::openUrl(QUrl("https://github.com/kgd-al/CookBook/issues"));
      });

  auto &settings = localSettings(this);
  QString lastBook = settings.value("lastBook").toString();
  if (!lastBook.isEmpty())  loadRecipes(lastBook);

  gui::restoreGeometry(this, settings);

  /// TODO remove
  showIngredientsManager();
  setFocus();
}

Book::~Book(void) {}

void Book::addRecipe(void) {
  Recipe drecipe (this);
  db::Recipe recipe = db::Recipe::defaultRecipe();
  bool validated = false;
  connect(&drecipe, &Recipe::validated, [&validated] { validated = true; });
  drecipe.show(&recipe, false);

  if (validated) {
    qDebug() << "Inserting: " << db::Recipe::toJson(recipe);
    db::Book::current().addRecipe(std::move(recipe));
    setModified(true);
  }
}

void Book::showRecipe(const QModelIndex &index) {
  Recipe recipe (this);
  connect(&recipe, &Recipe::validated, [this] { setModified(true); });
  recipe.show(&db::Book::current().recipes.fromIndex(index), true);
}

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

bool Book::overwriteRecipes(void) {
  return saveRecipes(db::Book::current().path);
}

bool Book::loadRecipes(void) {
  return loadRecipes(
          QFileDialog::getOpenFileName(
            this, "Select recipe book", ".", fileFilter));
}

bool Book::loadRecipes(const QString &path) {
  auto &book = db::Book::current();
  auto ret = book.load(path);
  if (ret) {
    _recipes->setModel(&book.recipes);
    setModified(false);
  }
  return ret;
}

void Book::showIngredientsManager(void) {
  auto &settings = localSettings(this);
  if (settings.value("modal").toBool()) {
    IngredientsManager manager (this);
    manager.exec();

  } else
    (new IngredientsManager (this))->show();
}

void Book::showUpdateManager(void) {
  UpdateManager manager (this);
  manager.exec();
}

void Book::setAutoTitle(void) {
  db::Book &book = db::Book::current();

  QString name = book.path;
  if (name.isEmpty())
    name = "Unsaved book";

  else {
    name = name.mid(name.lastIndexOf('/')+1);
    name = "CookBook: " + name.mid(0, name.lastIndexOf('.'));
  }

  if (book.modified)
    name += " *";

  setWindowTitle(name);
}

void Book::setModified(bool m) {
  db::Book::current().modified = m;
  setAutoTitle();
}

void Book::closeEvent(QCloseEvent *e) {
  auto &book = db::Book::current();
  if (book.modified) {
    auto ret = QMessageBox::warning(this, "Confirm closing",
                                    "Saving before closing?",
                                    QMessageBox::Yes,
                                    QMessageBox::No,
                                    QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Yes:
      overwriteRecipes();
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
  gui::saveGeometry(this, settings);
}

} // end of namespace gui
