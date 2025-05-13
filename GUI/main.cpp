#include "myWindows.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainController controller;

    return app.exec();
}
