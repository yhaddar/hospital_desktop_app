#pragma once

#include <QString>

class Database
{
public:
    // Se connecte au serveur MySQL et crée le schéma si nécessaire.
    static bool initialize(QString *error = nullptr);
};
