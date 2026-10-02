#include "simulationservice.h"

#include <QSqlError>
#include <QSqlQuery>

bool SimulationService::admitPatient(const QString &nom, const QString &prenom, QString *error)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) { if (error) *error = db.lastError().text(); return false; }
    QSqlQuery q;
    qlonglong patientId = 0;
    qlonglong parcoursId = 0;
    q.prepare("INSERT INTO patient(nom,prenom) VALUES(?,?)");
    q.addBindValue(nom); q.addBindValue(prenom);
    if (!q.exec()) goto fail;
    patientId = q.lastInsertId().toLongLong();
    q.prepare("INSERT INTO parcours(id_patient) VALUES(?)"); q.addBindValue(patientId);
    if (!q.exec()) goto fail;
    parcoursId = q.lastInsertId().toLongLong();
    q.prepare("INSERT INTO etape_parcours(id_parcours,id_service,type_etape,ordre,observation) SELECT ?,id_service,'Admission',1,'Patient admis dans le scénario' FROM service WHERE nom='Urgences'");
    q.addBindValue(parcoursId);
    if (!q.exec() || q.numRowsAffected() == 0) goto fail;
    q.prepare("INSERT INTO evolution_sante(id_patient,etat,observation) VALUES(?,'Stable','État initial de simulation')");
    q.addBindValue(patientId);
    if (!q.exec() || !db.commit()) goto fail;
    return true;
fail:
    if (error) *error = q.lastError().text();
    db.rollback();
    return false;
}

bool SimulationService::advancePatients(QString *error)
{
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.transaction()) { if (error) *error = db.lastError().text(); return false; }
    QSqlQuery q;
    // Chaque clic simule une transition: Admission -> Diagnostic -> Traitement -> Sortie.
    if (!q.exec("UPDATE etape_parcours e JOIN parcours p ON p.id_parcours=e.id_parcours SET e.date_fin=CURRENT_TIMESTAMP WHERE p.statut='En cours' AND e.date_fin IS NULL AND e.type_etape IN ('Admission','Diagnostic','Traitement')")) goto fail;
    if (!q.exec("INSERT INTO etape_parcours(id_parcours,id_service,type_etape,ordre,observation) SELECT d.id_parcours,s.id_service,CASE d.ordre WHEN 1 THEN 'Diagnostic' WHEN 2 THEN 'Traitement' ELSE 'Sortie' END,d.ordre+1,'Transition simulée' FROM (SELECT id_parcours,MAX(ordre) AS ordre FROM etape_parcours GROUP BY id_parcours) d JOIN parcours p ON p.id_parcours=d.id_parcours LEFT JOIN service s ON s.nom=CASE d.ordre WHEN 1 THEN 'Radiologie' WHEN 2 THEN 'Chirurgie' ELSE 'Urgences' END WHERE p.statut='En cours' AND d.ordre<4")) goto fail;
    if (!q.exec("UPDATE parcours SET statut='Terminé',date_fin=CURRENT_TIMESTAMP WHERE id_parcours IN (SELECT id_parcours FROM etape_parcours GROUP BY id_parcours HAVING MAX(ordre)>=4)")) goto fail;
    if (!q.exec("UPDATE patient p JOIN parcours r ON r.id_patient=p.id_patient SET p.date_sortie=CURRENT_TIMESTAMP WHERE r.statut='Terminé'")) goto fail;
    if (!q.exec("INSERT INTO evolution_sante(id_patient,etat,observation) SELECT id_patient,'Amélioration','Évolution générée par le pas de simulation' FROM patient WHERE date_sortie IS NULL")) goto fail;
    if (!db.commit()) goto fail;
    return true;
fail:
    if (error) *error = q.lastError().text();
    db.rollback();
    return false;
}
