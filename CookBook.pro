#-------------------------------------------------
#
# Project created by QtCreator 2020-09-29T17:26:23
#
#-------------------------------------------------

QT += core gui

QT += androidextras

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CookBook
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += BASE_DIR=\\\"$$PWD\\\" BASE_BUILD_DIR=\\\"$$OUT_PWD\\\"

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#CONFIG += object_parallel_to_source
CONFIG += c++11

SOURCES += \
    src/db/book.cpp \
    src/db/ingredientlistentries.cpp \
    src/db/ingredientsmodel.cpp \
    src/db/recipe.cpp \
    src/db/recipedata.cpp \
    src/db/recipesmodel.cpp \
    src/db/unitsmodel.cpp \
    src/gui/common.cpp \
    src/gui/gui_book.cpp \
    src/gui/gui_recipe.cpp \
    src/gui/ingrediententrydialog.cpp \
    src/gui/ingredientsmanager.cpp \
    src/gui/listcontrols.cpp \
    src/gui/repairsmanager.cpp \
    src/gui/updatemanager.cpp \
    src/main.cpp \
    src/gui/settings.cpp \
    src/gui/filterview.cpp

HEADERS += \
    src/db/basemodel.h \
    src/db/book.h \
    src/db/ingredientlistentries.h \
    src/db/ingredientsmodel.h \
    src/db/recipe.h \
    src/db/recipedata.h \
    src/db/recipesmodel.h \
    src/db/unitsmodel.h \
    src/gui/androidspecifics.hpp \
    src/gui/autofiltercombobox.hpp \
    src/gui/common.h \
    src/gui/gui_book.h \
    src/gui/gui_recipe.h \
    src/gui/ingrediententrydialog.h \
    src/gui/ingredientsmanager.h \
    src/gui/listcontrols.h \
    src/gui/repairsmanager.h \
    src/gui/updatemanager.h \
    src/gui/settings.h \
    src/gui/filterview.h

RESOURCES += \
    resources.qrc

DISTFILES += \
    android/AndroidManifest.xml \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradlew \
    android/res/values/libs.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew.bat

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
