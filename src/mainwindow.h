#pragma once

#include <QMainWindow>
#include <QVector>

class QTableView;
class QLabel;
class QSqlRelationalTableModel;
class QTabWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
private slots:
    void refresh();
    void addPatient();
    void simulateStep();
    void addRow();
    void editCell();
    void deleteRow();
    void saveChanges();
private:
    void loadTables();
    void installStatusCombos();
    QTabWidget *tabs;
    QLabel *kpiLabel;
    QVector<QTableView *> views;
    QVector<QSqlRelationalTableModel *> models;
    QStringList tableNames;
};
