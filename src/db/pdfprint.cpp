#include <algorithm>
#include <fstream>
#include <filesystem>

#include "book.h"

#include <QDebug>

#ifndef Q_OS_ANDROID

namespace db {
//   double amount;
//   UnitData *unit;
//   IngredientData *idata;
//   QString qualif;

void printRecipe(std::ofstream &ofs, const Recipe &r) {
  bool hasNotes = !r.notes.isEmpty();

  ofs << "\\section{" << r.title.toStdString() << "}\n";
  ofs << "\\label{recipe:" << r.title.toStdString() << "}\n\n";

  if (hasNotes)
    ofs << "\\begin{minipage}[t]{.49\\textwidth}\n";

  ofs << "\\subsection*{Pour " << r.portions << " " << r.portionsLabel.toStdString() << "}\n\n";
  ofs << " \\begin{itemize}\n";
  for (const auto &e: r.ingredients) {
    if (e->etype == EntryType::Ingredient) {
      const auto &i = static_cast<const IngredientEntry&>(*e);
      ofs << "  \\item " << i.amount;
      if (i.unit->text != "Ø")
        ofs << " " << i.unit->text.toStdString();
      ofs << " " << i.idata->text.toStdString();
      if (!i.qualif.isEmpty())
        ofs << " (" << i.qualif.toStdString() << ")";

    } else if (e->etype == EntryType::SubRecipe) {
      const auto &_r = static_cast<const SubRecipeEntry&>(*e);
      const auto title = _r.recipe->title.toStdString();
      ofs << "  \\item \\hyperlink{recipe:" << title << "}{" << title << "}\n";

    } else if (e->etype == EntryType::Decoration) {
      const auto &d = static_cast<const DecorationEntry&>(*e);
      ofs << "\\end{itemize}\n\\paragraph{" << d.text.toStdString() << "}\n"
          << R"(\begin{itemize})" << "\n";

    } else {
      ofs << "\\item type: " << int(e->etype) << "(ignored)";
    }
    ofs << "\n";
  }
  ofs << R"( \end{itemize})" << "\n";

  if (hasNotes) {
    ofs << "\\end{minipage}\n";
    ofs << "\\begin{minipage}[t]{.49\\textwidth}\n";
    ofs << "\\subsection*{Notes:}\n";
    ofs << r.notes.toStdString() << "\n";
    ofs << "\\end{minipage}\n";
  }

  ofs << "\\subsection*{Etapes}\n";
  ofs << "\\begin{enumerate}\n";
  for (const auto &step: r.steps)
    ofs << " \\item " << step.toStdString() << "\n";
  ofs << "\\end{enumerate}\n";
}

bool Book::print(void) {
  qDebug() << "Printing";

//   QPrinter printer(QPrinter::PrinterResolution);
//   printer.setOutputFormat(QPrinter::PdfFormat);
//   printer.setPageSize(QPageSize(QPageSize::A4));
//
//   QString filename(monitoredPath());
//   filename.replace(extension(), "pdf");
//   printer.setOutputFileName(filename);
//
//   QTextDocument doc;
//
//   QString html;
//   QTextStream stream (&html);
//
//   stream << "<h1>Hello, Malenda!</h1>\n";
//   stream << "<p>Lorem ipsum dolor sit amet, consectitur adipisci elit.</p>";
//
// //   doc.setPageSize(printer.pageRect().size()); // This is necessary if you want to hide the page number
//
//   for (const auto &[id, recipe]: recipes)
//       stream << "<h2>" << recipe.title << "</h2>\n";
//
//   doc.setHtml(html);
//   doc.print(&printer);

  // Sort recipes alphabetically
  std::vector<const Recipe*> sortedRecipes;
  sortedRecipes.reserve(recipes.rowCount());
  for (const auto &[id, recipe]: recipes)
    sortedRecipes.push_back(&recipe);
  std::sort(sortedRecipes.begin(), sortedRecipes.end(),
            [] (auto *lhs, auto *rhs) {
              return lhs->title < rhs->title;
            });

  QString workFolder = monitoredDir() + "/_latex";
  std::filesystem::create_directories(workFolder.toStdString());

  QString outfile = (workFolder + "/" + monitoredName().replace(extension(), "tex"));
  qDebug() << "Writing to" << outfile;
  std::ofstream ofs (outfile.toStdString());
  ofs << R"(\documentclass{article})" << "\n";
  ofs << R"(\usepackage{hyperref})" << "\n";
  ofs << R"(\begin{document})" << "\n";
  ofs << R"(\title{Malenda's CookBook})" << "\n";
  ofs << R"(\author{With love})" << "\n";
  ofs << R"(\maketitle)" << "\n";
  ofs << R"(\addtocontents{toc}{\protect\hypertarget{toc}{}})" << "\n";
  ofs << R"(\tableofcontents)" << "\n";
  ofs << R"(\newpage)" << "\n\n";

  for (const auto *recipe_ptr: sortedRecipes) {
    const auto &r = *recipe_ptr;
    printRecipe(ofs, r);
    ofs << R"(\vfill\small\hyperlink{toc}{Table of Contents})" << "\n\n";
    ofs << R"(\newpage)" << "\n\n";
  }

  ofs << R"(\end{document})" << "\n";
  ofs.close();

  auto cmd = (QString("pdflatex --interaction=nonstopmode --output-directory ")
              + workFolder + " " + outfile).toStdString();
  for (auto i=0; i<2; i++) {
    auto r = system(cmd.c_str());
    (void)r;
  }

  QString outpdf = outfile.chopped(3) + "pdf";
  QString pdf (monitoredPath());
  pdf.replace(extension(), "pdf");
  std::filesystem::rename(outpdf.toStdString(), pdf.toStdString());

  return true;
}

} // end of namespace db

#endif
