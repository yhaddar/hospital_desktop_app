#include "mainwindow.h"
#include "database.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("HospitalSim");

    QString error;
    if (!Database::initialize(&error)) {
        QMessageBox::critical(nullptr, "Erreur base de données", error);
        return 1;
    }

    MainWindow window;
    window.show();
    return app.exec();
}
