#pragma once

#include <QString>

class SimulationService
{
public:
    // Crée un patient, son parcours, la première étape et son état initial.
    static bool admitPatient(const QString &nom, const QString &prenom, QString *error = nullptr);
    // Avance les parcours actifs d'une étape; un patient peut sortir à la fin.
    static bool advancePatients(QString *error = nullptr);
};
