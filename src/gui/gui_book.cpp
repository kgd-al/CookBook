#include <QApplication>
#include <QFileDialog>
#include <QFontDialog>
#include <QMessageBox>
#include <QDesktopServices>

#include <QStyledItemDelegate>
#include <QPainter>

#include <QMenuBar>
#include <QMenu>

#include <QGridLayout>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>

#include "gui_book.h"
#include "autofiltercombobox.hpp"
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

struct RecipeListDelegate : public QStyledItemDelegate {
  RecipeListDelegate (QObject *parent) : QStyledItemDelegate (parent) {}

//  QSize sizeHint (const QStyleOptionViewItem &option,
//                  const QModelIndex &index) const override {
//    return QSize(db::RECIPE_ICONS_SIZE+)
//  }
  void paint (QPainter *painter, const QStyleOptionViewItem &option,
              const QModelIndex &index) const {
    static constexpr int S = 3, M = 5;
    int h = option.rect.height() - 2*S;
    auto *r = index.data(db::PtrRole).value<const db::Recipe*>();

    if (option.state & QStyle::State_Selected)
      painter->fillRect(option.rect, option.palette.highlight());

    painter->save();
      painter->setRenderHint(QPainter::Antialiasing, true);
      painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
      painter->translate(option.rect.x(), option.rect.y());

      int x = 0;
      for (const QPixmap &p: { r->regimen->decoration, r->status->decoration,
                               r->type->decoration, r->duration->decoration }) {
        int w = h;
        QIcon i;  /// TODO Overly expensive but otherwise not smooth
        i.addPixmap(p);
        i.paint(painter, x, S, w, h);
        x += w + M;
      }
    painter->restore();

    QStyleOptionViewItem itemOption(option);
    initStyleOption(&itemOption, index);
    itemOption.rect.adjust(x, 0, 0, 0);
    QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &itemOption,
                                       painter, nullptr);
  }
};

struct RecipeReference {
  QString _data;

  const QString& data (int) const {
    return _data;
  }

  bool setData (int, const QVariant &value) {
    _data = value.toString();
    return true;
  }
};

struct IngredientReference {
  QStringList _data;

  const QString& data (int c) const {
    static const QString error = "ERROR";
    if (c >= _data.size()) return error;
    return _data[c];
  }

  bool setData (int c, const QVariant &value) {
    if (c >= _data.size())  return false;
    _data[c] = value.toString();
    return true;
  }
};

template <typename T>
struct EditableModel : public QAbstractTableModel {
  int rowCount(const QModelIndex& = QModelIndex()) const override {
    return _data.size();
  }

  int columnCount(const QModelIndex& = QModelIndex()) const override {
    return 1;
  }

  QVariant data (const QModelIndex &index, int role) const override {
    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      return _data.at(index.row()).data(index.column());
    default:
      return QVariant();
    }
  }

  Qt::ItemFlags flags (const QModelIndex &index) const override {
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
  }

  bool setData (const QModelIndex &index, const QVariant &value) {
    return _data[index.row()].setData(index.column(), value);
  }

  void append (void) {
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    _data.append(T());
    endInsertRows();
  }

  QList<T> _data;
};

struct RecipeFilter : public QSortFilterProxyModel {
  template <typename T>
  struct Data {
    T data;
    bool active = false;
    operator bool (void) const { return active; }
  };
  Data<QString> title;
  Data<db::ID> regimen, status, type, duration;
  Data<bool> basic;

  using IngredientsModel = EditableModel<IngredientReference>;
  IngredientsModel ingredients;

  using RecipesModel = EditableModel<RecipeReference>;
  RecipesModel subrecipes;

  bool filterAcceptsRow(int source_row,
                        const QModelIndex &source_parent) const override {

    const db::Recipe &r =
      db::Book::current().recipes.at(
        db::ID(sourceModel()->index(source_row, 0, source_parent)
               .data(db::IDRole).toInt()));

//    auto q = qDebug().nospace();

#define TEST(NAME) \
  q << #NAME << " [" << (NAME? "on " : "off") << "]: " << NAME.data


//    TEST(title) << " !C " << r.title << "? " << !r.title.contains(title.data)
//                << "\n";
    if (title && !title.data.isEmpty()
        && !r.title.contains(title.data)) return false;

//    TEST(NAME) << NAME.data << " != " << r.NAME->id << "? " \
//               << (r.NAME->id != NAME.data) << "\n"; \

#define TEST_CB(NAME) \
  if (NAME && r.NAME->id != NAME.data) return false;

    TEST_CB(regimen)
    TEST_CB(status)
    TEST_CB(type)
    TEST_CB(duration)
#undef TEST_CB

//    TEST(basic) << " != " << r.used << "? " << (r.used != basic.data) << "\n";
    if (basic && r.used != basic.data)  return false;

#undef TEST
//    q << "Accepting row\n";
    return true;
  }
};

struct RecipeReferenceDelegate : public QStyledItemDelegate {
  RecipeReferenceDelegate (void) {}

//  void paint(QPainter *painter, const QStyleOptionViewItem &option,
//             const QModelIndex &index) const override {

//  }

  QWidget* createEditor(QWidget *parent,
                        const QStyleOptionViewItem &/*option*/,
                        const QModelIndex &/*index*/) const override {
    auto cbox = new AutoFilterComboBox(QComboBox::NoInsert, parent);
    cbox->setModel(&db::Book::current().recipes);
    return cbox;
  }

  void setEditorData(QWidget *editor,
                     const QModelIndex &index) const override {
    auto *cbox = static_cast<AutoFilterComboBox*>(editor);
    cbox->setCurrentIndex(cbox->findData(index.data(db::IDRole), db::IDRole));
    cbox->lineEdit()->selectAll();
  }

//  void updateEditorGeometry(QWidget */*editor*/,
//                            const QStyleOptionViewItem &/*option*/,
//                            const QModelIndex &index) const override {

//  }

  void setModelData(QWidget *editor,
                    QAbstractItemModel *model,
                    const QModelIndex &index) const override {
    auto *rmodel = static_cast<RecipeFilter::RecipesModel*>(model);
    auto *cbox = static_cast<AutoFilterComboBox*>(editor);
    rmodel->setData(index, cbox->currentData(db::IDRole));
  }
};

struct FilterView : public QWidget {
  RecipeFilter *filter;

  template <typename T>
  struct Entry {
    QCheckBox *cb;
    T *widget;

    Entry (const QString &label, QGridLayout *layout) {
      QLabel *l;
      int r = layout->rowCount(), c = 0;
      layout->addWidget(cb = new QCheckBox, r, c++);
      layout->addWidget(l = new QLabel(label), r, c++);
      if (!std::is_base_of<QAbstractItemView,T>::value)
        layout->addWidget(widget = new T, r, c++);
      else
        layout->addWidget(widget = new T, r+1, 1, 1, 2);

      QWidget::connect(cb, &QCheckBox::toggled, widget, &QWidget::setEnabled);
      cb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
      l->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
      cb->setChecked(true);
      cb->toggle();
    }
  };
  Entry<QLineEdit> *title;
  Entry<QComboBox> *regimen, *status, *type, *duration;
  Entry<QGroupBox> *basic;
  QRadioButton *basic_yes;
  Entry<QListView> *ingredients, *subrecipes;
  ListControls *icontrols, *scontrols;

  FilterView (RecipeFilter *filter, QWidget *parent)
    : QWidget(parent), filter(filter) {

    QGridLayout *layout = new QGridLayout;

    title = new Entry<QLineEdit> ("Titre", layout);
    regimen = new Entry<QComboBox> ("Régime", layout);
    status = new Entry<QComboBox> ("Status", layout);
    type = new Entry<QComboBox> ("Type", layout);
    duration = new Entry<QComboBox> ("Durée", layout);
    basic = new Entry<QGroupBox> ("Basique", layout);
    ingredients = new Entry<QListView> ("Ingrédients", layout);
    ingredients->widget->setModel(&filter->ingredients);
    layout->addWidget(icontrols = new ListControls (ingredients->widget,
                                                    QBoxLayout::TopToBottom),
                      layout->rowCount()-1, 3);
    subrecipes = new Entry<QListView> ("Recettes", layout);
    subrecipes->widget->setModel(&filter->subrecipes);
    layout->addWidget(scontrols = new ListControls (subrecipes->widget,
                                                    QBoxLayout::TopToBottom),
                      layout->rowCount()-1, 3);

    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 1);
    layout->setColumnStretch(3, 0);

    regimen->widget->setModel(db::getStaticModel<db::RegimenData>());
    status->widget->setModel(db::getStaticModel<db::StatusData>());
    type->widget->setModel(db::getStaticModel<db::DishTypeData>());
    duration->widget->setModel(db::getStaticModel<db::DurationData>());

    QHBoxLayout *basic_layout = new QHBoxLayout;
    basic_layout->addWidget(basic_yes = new QRadioButton("oui"));
    basic_layout->addWidget(new QRadioButton("non"));
    basic->widget->setLayout(basic_layout);
    basic_yes->setChecked(true);

    connectMany(title, &QLineEdit::textChanged);
    for (const auto &cb: {regimen, status, type, duration})
      connectMany(cb, qOverload<int>(&QComboBox::currentIndexChanged));
    connectMany(basic);
    connect(basic_yes, &QRadioButton::toggled, this, &FilterView::filterChanged);

//    ingredients->widget->setModel(filter->ingredients);
    connect(icontrols->addButton(), &QToolButton::clicked,
            [this] {
//      ingredients->widget->model()->in
//      QListWidgetItem *item = new QListWidgetItem;
//      item->setSizeHint(iholder->sizeHint());
//      ingredients->widget->setItemWidget(item, iholder);
//      ingredients->widget->addItem(item);
    });

    subrecipes->widget->setItemDelegate(new RecipeReferenceDelegate);
    connect(scontrols->addButton(), &QToolButton::clicked,
            &filter->subrecipes, &RecipeFilter::RecipesModel::append);

    setLayout(layout);
  }

  template <typename T, typename... SRC>
  void connectMany (Entry<T> *entry, SRC... members) {
    connect(entry->cb, &QCheckBox::stateChanged,
            this, &FilterView::filterChanged);

    using expander = int[];
    (void) expander{
      (
        connect(entry->widget, std::forward<SRC>(members),
                this, &FilterView::filterChanged),
        void(),
        0)...
    };
  }

  void filterChanged (void) {
    filter->title.active = title->cb->isChecked();
    filter->title.data = title->widget->text();

#define UPDATE_CB(NAME) \
  filter->NAME.active = NAME->cb->isChecked(); \
  filter->NAME.data = db::ID(NAME->widget->currentData(db::IDRole).toInt());

    UPDATE_CB(regimen)
    UPDATE_CB(status)
    UPDATE_CB(type)
    UPDATE_CB(duration)
#undef UPDATE_CB

  filter->basic.active = basic->cb->isChecked();
  filter->basic.data = basic_yes->isChecked();

    filter->invalidate();
  }
};

Book::Book(QWidget *parent) : QMainWindow(parent) {
    _splitter = new QSplitter;

      _recipes = new QListView;
      _recipes->setEditTriggers(QAbstractItemView::NoEditTriggers);
      _recipes->setModel(_proxy = new RecipeFilter);
      _recipes->setItemDelegate(new RecipeListDelegate (this));
      _proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
      _proxy->setDynamicSortFilter(true);
      _proxy->sort(0);
      _splitter->addWidget(_recipes);

      _splitter->addWidget(_filter = new FilterView (_proxy, this));

  setCentralWidget(_splitter);


  connect(_recipes, &QListView::activated,
          this, &Book::showRecipe);

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
          db::fontChanged(font);
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
  _splitter->setSizes(settings.value("splitter").value<QList<int>>());

  gui::restoreGeometry(this, settings);
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
    if (db::Book::current().modified)
      saveRecipes();
  }
}

void Book::showRecipe(const QModelIndex &index) {
  Recipe recipe (this);
  connect(&recipe, &Recipe::validated, [this] { setModified(true); });
  recipe.show(&db::Book::current()
              .recipes.at(db::ID(index.data(db::IDRole).toInt())),
              true);
  if (db::Book::current().modified)
    saveRecipes();
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
    _proxy->setSourceModel(&book.recipes);
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
  settings.setValue("splitter", QVariant::fromValue(_splitter->sizes()));
  gui::saveGeometry(this, settings);
}

} // end of namespace gui
