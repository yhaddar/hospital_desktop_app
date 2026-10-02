#include "mainwindow.h"
#include "simulationservice.h"

#include <QDateTime>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QSqlRelationalTableModel>
#include <QSqlRelation>
#include <QSqlRelationalDelegate>
#include <QPersistentModelIndex>
#include <QTabWidget>
#include <QTableView>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QLabel>
#include <QStatusBar>
#include <QAbstractItemView>
#include <QComboBox>
#include <QApplication>
#include <QPainter>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QDateEdit>
#include <QDate>

class StatusCellDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &) const override
    {
        QStyleOptionViewItem background(option);
        background.text.clear();
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &background, painter);
    }
};

class DateEditDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        auto *editor = new QDateEdit(parent);
        editor->setCalendarPopup(true);
        editor->setDisplayFormat("yyyy-MM-dd");
        editor->setDate(QDate::currentDate());
        return editor;
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        auto *dateEditor = qobject_cast<QDateEdit *>(editor);
        if (!dateEditor) return;
        const QDate date = QDate::fromString(index.data(Qt::EditRole).toString(), Qt::ISODate);
        if (date.isValid()) dateEditor->setDate(date);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        auto *dateEditor = qobject_cast<QDateEdit *>(editor);
        if (dateEditor) model->setData(index, dateEditor->date().toString(Qt::ISODate), Qt::EditRole);
    }
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("Hospital Project");
    resize(1180, 740);
    auto *toolbar = addToolBar("Actions");
    toolbar->addAction("Ajouter une ligne", this, &MainWindow::addRow);
    toolbar->addAction("Supprimer la ligne", this, &MainWindow::deleteRow);
    toolbar->addAction("Enregistrer", this, &MainWindow::saveChanges);
    toolbar->addAction("Actualiser", this, &MainWindow::refresh);

    auto *root = new QWidget;
    auto *layout = new QVBoxLayout(root);
    kpiLabel = new QLabel;
    layout->addWidget(kpiLabel);
    tabs = new QTabWidget;
    layout->addWidget(tabs);
    setCentralWidget(root);

    tableNames = {"patient", "parcours", "etape_parcours", "evolution_sante", "service",
                  "personnel", "affectation_personnel", "lit", "equipement"};
    loadTables();
    refresh();
}

void MainWindow::loadTables()
{
    const QStringList titles = {"Patients", "Parcours", "Étapes du parcours", "Évolution santé",
                                "Services", "Personnel", "Affectations personnel", "Lits", "Équipements"};
    for (int i = 0; i < tableNames.size(); ++i) {
        auto *view = new QTableView;
        auto *model = new QSqlRelationalTableModel(view);
        model->setTable(tableNames.at(i));
        // Keep rows visible even when an optional foreign key (for example
        // etape_parcours.id_service) has not been selected yet.
        model->setJoinMode(QSqlRelationalTableModel::LeftJoin);
        auto relate = [model](const QString &field, const QString &lookupView, const QString &displayField) {
            const int column = model->fieldIndex(field);
            if (column >= 0) model->setRelation(column, QSqlRelation(lookupView, field, displayField));
        };
        if (tableNames.at(i) == "parcours") relate("id_patient", "ref_patient", "libelle");
        else if (tableNames.at(i) == "etape_parcours") {
            relate("id_parcours", "ref_parcours", "libelle");
            relate("id_service", "ref_service", "libelle");
            const int parcoursColumn = model->fieldIndex("id_parcours");
            const int serviceColumn = model->fieldIndex("id_service");
            if (parcoursColumn >= 0) model->setHeaderData(parcoursColumn, Qt::Horizontal, "Parcours");
            if (serviceColumn >= 0) model->setHeaderData(serviceColumn, Qt::Horizontal, "Service");
        } else if (tableNames.at(i) == "evolution_sante") relate("id_patient", "ref_patient", "libelle");
        else if (tableNames.at(i) == "personnel") relate("id_service", "ref_service", "libelle");
        else if (tableNames.at(i) == "affectation_personnel") {
            relate("id_personnel", "ref_personnel", "libelle");
            relate("id_service", "ref_service", "libelle");
        } else if (tableNames.at(i) == "lit" || tableNames.at(i) == "equipement") {
            relate("id_service", "ref_service", "libelle");
        }
        model->setEditStrategy(QSqlTableModel::OnManualSubmit);
        model->select();
        view->setModel(model);
        view->setItemDelegate(new QSqlRelationalDelegate(view));
        if (tableNames.at(i) == "equipement") {
            const int maintenanceColumn = model->fieldIndex("date_derniere_maintenance");
            if (maintenanceColumn >= 0)
                view->setItemDelegateForColumn(maintenanceColumn, new DateEditDelegate(view));
        }
        const QString choiceField = tableNames.at(i) == "parcours" ? "statut" :
                                    tableNames.at(i) == "evolution_sante" ? "etat" : QString();
        if (!choiceField.isEmpty()) {
            const int choiceColumn = model->fieldIndex(choiceField);
            if (choiceColumn >= 0) view->setItemDelegateForColumn(choiceColumn, new StatusCellDelegate(view));
        }
        if (model->record().fieldName(0).startsWith("id_")) view->setColumnHidden(0, true);
        view->setAlternatingRowColors(true);
        view->setSortingEnabled(true);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->horizontalHeader()->setStretchLastSection(true);
        view->resizeColumnsToContents();
        tabs->addTab(view, titles.at(i));
        views.append(view);
        models.append(model);
    }
    installStatusCombos();
}

void MainWindow::installStatusCombos()
{
    const QStringList tables = {"parcours", "evolution_sante"};
    const QStringList fields = {"statut", "etat"};
    const QStringList defaults = {"En cours", "Stable"};
    const QList<QStringList> choices = {
        {"En cours", "Terminé", "Annulé"},
        {"Stable", "Aggravation", "Critique", "Amélioration"}
    };

    for (int target = 0; target < tables.size(); ++target) {
        const int tableIndex = tableNames.indexOf(tables.at(target));
        if (tableIndex < 0) continue;
        QSqlRelationalTableModel *model = models.at(tableIndex);
        QTableView *view = views.at(tableIndex);
        const int choiceColumn = model->fieldIndex(fields.at(target));
        if (choiceColumn < 0) continue;
        for (int row = 0; row < model->rowCount(); ++row) {
            const QModelIndex index = model->index(row, choiceColumn);
            auto *combo = new QComboBox(view);
            combo->addItems(choices.at(target));
            combo->setAutoFillBackground(true);
            combo->setStyleSheet("QComboBox { background-color: palette(base); }");
            const QString current = index.data(Qt::EditRole).toString();
            int currentIndex = combo->findText(current);
            if (currentIndex < 0) {
                currentIndex = combo->findText(defaults.at(target));
                model->setData(index, defaults.at(target), Qt::EditRole);
            }
            combo->setCurrentIndex(currentIndex);
            const QPersistentModelIndex persistentIndex(index);
            connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this,
                    [model, persistentIndex, combo](int) {
                model->setData(persistentIndex, combo->currentText(), Qt::EditRole);
            });
            view->setIndexWidget(index, combo);
        }
    }
}

void MainWindow::refresh()
{
    for (int i = 0; i < models.size(); ++i) {
        models.at(i)->select();
        views.at(i)->resizeColumnsToContents();
        for (int column = 0; column < models.at(i)->columnCount(); ++column) {
            if (QSqlTableModel *lookup = models.at(i)->relationModel(column))
                lookup->select();
        }
    }

    installStatusCombos();
    QSqlQuery kpi;
    kpi.exec("SELECT (SELECT COUNT(*) FROM patient WHERE date_sortie IS NULL), (SELECT COUNT(*) FROM patient), (SELECT COUNT(*) FROM parcours WHERE statut='Terminé')");
    if (kpi.next()) kpiLabel->setText(QString("Patients en cours : %1    |    Patients enregistrés : %2    |    Parcours terminés : %3    |    Actualisé : %4")
        .arg(kpi.value(0).toInt()).arg(kpi.value(1).toInt()).arg(kpi.value(2).toInt()).arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
}

void MainWindow::addRow()
{
    const int tab = tabs->currentIndex();
    if (tab < 0 || tab >= models.size()) return;
    QSqlRelationalTableModel *model = models.at(tab);
    QString observation;
    const bool hasObservation = tableNames.at(tab) == "etape_parcours" || tableNames.at(tab) == "evolution_sante";
    if (hasObservation) {
        bool ok = false;
        // observation = QInputDialog::getText(this, "Observation", "Saisis l'observation :", &ok);
        // if (!ok) return;
        if (observation.trimmed().isEmpty()) observation = "Aucune observation saisie";
    }
    const int row = model->rowCount();
    if (!model->insertRow(row)) {
        QMessageBox::critical(this, "Ajout impossible", model->lastError().text());
        return;
    }
    if (hasObservation) {
        const int observationColumn = model->fieldIndex("observation");
        if (observationColumn >= 0 && !model->setData(model->index(row, observationColumn), observation.trimmed(), Qt::EditRole)) {
            model->removeRow(row);
            QMessageBox::critical(this, "Observation non enregistrée",
                                  "Impossible d'ajouter l'observation à la ligne.");
            return;
        }
    }
    views.at(tab)->scrollToBottom();
    views.at(tab)->selectRow(row);
    installStatusCombos();
    const QString firstField = model->record().fieldName(0);
    const int idColumn = model->fieldIndex(firstField);
    if (idColumn >= 0 && firstField.startsWith("id_")) views.at(tab)->setColumnHidden(idColumn, true);
    statusBar()->showMessage("Complète les cellules de la nouvelle ligne, puis clique sur Enregistrer.", 6000);
}

void MainWindow::deleteRow()
{
    const int tab = tabs->currentIndex();
    if (tab < 0 || tab >= models.size()) return;
    const QModelIndex index = views.at(tab)->currentIndex();
    if (!index.isValid()) {
        QMessageBox::information(this, "Sélection requise", "Sélectionne d'abord une ligne à supprimer.");
        return;
    }
    const auto answer = QMessageBox::question(this, "Confirmer la suppression",
        "Supprimer la ligne sélectionnée ? Les dépendances configurées en cascade peuvent aussi être supprimées.");
    if (answer != QMessageBox::Yes) return;
    QSqlRelationalTableModel *model = models.at(tab);
    if (!model->removeRow(index.row()) || !model->submitAll()) {
        QMessageBox::critical(this, "Suppression impossible", model->lastError().text());
        model->revertAll();
        return;
    }
    refresh();
}

void MainWindow::saveChanges()
{
    const int tab = tabs->currentIndex();
    if (tab < 0 || tab >= models.size()) return;
    QSqlRelationalTableModel *model = models.at(tab);
    if (!model->submitAll()) {
        QMessageBox::critical(this, "Enregistrement impossible",
            model->lastError().text() + "\nVérifie les champs obligatoires et les ID des clés étrangères.");
        return;
    }
    const QString table = tableNames.at(tab);
    if (table == "etape_parcours" || table == "evolution_sante") {
        const int observationColumn = model->fieldIndex("observation");
        const int idColumn = model->fieldIndex(table == "etape_parcours" ? "id_etape" : "id_evolution");
        QSqlQuery persistObservation;
        persistObservation.prepare(QString("UPDATE %1 SET observation=? WHERE %2=?")
            .arg(table, table == "etape_parcours" ? "id_etape" : "id_evolution"));
        for (int row = 0; row < model->rowCount(); ++row) {
            const QVariant id = model->data(model->index(row, idColumn), Qt::EditRole);
            if (!id.isValid() || id.isNull()) continue;
            QString observation = model->data(model->index(row, observationColumn), Qt::EditRole).toString().trimmed();
            if (observation.isEmpty()) observation = "Aucune observation saisie";
            persistObservation.bindValue(0, observation);
            persistObservation.bindValue(1, id);
            if (!persistObservation.exec()) {
                QMessageBox::critical(this, "Observation non enregistrée", persistObservation.lastError().text());
                return;
            }
        }
    }
    model->select();
    refresh();
    statusBar()->showMessage("Modifications enregistrées.", 3000);
}

void MainWindow::addPatient()
{
    bool ok = false;
    const QString nom = QInputDialog::getText(this, "Admission", "Nom du patient :", QLineEdit::Normal, {}, &ok);
    if (!ok || nom.trimmed().isEmpty()) return;
    const QString prenom = QInputDialog::getText(this, "Admission", "Prénom :", QLineEdit::Normal, {}, &ok);
    if (!ok || prenom.trimmed().isEmpty()) return;
    QString error;
    if (!SimulationService::admitPatient(nom.trimmed(), prenom.trimmed(), &error)) QMessageBox::critical(this, "Admission impossible", error);
    refresh();
    tabs->setCurrentIndex(0);
}

void MainWindow::simulateStep()
{
    QString error;
    if (!SimulationService::advancePatients(&error)) QMessageBox::critical(this, "Simulation impossible", error);
    else statusBar()->showMessage("Un pas de simulation a été appliqué.", 3000);
    refresh();
}

void MainWindow::editCell()
{
    const int tab = tabs->currentIndex();
    if (tab < 0 || tab >= views.size()) return;
    const QModelIndex index = views.at(tab)->currentIndex();
    if (!index.isValid()) {
        QMessageBox::information(this, "Sélection requise", "Sélectionne d'abord la cellule à modifier.");
        return;
    }
    if (!(index.flags() & Qt::ItemIsEditable)) {
        QMessageBox::information(this, "Cellule non modifiable", "Cette cellule ne peut pas être modifiée.");
        return;
    }
    views.at(tab)->edit(index);
}