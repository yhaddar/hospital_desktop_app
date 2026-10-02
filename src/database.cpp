#include "database.h"

#include <QCoreApplication>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

bool Database::initialize(QString *error)
{
    const QString path = QDir(QCoreApplication::applicationDirPath()).filePath("hospital_sim.db");
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(path);
    if (!db.open()) {
        if (error) *error = QString("Connexion SQLite impossible : %1").arg(db.lastError().text());
        return false;
    }

    QSqlQuery query;
    const QStringList schema = {
        "PRAGMA foreign_keys = ON",
        "CREATE TABLE IF NOT EXISTS service (id_service INTEGER PRIMARY KEY AUTOINCREMENT, nom TEXT NOT NULL UNIQUE, type TEXT NOT NULL, capacite_lits INTEGER NOT NULL CHECK (capacite_lits >= 0))",
        "CREATE TABLE IF NOT EXISTS patient (id_patient INTEGER PRIMARY KEY AUTOINCREMENT, nom TEXT NOT NULL, prenom TEXT NOT NULL, date_naissance TEXT, sexe TEXT, date_admission TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, date_sortie TEXT)",
        "CREATE TABLE IF NOT EXISTS parcours (id_parcours INTEGER PRIMARY KEY AUTOINCREMENT, id_patient INTEGER NOT NULL REFERENCES patient(id_patient) ON DELETE CASCADE, date_debut TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, date_fin TEXT, statut TEXT NOT NULL DEFAULT 'En cours')",
        "CREATE TABLE IF NOT EXISTS etape_parcours (id_etape INTEGER PRIMARY KEY AUTOINCREMENT, id_parcours INTEGER NOT NULL REFERENCES parcours(id_parcours) ON DELETE CASCADE, id_service INTEGER REFERENCES service(id_service), type_etape TEXT NOT NULL, date_debut TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, date_fin TEXT, ordre INTEGER NOT NULL, observation TEXT NOT NULL DEFAULT 'Aucune observation saisie')",
        "CREATE TABLE IF NOT EXISTS evolution_sante (id_evolution INTEGER PRIMARY KEY AUTOINCREMENT, id_patient INTEGER NOT NULL REFERENCES patient(id_patient) ON DELETE CASCADE, etat TEXT NOT NULL, date_heure TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, observation TEXT NOT NULL DEFAULT 'Aucune observation saisie')",
        "CREATE TABLE IF NOT EXISTS personnel (id_personnel INTEGER PRIMARY KEY AUTOINCREMENT, nom TEXT NOT NULL, prenom TEXT NOT NULL, specialite TEXT NOT NULL, disponibilite INTEGER NOT NULL DEFAULT 1 CHECK (disponibilite IN (0,1)), id_service INTEGER REFERENCES service(id_service))",
        "CREATE TABLE IF NOT EXISTS affectation_personnel (id_affectation INTEGER PRIMARY KEY AUTOINCREMENT, id_personnel INTEGER NOT NULL REFERENCES personnel(id_personnel), id_service INTEGER NOT NULL REFERENCES service(id_service), date_debut TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP, date_fin TEXT, fonction TEXT NOT NULL)",
        "CREATE TABLE IF NOT EXISTS lit (id_lit INTEGER PRIMARY KEY AUTOINCREMENT, numero TEXT NOT NULL, statut TEXT NOT NULL DEFAULT 'Libre', id_service INTEGER NOT NULL REFERENCES service(id_service), UNIQUE(numero,id_service))",
        "CREATE TABLE IF NOT EXISTS equipement (id_equipement INTEGER PRIMARY KEY AUTOINCREMENT, id_service INTEGER NOT NULL REFERENCES service(id_service), nom TEXT NOT NULL, type TEXT NOT NULL, etat TEXT NOT NULL DEFAULT 'Opérationnel', date_derniere_maintenance TEXT)"
    };
    for (const QString &sql : schema) {
        if (!query.exec(sql)) {
            if (error) *error = QString("Erreur SQLite : %1\n\nInstruction échouée :\n%2").arg(query.lastError().text(), sql);
            return false;
        }
    }

    const QStringList views = {
        "CREATE VIEW IF NOT EXISTS ref_patient AS SELECT id_patient, nom||' '||prenom||' (#'||id_patient||')' AS libelle FROM patient",
        "CREATE VIEW IF NOT EXISTS ref_service AS SELECT id_service, nom||' — '||type AS libelle FROM service",
        "CREATE VIEW IF NOT EXISTS ref_personnel AS SELECT id_personnel, nom||' '||prenom||' — '||specialite||' (#'||id_personnel||')' AS libelle FROM personnel",
        "CREATE VIEW IF NOT EXISTS ref_parcours AS SELECT r.id_parcours, p.nom||' '||p.prenom||' — Parcours #'||r.id_parcours||' ('||r.statut||')' AS libelle FROM parcours r JOIN patient p ON p.id_patient=r.id_patient"
    };
    for (const QString &sql : views) {
        if (!query.exec(sql)) {
            if (error) *error = QString("Erreur SQLite : %1\n\nInstruction échouée :\n%2").arg(query.lastError().text(), sql);
            return false;
        }
    }

    const QStringList observationDefaults = {
        "UPDATE etape_parcours SET observation='Aucune observation saisie' WHERE observation IS NULL OR TRIM(observation)=''",
        "UPDATE evolution_sante SET observation='Aucune observation saisie' WHERE observation IS NULL OR TRIM(observation)=''"
    };
    for (const QString &sql : observationDefaults) {
        if (!query.exec(sql)) {
            if (error) *error = QString("Erreur SQLite : %1\n\nInstruction échouée :\n%2").arg(query.lastError().text(), sql);
            return false;
        }
    }
    return true;
}
