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
#include "androidspecifics.hpp"
#endif

#include "common.h"
#include "gui_book.h"
#include "autofiltercombobox.hpp"
#include "ingredientsmanager.h"
#include "updatemanager.h"
#include "repairsmanager.h"
#include "settings.h"

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
      for (const QIcon &i: { r->basicIcon(), r->subrecipeIcon(),
                             r->regimen->decoration, r->type->decoration,
                             r->duration->decoration, r->status->decoration }) {
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
  using database_t = QList<T>;

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

  const database_t& operator() (void) const {
    return _data;
  }

  void clear (void) {
    beginResetModel();
    _data.clear();
    endResetModel();
  }

private:
  database_t _data;
};

struct RecipeFilter : public QSortFilterProxyModel {
  template <typename T>
  struct Data {
    T data;
    bool active = false;
    operator bool (void) const { return active; }
  };
  Data<QString> title;
  Data<bool> basic, subrecipe;
  Data<db::ID> regimen, type, duration, status;

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

//#define TEST(NAME) \
//  q << #NAME << " [" << (NAME? "on " : "off") << "]: " << NAME.data

//    TEST(title) << " !C " << r.title << "? " << !r.title.contains(title.data)
//                << "\n";
    if (title && !title.data.isEmpty()
        && !r.title.contains(title.data, Qt::CaseInsensitive)) return false;

//    TEST(basic) << " != " << r.basic << "? " << (r.basic != basic.data) << "\n";
    if (basic && r.basic != basic.data)  return false;

//    TEST(subrecipe) << " != " << r.used << "? " << (bool(r.used) != subrecipe.data) << "\n";
    if (subrecipe && bool(r.used) != subrecipe.data)  return false;

#define TEST_CB(NAME) \
  if (NAME && r.NAME->id != NAME.data) return false;

    TEST_CB(regimen)
    TEST_CB(status)
    TEST_CB(type)
    TEST_CB(duration)
#undef TEST_CB

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
    cbox->setModelColumn(db::RecipesModel::titleColumn());
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

struct YesNoGroupBox : public QWidget {
  std::array<QRadioButton*, 2> buttons;
  YesNoGroupBox (void) {
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(buttons[0] = new QRadioButton("non"));
    layout->addWidget(buttons[1] = new QRadioButton("oui"));
    buttons[0]->setChecked(true);
    setLayout(layout);

    QButtonGroup *group = new QButtonGroup (this);
    group->addButton(buttons[0], 0);
    group->addButton(buttons[1], 1);
  }

  bool isYes (void) const {
    return buttons[1]->isChecked();
  }
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
  Entry<YesNoGroupBox> *basic, *subrecipe;
  QRadioButton *basic_yes, *subrecipe_yes;
  Entry<QComboBox> *regimen, *status, *type, *duration;

#ifndef Q_OS_ANDROID
  Entry<QTableView> *ingredients;
  Entry<QListView> *subrecipes;
  ListControls *icontrols, *scontrols;
#endif

  FilterView (RecipeFilter *f, QWidget *parent)
    : QWidget(parent), filter(f) {

    QGridLayout *layout = new QGridLayout;

    title = new Entry<QLineEdit> ("Titre", layout);
    basic = new Entry<YesNoGroupBox> ("Basique", layout);
    subrecipe = new Entry<YesNoGroupBox> ("Sous-Recette", layout);
    regimen = new Entry<QComboBox> ("Régime", layout);
    type = new Entry<QComboBox> ("Type", layout);
    duration = new Entry<QComboBox> ("Durée", layout);
    status = new Entry<QComboBox> ("Status", layout);

#ifndef Q_OS_ANDROID
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
#endif

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

    connectMany(title, &QLineEdit::textChanged);
    connectMany(basic);
    connect(basic->widget->buttons[1], &QRadioButton::toggled,
            this, &FilterView::filterChanged);
    connectMany(subrecipe);
    connect(subrecipe->widget->buttons[1], &QRadioButton::toggled,
            this, &FilterView::filterChanged);
    for (const auto &cb: {regimen, status, type, duration})
      connectMany(cb, QOverload<int>::of(&QComboBox::currentIndexChanged));

#ifndef Q_OS_ANDROID
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

    /// TODO Remove (?)
    ingredients->cb->setChecked(true);
    subrecipes->cb->setChecked(true);
#else
    title->widget->setInputMethodHints(Qt::ImhNoPredictiveText);
    title->cb->setChecked(true);
#endif
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
#if 0
    auto q = qDebug().nospace();
    q << __PRETTY_FUNCTION__ << "\n";

#define TEST(NAME, FUNC) \
    q << "[" << (NAME->cb->isChecked() ? "on " : "off") << "] " \
      << #NAME << ": " << NAME->widget->FUNC() << "\n";
#else
#define TEST(X,Y)
#endif

    TEST(title, text)
    filter->title.active = title->cb->isChecked();
    filter->title.data = title->widget->text();

    TEST(basic, isYes)
    filter->basic.active = basic->cb->isChecked();
    filter->basic.data = basic->widget->isYes();

    TEST(subrecipe, isYes)
    filter->subrecipe.active = subrecipe->cb->isChecked();
    filter->subrecipe.data = subrecipe->widget->isYes();

#define UPDATE_CB(NAME) \
  TEST(NAME, currentText) \
  filter->NAME.active = NAME->cb->isChecked(); \
  filter->NAME.data = db::ID(NAME->widget->currentData(db::IDRole).toInt());

    UPDATE_CB(regimen)
    UPDATE_CB(status)
    UPDATE_CB(type)
    UPDATE_CB(duration)
#undef UPDATE_CB

#ifndef Q_OS_ANDROID
//    q << "ingredients: " << ingredients->widget->model()->rowCount()
//      << " items:\n";
//    for (const auto &i: filter->ingredients.data())
//      q << "\t" << i._data[0] << " (" << i._data[1] << ")\n";
    filter->ingredients.active = ingredients->cb->isChecked();

//    q << "subrecipes: " << subrecipes->widget->model()->rowCount()
//      << " items:\n";
//    for (const auto &i: filter->subrecipes.data())
//      q << "\t" << i._data << "\n";
    filter->subrecipes.active = subrecipes->cb->isChecked();
#endif

    filter->invalidate();

    if (filter->sourceModel()) {
      QString msg;
      QTextStream qts (&msg);
      qts << filter->rowCount() << " recettes";
      if (filter->rowCount() != filter->sourceModel()->rowCount())
        qts << " (sur un total de " << filter->sourceModel()->rowCount() << ")";
      static_cast<QMainWindow*>(this->topLevelWidget())
          ->statusBar()->showMessage(msg, 5000);
    }
  }

  void clear (void) {
    for (QCheckBox *cb: { title->cb,
                          basic->cb, subrecipe->cb,
                          regimen->cb, status->cb, type->cb, duration->cb,
#ifndef Q_OS_ANDROID
                          ingredients->cb, subrecipes->cb
#endif
                        }) {

      cb->blockSignals(true);
      cb->setChecked(false);
      cb->blockSignals(false);
    }

    title->widget->blockSignals(true);
    title->widget->clear();
    title->widget->blockSignals(false);

    filter->ingredients.data.clear();
    filter->subrecipes.data.clear();
    filterChanged();
  }
};

Book::Book(QWidget *parent) : QMainWindow(parent) {
#ifndef Q_OS_ANDROID
  auto sorientation = Qt::Horizontal;
#else
  auto sorientation = Qt::Vertical;
#endif
    _splitter = new QSplitter (sorientation);

      _recipes = new QTableView;
      _recipes->setEditTriggers(QAbstractItemView::NoEditTriggers);
      _recipes->setModel(_proxy = new RecipeFilter);
      _splitter->addWidget(_recipes);
      auto rheader = _recipes->horizontalHeader();
      Q_ASSERT(rheader);
#ifdef Q_OS_ANDROID
      rheader->setMinimumSectionSize(2*db::iconSize());
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
      _proxy->setSortRole(db::RecipesModel::SortRole);
      _proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
      _proxy->setDynamicSortFilter(true);
      _recipes->sortByColumn(db::RecipesModel::titleColumn(),
                             Qt::AscendingOrder);

      _splitter->addWidget(_filter = new FilterView (_proxy, this));
#ifndef Q_OS_ANDROID
      auto ftoggle = new QShortcut(QKeySequence("Ctrl+F"), this);
      connect(ftoggle, &QShortcut::activated, this, &Book::toggleFilterArea);
#endif

  setCentralWidget(_splitter);

  connect(_recipes, &QTableView::activated, this, &Book::showRecipe);

  QMenuBar *bar = menuBar();
    QMenu *m_book = bar->addMenu("Book");
      m_book->addAction(QIcon::fromTheme(""), "Load",
                        this, QOverload<>::of(&Book::loadRecipes),
                        QKeySequence("Ctrl+O"));

#ifndef Q_OS_ANDROID
      m_book->addAction(QIcon::fromTheme(""), "Save",
                        [this] { overwriteRecipes(false); },
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
  grabGesture(android::SingleFingerSwipeRecognizer::type());
#endif

  statusBar();  // Invoke now to make space

  auto &settings = localSettings(this);
  QString lastBook = settings.value("lastBook").toString();
  if (!lastBook.isEmpty())  loadRecipes(lastBook);

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
    drecipe.setIndex(_proxy->mapFromSource(source_index));
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
    _proxy->setSourceModel(&book.recipes);
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

#ifndef Q_OS_ANDROID
  if (book.modified) {
    auto ret = QMessageBox::warning(this, "Confirm closing",
                                    "Saving before closing?",
                                    QMessageBox::Yes,
                                    QMessageBox::No,
                                    QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Yes:
      overwriteRecipes(false);
      break;
    case QMessageBox::No:
      e->accept();
      break;
    case QMessageBox::Cancel:
    default:
      e->ignore();
    }
  }
#else
  (void)e;
#endif

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
