#-------------------------------------------------
#
# Project created by QtCreator 2020-09-29T17:26:23
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CookBook
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += BASE_DIR=\\\"$$system(pwd)\\\"

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#CONFIG += object_parallel_to_source

SOURCES += \
    src/db/ingredientdata.cpp \
    src/db/recipe.cpp \
    src/db/book.cpp \
    src/db/editablestringlistmodel.cpp \
    src/db/ingredientsmodel.cpp \
    src/db/ingredientlistentries.cpp \
    src/gui/ingrediententrydialog.cpp \
    src/gui/gui_recipe.cpp \
    src/gui/gui_book.cpp \
    src/gui/ingredientsmanager.cpp \
    src/main.cpp \
    src/gui/updatemanager.cpp \
    src/gui/common.cpp \
    src/db/unitsmodel.cpp \
    src/db/recipesmodel.cpp \
    src/gui/listcontrols.cpp

HEADERS += \
    src/db/ingredientdata.h \
    src/db/recipe.h \
    src/db/book.h \
    src/db/editablestringlistmodel.h \
    src/db/ingredientsmodel.h \
    src/db/ingredientlistentries.h \
    src/gui/gui_book.h \
    src/gui/gui_recipe.h \
    src/gui/autofiltercombobox.hpp \
    src/gui/ingrediententrydialog.h \
    src/gui/ingredientsmanager.h \
    src/gui/updatemanager.h \
    src/gui/common.h \
    src/db/unitsmodel.h \
    src/db/recipesmodel.h \
    src/db/basemodel.h \
    src/gui/listcontrols.h
