#include <QVBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QCheckBox>
#include <QLabel>
#include <QTextStream>
#include <QDialogButtonBox>

#include "repairsmanager.h"
#include "../db/book.h"

#include <QDebug>

namespace gui {

QIcon shortDesc (Analysis a) {
  switch (a) {
  case Analysis::COUNT_RECIPE:
  case Analysis::COUNT_INGREDIENT:
  case Analysis::COUNT_UNIT:

  case Analysis::HOMONYMOUS_INGREDIENT:
  case Analysis::HOMONYMOUS_UNIT:
    return QIcon::fromTheme("dialog-error");

  case Analysis::UNUSED_INGREDIENT:
  case Analysis::UNUSED_UNIT:
    return QIcon::fromTheme("dialog-warning");

  default:
    return QIcon();
  }
}

QString longDesc (Analysis a) {
  QMap<Analysis, QString> map {
    {          Analysis::COUNT_RECIPE, "Décompte de sous-recette invalide"  },
    {      Analysis::COUNT_INGREDIENT, "Décompte d'ingrédient invalide"     },
    {            Analysis::COUNT_UNIT, "Décompte d'unité invalide"          },

    { Analysis::HOMONYMOUS_INGREDIENT, "Ingrédients homonymes"  },
    {       Analysis::HOMONYMOUS_UNIT, "Unités homonymes"       },

    {     Analysis::UNUSED_INGREDIENT, "Ingrédient(s) inutilisé"  },
    {           Analysis::UNUSED_UNIT, "Unitée(s) inutilisée"     },
  };
  return map.value(a);
}

struct CachedAnalysis {
  template <typename T>
  using count_t = std::map<T*, int>;
  count_t<db::Recipe> rcounts;
  count_t<db::IngredientData> icounts;
  count_t<db::UnitData> ucounts;

  template <typename K, typename T>
  using homonymous_t = std::map<K,
                                std::map<T*,
                                  std::vector<
                                    std::pair<db::Recipe*,
                                              db::IngredientEntry*>>>>;

  homonymous_t<std::pair<const QString&, const QString&>,
               db::IngredientData> ihomonymous;
  homonymous_t<std::reference_wrapper<const QString>, db::UnitData> uhomonymous;

  void reset (void) {
    rcounts.clear();
    icounts.clear();
    ucounts.clear();
    ihomonymous.clear();
    uhomonymous.clear();
  }
};

class Summary : public QWidget {
  struct Logger {
    QTextEdit *editer;
    QString text;
    QTextStream streamer;
    QDebug debug;

    Logger (QTextEdit *editer) : editer(editer), streamer(&text),
                                 debug(qDebug().noquote().nospace())
      {}

    Logger (const Logger &that) : Logger(that.editer) {
      text = that.text;
    }

    ~Logger (void) { editer->append(text); }

    template <typename T>
    Logger& operator<< (const T &value) {
      streamer << value;
      debug << value;
      return *this;
    }

    Logger& operator<< (const char *s) {
      return this->operator <<(QString(s));
    }
  };

  const Analysis a;
  const QIcon icon;
  QLabel *label;
  QTextEdit *text;
  QCheckBox *repair;

public:
  Summary (Analysis a) : a(a), icon(shortDesc(a)) {
    QVBoxLayout *layout = new QVBoxLayout;
      layout->addWidget(label = new QLabel(longDesc(a)));
      layout->addWidget(text = new QTextEdit);
      layout->addWidget(repair = new QCheckBox("Réparer"));
    setLayout(layout);
    text->setReadOnly(true);
    text->setMinimumSize(200, 300);
  }

  const QCheckBox* checkbox (void) const {
    return repair;
  }

  const QIcon& tabIcon (void) const {
    return icon;
  }

  bool empty (void) const {
    return text->toPlainText().isEmpty();
  }

  bool needsRepair (void) const {
    return !empty();
  }

  bool wantsRepairs (void) const {
    return repair->isChecked();
  }

  void insertInto (QTabWidget *w) {
    w->addTab(this, icon, "");
  }

  Logger append (void) {
    return Logger (text);
  }

  void clear (void) {
    text->clear();
    repair->setChecked(false);
  }
};

RepairsManager::RepairsManager(QWidget *parent) : QDialog(parent) {
  QVBoxLayout *mainLayout = new QVBoxLayout;
    QPushButton *go = new QPushButton("Vérifier");
    mainLayout->addWidget(go);

    _resultsDisplayer = new QTabWidget;
    mainLayout->addWidget(_resultsDisplayer);

    _allGood = new QLabel("Aucun problème détecté");
    mainLayout->addWidget(_allGood);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close
                                                   | QDialogButtonBox::Apply);
    _apply = buttons->button(QDialogButtonBox::Apply);
    connect(_apply, &QPushButton::clicked, this, &RepairsManager::correct);
    _apply->setEnabled(false);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);

  setLayout(mainLayout);
  setWindowTitle("Réparateur de livre");

  for (Analysis a: {
    Analysis::COUNT_RECIPE, Analysis::COUNT_INGREDIENT, Analysis::COUNT_UNIT,
    Analysis::HOMONYMOUS_INGREDIENT, Analysis::HOMONYMOUS_UNIT,
    Analysis::UNUSED_INGREDIENT, Analysis::UNUSED_UNIT }) {

    auto s = _summaries[a] = new Summary (a);
    connect(s->checkbox(), &QCheckBox::clicked,
            [this, s] (bool checked){ summaryChecked(s, checked); });
  }

  _results = new CachedAnalysis;

  connect(go, &QPushButton::clicked, this, &RepairsManager::checkAll);

  _resultsDisplayer->setVisible(false);
  _allGood->setVisible(false);
}

RepairsManager::~RepairsManager (void) {
  delete _results;
}

void RepairsManager::postAction(void) {
  bool ok = (_resultsDisplayer->count() == 0);
  _apply->setEnabled(!ok);
  _resultsDisplayer->setVisible(!ok);
  _allGood->setVisible(ok);

  if (!(windowFlags() & Qt::WindowMaximized))  adjustSize();
}

void RepairsManager::summaryChecked(Summary *s, bool checked) {
  int i = _resultsDisplayer->indexOf(s);
  if (checked) {
    _resultsDisplayer->setTabIcon(i, QIcon::fromTheme("dialog-information"));

    // Show next unchecked (if any)
    const int n = _resultsDisplayer->count();
    int seen = 0;
    while (seen < n) {
      auto *s_ = static_cast<Summary*>(_resultsDisplayer->widget(i));
      if (s_->checkbox()->isChecked()) {
        seen++;
        i = (i+1) % n;

      } else {
        _resultsDisplayer->setCurrentIndex(i);
        break;
      }
    }

  } else
    _resultsDisplayer->setTabIcon(i, s->tabIcon());
}

std::pair<const QString&, const QString&> hkey (const db::IngredientData &id) {
  return { id.text, id.group->text };
}

void RepairsManager::checkAll(void) {
  using namespace db;
  Book &book = Book::current();

  _results->reset();
  while (_resultsDisplayer->count()) _resultsDisplayer->removeTab(0);
  for (Summary *s: _summaries) s->clear();

  for (auto &p: book.recipes) _results->rcounts[&p.second] = 0;
  for (auto &p: book.ingredients) {
    _results->icounts[&p.second] = 0;
    _results->ihomonymous[hkey(p.second)][&p.second] = {};
  }
  for (auto &p: book.units) {
    _results->ucounts[&p.second] = 0;
    _results->uhomonymous[p.second.text][&p.second] = {};
  }

  for (auto &p: book.recipes) {
    for (auto &li: p.second.ingredients) {
      if (li->etype == EntryType::Ingredient) {
        IngredientEntry &e = static_cast<IngredientEntry&>(*li);
        _results->icounts[e.idata]++;
        _results->ucounts[e.unit]++;

        auto &ih = _results->ihomonymous[hkey(*e.idata)];
        if (ih.size() > 1)  ih[e.idata].push_back({&p.second, &e});

        auto &uh = _results->uhomonymous[e.unit->text];
        if (uh.size() > 1)  uh[e.unit].push_back({&p.second, &e});

      } else if (li->etype == EntryType::SubRecipe)
        _results->rcounts[static_cast<SubRecipeEntry&>(*li).recipe]++;
    }
  }

  for (const auto &p: _results->rcounts) {
    if (p.first->used != p.second) {
      Summary *s = _summaries.value(Analysis::COUNT_RECIPE);
      if (s->empty()) s->insertInto(_resultsDisplayer);
      s->append() << p.first->title << ":\n"
          << "    " << tr("déclaré") << ": " << p.first->used << "\n"
          << "     " << tr("trouvé") << ": " << p.second << "\n"
          << "  " << tr("variation") << ": "
          << forcesign << (p.first->used - p.second) << "\n";
    }
  }

  for (const auto &p: _results->icounts) {
    if (p.first->used != p.second) {
      Summary *s = _summaries.value(Analysis::COUNT_INGREDIENT);
      if (s->empty()) s->insertInto(_resultsDisplayer);
      s->append() << p.first->text << ":\n"
          << "    " << tr("déclaré") << ": " << p.first->used << "\n"
          << "     " << tr("trouvé") << ": " << p.second << "\n"
          << "  " << tr("variation") << ": "
          << forcesign << (p.first->used - p.second) << "\n";
    }
  }

  for (const auto &p: _results->ucounts) {
    if (p.first->used != p.second) {
      Summary *s = _summaries.value(Analysis::COUNT_UNIT);
      if (s->empty()) s->insertInto(_resultsDisplayer);
      s->append() << p.first->text << ":\n"
                  << "    " << tr("déclaré") << ": " << p.first->used << "\n"
                  << "     " << tr("trouvé") << ": " << p.second << "\n"
                  << "  " << tr("variation") << ": "
                  << forcesign << (p.first->used - p.second) << "\n";
    }
  }

  for (const auto &p: _results->ihomonymous) {
    if (p.second.size() == 1) continue;

    Summary *s = _summaries.value(Analysis::HOMONYMOUS_INGREDIENT);
    if (s->empty()) s->insertInto(_resultsDisplayer);
    auto stream = s->append();
    stream << p.first.first << " (" << p.first.second << "):\n";
    for (const auto &d: p.second) {
      stream << "  " << d.first->id;
      if (d.second.size() > 0) {
        stream << " dans :\n";
        for (const auto &r: d.second)
          stream << "    " << r.first->title << "\n";
      } else
        stream << " (inutilisé)";
      stream << "\n";
    }
  }

  for (const auto &p: _results->uhomonymous) {
    if (p.second.size() == 1) continue;

    Summary *s = _summaries.value(Analysis::HOMONYMOUS_UNIT);
    if (s->empty()) s->insertInto(_resultsDisplayer);
    auto stream = s->append();
    stream << p.first << ":\n";
    for (const auto &d: p.second) {
      stream << "  " << d.first->id;
      if (d.second.size() > 0) {
        stream << " dans :\n";
        for (const auto &r: d.second)
          stream << "    " << r.first->title << "\n";
      } else
        stream << " (inutilisé)";
      stream << "\n";
    }
  }

  for (const auto &p: _results->icounts) {
    if (p.second == 0) {
      Summary *s = _summaries.value(Analysis::UNUSED_INGREDIENT);
      if (s->empty()) s->insertInto(_resultsDisplayer);
      s->append() << p.first->text << " est inutilisé";
    }
  }

  for (const auto &p: _results->ucounts) {
    if (p.second == 0) {
      Summary *s = _summaries.value(Analysis::UNUSED_UNIT);
      if (s->empty()) s->insertInto(_resultsDisplayer);
      s->append() << p.first->text << " est inutilisé";
    }
  }

  postAction();
}

void RepairsManager::correct(void) {
  using F = void (*) (CachedAnalysis*);
  const auto process = [this] (Analysis a, F f) {
    if (Summary *s = _summaries.value(a)) {
      if (s->needsRepair() && s->wantsRepairs()) {
        f(_results);
        _resultsDisplayer->removeTab(_resultsDisplayer->indexOf(s));
//        db::Book::current().modified = true;
      }
    }
  };

  process(Analysis::COUNT_RECIPE, [] (CachedAnalysis *results) {
    for (const auto &p: results->rcounts)
      if (p.first->used != p.second)
        p.first->used = p.second;
  });

  process(Analysis::COUNT_INGREDIENT, [] (CachedAnalysis *results) {
    for (const auto &p: results->icounts)
      if (p.first->used != p.second)
        p.first->used = p.second;
  });

  process(Analysis::COUNT_UNIT, [] (CachedAnalysis *results) {
    for (const auto &p: results->ucounts)
      if (p.first->used != p.second)
        p.first->used = p.second;
  });

  process(Analysis::HOMONYMOUS_INGREDIENT, [] (CachedAnalysis *results) {
    for (auto &p: results->ihomonymous) {
      if (p.second.size() == 1) continue;
      auto &map = p.second;

      // Abritrarily keep the lowest index
      auto iptr = map.begin()->first;
      for (const auto &p: map)  if (p.first->id < iptr->id) iptr = p.first;

      map.erase(iptr);
      for (auto &d: map) {
        for (auto &e: d.second) e.second->idata = iptr;
        iptr->used += d.first->used;
        db::Book::current().ingredients.removeItem(d.first->id);

        // Maybe remove from next correction
        results->icounts.erase(d.first);
      }
    }
  });

  process(Analysis::HOMONYMOUS_UNIT, [] (CachedAnalysis *results) {
    for (auto &p: results->uhomonymous) {
      if (p.second.size() == 1) continue;
      auto &map = p.second;

      // Abritrarily keep the lowest index
      auto uptr = map.begin()->first;
      for (const auto &p: map)  if (p.first->id < uptr->id) uptr = p.first;

      map.erase(uptr);
      for (auto &d: map) {
        for (auto &e: d.second) e.second->unit = uptr;
        uptr->used += d.first->used;
        db::Book::current().units.removeItem(d.first->id);

        // Maybe remove from next correction
        results->ucounts.erase(d.first);
      }
    }
  });

  process(Analysis::UNUSED_INGREDIENT, [] (CachedAnalysis *results) {
    for (const auto &p: results->icounts)
      if (p.second == 0)
        db::Book::current().ingredients.removeItem(p.first->id);
  });

  process(Analysis::UNUSED_UNIT, [] (CachedAnalysis *results) {
    for (const auto &p: results->ucounts)
      if (p.second == 0)
        db::Book::current().units.removeItem(p.first->id);
  });

  postAction();
}

} // end of namespace gui
