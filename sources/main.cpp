#include "mainwindow.h"
#include "accueil.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Accueil splash;
    MainWindow w;
    QObject::connect(&splash, &Accueil::demarrerClique, [&]() {
        splash.close();
        w.show();
    });
    splash.show();
    return a.exec();
}
