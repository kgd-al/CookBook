#include <random>

#include <QListWidgetItem>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QApplication>
#include <QButtonGroup>
#include <QHeaderView>
#include <QPushButton>
#include <QMainWindow>
#include <QStatusBar>

#include "filterview.h"
#include "autofiltercombobox.hpp"
#include "../db/book.h"

namespace gui {

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

  static constexpr int RandomRole = db::RecipesModel::SortRole+1;

  RecipeFilter (void) {
    qsrand(QDateTime::currentMSecsSinceEpoch());
  }

  QMap<db::ID, int> shuffled_ids;
  void shuffle (void) {
    QList<db::ID> ids;
    for (QModelIndex i=index(0,0); i.isValid(); i=i.sibling(i.row()+1,i.column())) {
      Q_ASSERT(i.isValid());
      ids.push_back(db::ID(i.data(db::IDRole).toInt()));
    }

    for (int i=ids.size()-1; i>0; i--) {
      int j = qrand()%(i+1);
      std::swap(ids[i], ids[j]);
    }

    for (int i=0; i<ids.size(); i++)
      shuffled_ids[ids[i]] = i;

    QSortFilterProxyModel::sort(db::RecipesModel::titleColumn(),
                                Qt::SortOrder(1-sortOrder()));

    shuffled_ids.clear();
  }

  void sort (int column, Qt::SortOrder order) override {
    QSortFilterProxyModel::sort(column, order);
  }

  bool lessThan(const QModelIndex &source_left,
                const QModelIndex &source_right) const override {
    if (!shuffled_ids.empty())
      return shuffled_ids[db::ID(source_left.data(db::IDRole).toInt())]
           < shuffled_ids[db::ID(source_right.data(db::IDRole).toInt())];
    else
      return QSortFilterProxyModel::lessThan(source_left, source_right);
  }

  bool filterAcceptsRow(int source_row,
                        const QModelIndex &source_parent) const override {

    const db::Recipe &r =
      db::Book::current().recipes.at(
        db::ID(sourceModel()->index(source_row, 0, source_parent)
               .data(db::IDRole).toInt()));

//    auto q = qDebug().nospace();

/*#define TEST(NAME) \
  q << #NAME << " [" << (NAME? "on " : "off") << "]: " << NAME.data
*/
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

template <typename T>
FilterView::Entry<T>::Entry (const QString &label, QGridLayout *layout) {
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

// =============================================================================

FilterView::FilterView (QWidget *parent)
  : QWidget(parent), _filter(new RecipeFilter) {

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
  ingredients->widget->setModel(&_filter->ingredients.data);
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
  subrecipes->widget->setModel(&_filter->subrecipes.data);
  layout->addWidget(scontrols = new ListControls (subrecipes->widget,
                                                  QBoxLayout::TopToBottom),
                    layout->rowCount()-1, 3);
  scontrols->editButton()->hide();
  scontrols->setNeedsConfirmation(false);
  connect(subrecipes->cb, &QCheckBox::toggled,
          scontrols, &ListControls::setEnabled);
#endif

  QHBoxLayout *blayout = new QHBoxLayout;

    QPushButton *random = new QPushButton ("Random");
    blayout->addWidget(random);
    connect(random, &QPushButton::clicked, this, &FilterView::random);

    QPushButton *clear = new QPushButton ("Clear");
    blayout->addWidget(clear);
    connect(clear, &QPushButton::clicked, this, &FilterView::clear);

  layout->addLayout(blayout, layout->rowCount(), 0, 1, 4, Qt::AlignRight);

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
          this, &FilterView::processFilterChanges);
  connectMany(subrecipe);
  connect(subrecipe->widget->buttons[1], &QRadioButton::toggled,
          this, &FilterView::processFilterChanges);
  for (const auto &cb: {regimen, status, type, duration})
    connectMany(cb, QOverload<int>::of(&QComboBox::currentIndexChanged));

#ifndef Q_OS_ANDROID
  connectMany(ingredients);
  connectMany(subrecipes);

  connect(ingredients->widget->model(), &QAbstractItemModel::dataChanged,
          this, &FilterView::processFilterChanges);
  ingredients->widget->setItemDelegate(
    new IngredientReferenceDelegate(ingredients->widget->model()));
  connect(icontrols->addButton(), &QToolButton::clicked,
          [this] {
    QModelIndex inserted = _filter->ingredients.data.append();
    ingredients->widget->setCurrentIndex(inserted);
    ingredients->widget->edit(inserted);
  });
  connect(icontrols->delButton(), &QToolButton::clicked,
          this, &FilterView::processFilterChanges);

  connect(subrecipes->widget->model(), &QAbstractItemModel::dataChanged,
          this, &FilterView::processFilterChanges);
  subrecipes->widget->setItemDelegate(
    new RecipeReferenceDelegate (subrecipes->widget->model()));
  connect(scontrols->addButton(), &QToolButton::clicked,
          [this] {
    QModelIndex inserted = _filter->subrecipes.data.append();
    subrecipes->widget->setCurrentIndex(inserted);
    subrecipes->widget->edit(inserted);
  });
  connect(scontrols->delButton(), &QToolButton::clicked,
          this, &FilterView::processFilterChanges);

  /// TODO Remove (?)
  ingredients->cb->setChecked(true);
  subrecipes->cb->setChecked(true);
#else
  title->widget->setInputMethodHints(Qt::ImhNoPredictiveText);
  title->cb->setChecked(true);
#endif
}

QSortFilterProxyModel* FilterView::proxyModel(void) {
  return _filter;
}

template <typename T, typename... SRC>
void FilterView::connectMany (Entry<T> *entry, SRC... members) {
  connect(entry->cb, &QCheckBox::stateChanged,
          this, &FilterView::processFilterChanges);

  using expander = int[];
  (void) expander{
    (
      connect(entry->widget, std::forward<SRC>(members),
              this, &FilterView::processFilterChanges),
      void(),
      0)...
  };
}

void FilterView::processFilterChanges (void) {
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
  _filter->title.active = title->cb->isChecked();
  _filter->title.data = title->widget->text();

  TEST(basic, isYes)
  _filter->basic.active = basic->cb->isChecked();
  _filter->basic.data = basic->widget->isYes();

  TEST(subrecipe, isYes)
  _filter->subrecipe.active = subrecipe->cb->isChecked();
  _filter->subrecipe.data = subrecipe->widget->isYes();

#define UPDATE_CB(NAME) \
  TEST(NAME, currentText) \
  _filter->NAME.active = NAME->cb->isChecked(); \
  _filter->NAME.data = db::ID(NAME->widget->currentData(db::IDRole).toInt());

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
  _filter->ingredients.active = ingredients->cb->isChecked();

//    q << "subrecipes: " << subrecipes->widget->model()->rowCount()
//      << " items:\n";
//    for (const auto &i: filter->subrecipes.data())
//      q << "\t" << i._data << "\n";
  _filter->subrecipes.active = subrecipes->cb->isChecked();
#endif

  _filter->invalidate();
  emit filterChanged();
}

void FilterView::clear (void) {
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

  _filter->ingredients.data.clear();
  _filter->subrecipes.data.clear();
  processFilterChanges();
}

void FilterView::random(void) {
//  _filter->setSortRole(RecipeFilter::RandomRole);
//  qDebug() << "Sort role: " << _filter->sortRole();
//  _filter->sort(db::RecipesModel::titleColumn());
//  qDebug() << "Sorting on column " << _filter->sortColumn();
//  _filter->setSortRole(db::RecipesModel::SortRole);
//  qDebug() << "Sort role: " << _filter->sortRole();
  _filter->shuffle();
}

} // end of namespace gui
