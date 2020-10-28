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
#include <QTableView>
#include <QHeaderView>
#include <QCheckBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QPushButton>

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
      for (const QIcon &i: { /*r->*/
                             r->regimen->decoration, r->status->decoration,
                             r->type->decoration, r->duration->decoration }) {
        int w = h;
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
  static constexpr int C = 1;
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
  static constexpr int C = 2;
  std::array<QString, C> _data;

  const QString& data (int c) const {
    return _data[c];
  }

  bool setData (int c, const QVariant &value) {
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
    return T::C;
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

  bool setData (const QModelIndex &index, const QVariant &value,
                int role)  override {
    qDebug() << __PRETTY_FUNCTION__ << "(" << index << value << ")";
    if (role != Qt::EditRole) return false;
    bool ok = _data[index.row()].setData(index.column(), value);
    if (ok) emit dataChanged(index, index, {role});
    return ok;
  }

  QModelIndex append (void) {
    int rows = rowCount();
    beginInsertRows(QModelIndex(), rows, rows);
    _data.append(T());
    endInsertRows();
    return index(rows, 0);
  }

  bool removeRows(int row, int count, const QModelIndex &parent) override {
    beginRemoveRows(parent, row, row+count-1);
    for (int i=0; i<count; i++) _data.removeAt(row);
    endRemoveRows();
    return true;
  }

  const auto& operator() (void) const {
    return _data;
  }

  void clear (void) {
    beginResetModel();
    _data.clear();
    endResetModel();
  }

private:
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
  Data<IngredientsModel> ingredients;

  using RecipesModel = EditableModel<RecipeReference>;
  Data<RecipesModel> subrecipes;

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
        && !r.title.contains(title.data, Qt::CaseInsensitive)) return false;

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

    if (ingredients) {
      int found = 0;
      for (const auto &s: ingredients.data()) {
        if (s._data[0].isEmpty()) {
          found++;
          continue;
        }
//        q << "\t" << s._data[0];
//        if (!s._data[1].isEmpty())  q << " (" << s._data[1] << ")";
//        q << "\n";
        for (const auto &i: r.ingredients) {
          if (i->etype != db::EntryType::Ingredient) continue;
          auto ientry = static_cast<db::IngredientEntry*>(i.data());
//          q << "\t\t" << ientry->idata->text << " (" << ientry->qualif << ")\n";
          if (!ientry->idata->text.contains(s._data[0], Qt::CaseInsensitive))
            continue;
          if (!ientry->qualif.contains(s._data[1], Qt::CaseInsensitive))
            continue;
          found++;
          break;
        }
      }
      if (found != ingredients.data().size())  return false;
    }

    if (subrecipes) {
      int found = 0;
      for (const auto &s: subrecipes.data()) {
        if (s._data.isEmpty()) continue;
        for (const auto &i: r.ingredients) {
          if (i->etype != db::EntryType::SubRecipe) continue;
          auto sentry = static_cast<db::SubRecipeEntry*>(i.data());
          if (!sentry->recipe->title.contains(s._data, Qt::CaseInsensitive))
            continue;
          found++;
          break;
        }
      }
      if (found != subrecipes.data().size())  return false;
    }

#undef TEST
//    q << "Accepting row\n";
    return true;
  }
};

struct IngredientReferenceDelegate : public QStyledItemDelegate {
  IngredientReferenceDelegate (QAbstractItemModel *m) : model(m) {
    cbox = new AutoFilterComboBox(QComboBox::NoInsert);
    cbox->setForceCompletion(false);
    cbox->setModel(&db::Book::current().ingredients);
    cbox->setAutoFillBackground(true);
    cbox->lineEdit()->setPlaceholderText("Ingrédient");
    connect(cbox->lineEdit(), &QLineEdit::textChanged,
            [this] {
      if(currentIndex.isValid())
        model->setData(currentIndex, cbox->lineEdit()->text(), Qt::EditRole);
    });

    ledit = new QLineEdit;
    ledit->setPlaceholderText("Qualificatif(s)");
    connect(ledit, &QLineEdit::textChanged,
            [this] {
      if(currentIndex.isValid())
        model->setData(currentIndex, ledit->text(), Qt::EditRole);
    });
  }

  QWidget* createEditor(QWidget *parent,
                        const QStyleOptionViewItem &/*option*/,
                        const QModelIndex &index) const override {
    QWidget *w = nullptr;
    int c = index.column();
    if (c == 0)       w = cbox;
    else if (c == 1)  w = ledit;

    if (!w) return nullptr;

    w->setParent(parent);
    Q_ASSERT(!currentIndex.isValid());
    currentIndex = index;
    Q_ASSERT(currentIndex.isValid());
    externalSet = true;
    return w;
  }

  void destroyEditor(QWidget *editor, const QModelIndex &index) const override {
    Q_ASSERT(editor == cbox || editor == ledit);
    Q_ASSERT(index == currentIndex);
    cbox->setParent(nullptr);
    currentIndex = QModelIndex();
  }

  void setEditorData(QWidget */*editor*/,
                     const QModelIndex &index) const override {
    QString text = index.data().toString();
    if (index.column() == 0) {
      if (cbox->currentText() != text) cbox->setCurrentText(text);
      if (externalSet)  cbox->lineEdit()->selectAll();

    } else if (index.column() == 1) {
      if (ledit->text() != text)  ledit->setText(text);
      if (externalSet)  ledit->selectAll();
    }

    externalSet = false;
  }

  void setModelData(QWidget */*editor*/,
                    QAbstractItemModel *model,
                    const QModelIndex &index) const override {
    QString text;
    if (index.column() == 0)      text = cbox->currentText();
    else if (index.column() == 1) text = ledit->text();
    model->setData(index, text, Qt::EditRole);
  }

private:
  AutoFilterComboBox *cbox;
  QLineEdit *ledit;
  QAbstractItemModel *model;
  mutable QModelIndex currentIndex;
  mutable bool externalSet = false;
};

// =============================================================================

struct RecipeReferenceDelegate : public QStyledItemDelegate {
  RecipeReferenceDelegate (QAbstractItemModel *m) : model(m) {
    cbox = new AutoFilterComboBox(QComboBox::NoInsert);
    cbox->setForceCompletion(false);
    cbox->setModel(&db::Book::current().recipes);
    cbox->setAutoFillBackground(true);
    cbox->lineEdit()->setPlaceholderText("Sous-Recette");
    connect(cbox->lineEdit(), &QLineEdit::textChanged,
            [this] {
      if(currentIndex.isValid())
        model->setData(currentIndex, cbox->lineEdit()->text(), Qt::EditRole);
    });
  }

  QWidget* createEditor(QWidget *parent,
                        const QStyleOptionViewItem &/*option*/,
                        const QModelIndex &index) const override {
    cbox->setParent(parent);
    Q_ASSERT(!currentIndex.isValid());
    currentIndex = index;
    Q_ASSERT(currentIndex.isValid());
    qDebug() << __PRETTY_FUNCTION__ << "(" << parent << index << ")";
    externalSet = true;
    return cbox;
  }

  void destroyEditor(QWidget *editor, const QModelIndex &index) const override {
    qDebug() << __PRETTY_FUNCTION__ << "(" << index << ")";
    Q_ASSERT(editor == cbox);
    Q_ASSERT(index == currentIndex);
    cbox->setParent(nullptr);
    currentIndex = QModelIndex();
  }

  void setEditorData(QWidget *editor, const QModelIndex &index) const override {
    qDebug() << __PRETTY_FUNCTION__ << "(" << index << ")";
    Q_ASSERT(editor == cbox);
    QString text = index.data().toString();
    if (cbox->currentText() != text)  cbox->setCurrentText(text);
    if (externalSet)  cbox->lineEdit()->selectAll();
    externalSet = false;
  }

  void setModelData(QWidget *editor,
                    QAbstractItemModel *model,
                    const QModelIndex &index) const override {

    qDebug() << __PRETTY_FUNCTION__ << "(" << index << ")";
    Q_ASSERT(editor == cbox);
    Q_ASSERT(model == this->model);
    model->setData(index, cbox->currentText(), Qt::EditRole);
  }

private:
  AutoFilterComboBox *cbox;
  QAbstractItemModel *model;
  mutable QModelIndex currentIndex;
  mutable bool externalSet = false;
};


// =============================================================================

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
  Entry<QTableView> *ingredients;
  Entry<QListView> *subrecipes;
  ListControls *icontrols, *scontrols;

  FilterView (RecipeFilter *f, QWidget *parent)
    : QWidget(parent), filter(f) {

    QGridLayout *layout = new QGridLayout;

    title = new Entry<QLineEdit> ("Titre", layout);
    regimen = new Entry<QComboBox> ("Régime", layout);
    status = new Entry<QComboBox> ("Status", layout);
    type = new Entry<QComboBox> ("Type", layout);
    duration = new Entry<QComboBox> ("Durée", layout);
    basic = new Entry<QGroupBox> ("Basique", layout);
    ingredients = new Entry<QTableView> ("Ingrédients", layout);
    ingredients->widget->setModel(&filter->ingredients.data);
    {
      auto h = ingredients->widget->horizontalHeader();
      h->hide();
//      h->setSectionResizeMode(0, QHeaderView::ResizeToContents);
      h->setSectionResizeMode(1, QHeaderView::Stretch);
    }

    ingredients->widget->verticalHeader()->hide();
    layout->addWidget(icontrols = new ListControls (ingredients->widget,
                                                    QBoxLayout::TopToBottom),
                      layout->rowCount()-1, 3);
    icontrols->editButton()->hide();
    icontrols->setNeedsConfirmation(false);
    connect(ingredients->cb, &QCheckBox::toggled,
            icontrols, &ListControls::setEnabled);

    subrecipes = new Entry<QListView> ("Recettes", layout);
    subrecipes->widget->setModel(&filter->subrecipes.data);
    layout->addWidget(scontrols = new ListControls (subrecipes->widget,
                                                    QBoxLayout::TopToBottom),
                      layout->rowCount()-1, 3);
    scontrols->editButton()->hide();
    scontrols->setNeedsConfirmation(false);
    connect(subrecipes->cb, &QCheckBox::toggled,
            scontrols, &ListControls::setEnabled);

    QPushButton *clear = new QPushButton ("Clear");
    layout->addWidget(clear, layout->rowCount(), 0, 1, 4, Qt::AlignRight);
    connect(clear, &QPushButton::clicked, this, &FilterView::clear);

    layout->setColumnStretch(0, 0);
    layout->setColumnStretch(1, 0);
    layout->setColumnStretch(2, 1);
    layout->setColumnStretch(3, 0);
    setLayout(layout);

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
    connectMany(ingredients);
    connectMany(subrecipes);

    connect(ingredients->widget->model(), &QAbstractItemModel::dataChanged,
            this, &FilterView::filterChanged);
    ingredients->widget->setItemDelegate(
      new IngredientReferenceDelegate(ingredients->widget->model()));
    connect(icontrols->addButton(), &QToolButton::clicked,
            [this] {
      QModelIndex inserted = filter->ingredients.data.append();
      ingredients->widget->setCurrentIndex(inserted);
      ingredients->widget->edit(inserted);
    });
    connect(icontrols->delButton(), &QToolButton::clicked,
            this, &FilterView::filterChanged);

    connect(subrecipes->widget->model(), &QAbstractItemModel::dataChanged,
            this, &FilterView::filterChanged);
    subrecipes->widget->setItemDelegate(
      new RecipeReferenceDelegate (subrecipes->widget->model()));
    connect(scontrols->addButton(), &QToolButton::clicked,
            [this] {
      QModelIndex inserted = filter->subrecipes.data.append();
      subrecipes->widget->setCurrentIndex(inserted);
      subrecipes->widget->edit(inserted);
    });
    connect(scontrols->delButton(), &QToolButton::clicked,
            this, &FilterView::filterChanged);    

    /// TODO Remove
    ingredients->cb->setChecked(true);
    subrecipes->cb->setChecked(true);
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

    auto q = qDebug().nospace();

    q << "ingredients: " << ingredients->widget->model()->rowCount()
      << " items:\n";
    for (const auto &i: filter->ingredients.data())
      q << "\t" << i._data[0] << " (" << i._data[1] << ")\n";
    filter->ingredients.active = ingredients->cb->isChecked();

    q << "subrecipes: " << subrecipes->widget->model()->rowCount()
      << " items:\n";
    for (const auto &i: filter->subrecipes.data())
      q << "\t" << i._data << "\n";
    filter->subrecipes.active = subrecipes->cb->isChecked();
//    subrecipes->widget->

    filter->invalidate();
  }

  void clear (void) {
    for (QCheckBox *cb: { title->cb, regimen->cb, status->cb, type->cb,
                          duration->cb, basic->cb, ingredients->cb,
                          subrecipes->cb }) {

      cb->blockSignals(true);
      cb->setChecked(false);
      cb->blockSignals(false);
    }
    title->widget->clear();
    filter->ingredients.data.clear();
    filter->subrecipes.data.clear();
    filterChanged();
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
      overwriteRecipes();
  }
}

void Book::showRecipe(const QModelIndex &index) {
  Recipe recipe (this);
  connect(&recipe, &Recipe::validated, [this] { setModified(true); });
  recipe.show(&db::Book::current()
              .recipes.at(db::ID(index.data(db::IDRole).toInt())),
              true);
  if (db::Book::current().modified)
    overwriteRecipes();
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
