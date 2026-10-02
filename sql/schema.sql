-- MySQL 8+ : run this once before starting HospitalSim.
CREATE DATABASE IF NOT EXISTS hospital_sim
  CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE hospital_sim;

CREATE TABLE IF NOT EXISTS service (
  id_service INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  nom VARCHAR(150) NOT NULL UNIQUE,
  type VARCHAR(100) NOT NULL,
  capacite_lits INT NOT NULL CHECK (capacite_lits >= 0)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS patient (
  id_patient INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  nom VARCHAR(150) NOT NULL,
  prenom VARCHAR(150) NOT NULL,
  date_naissance DATE NULL,
  sexe VARCHAR(30) NULL,
  date_admission DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  date_sortie DATETIME NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS parcours (
  id_parcours INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  id_patient INT UNSIGNED NOT NULL,
  date_debut DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  date_fin DATETIME NULL,
  statut VARCHAR(40) NOT NULL DEFAULT 'En cours',
  CONSTRAINT fk_parcours_patient FOREIGN KEY (id_patient) REFERENCES patient(id_patient) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS etape_parcours (
  id_etape INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  id_parcours INT UNSIGNED NOT NULL,
  id_service INT UNSIGNED NULL,
  type_etape VARCHAR(80) NOT NULL,
  date_debut DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  date_fin DATETIME NULL,
  ordre INT NOT NULL,
  observation VARCHAR(2000) NOT NULL DEFAULT 'Aucune observation saisie',
  CONSTRAINT fk_etape_parcours FOREIGN KEY (id_parcours) REFERENCES parcours(id_parcours) ON DELETE CASCADE,
  CONSTRAINT fk_etape_service FOREIGN KEY (id_service) REFERENCES service(id_service)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS evolution_sante (
  id_evolution INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  id_patient INT UNSIGNED NOT NULL,
  etat VARCHAR(50) NOT NULL,
  date_heure DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  observation VARCHAR(2000) NOT NULL DEFAULT 'Aucune observation saisie',
  CONSTRAINT fk_evolution_patient FOREIGN KEY (id_patient) REFERENCES patient(id_patient) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS personnel (
  id_personnel INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  nom VARCHAR(150) NOT NULL,
  prenom VARCHAR(150) NOT NULL,
  specialite VARCHAR(150) NOT NULL,
  disponibilite TINYINT NOT NULL DEFAULT 1 CHECK (disponibilite IN (0,1)),
  id_service INT UNSIGNED NULL,
  CONSTRAINT fk_personnel_service FOREIGN KEY (id_service) REFERENCES service(id_service)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS affectation_personnel (
  id_affectation INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  id_personnel INT UNSIGNED NOT NULL,
  id_service INT UNSIGNED NOT NULL,
  date_debut DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
  date_fin DATETIME NULL,
  fonction VARCHAR(150) NOT NULL,
  CONSTRAINT fk_affectation_personnel FOREIGN KEY (id_personnel) REFERENCES personnel(id_personnel),
  CONSTRAINT fk_affectation_service FOREIGN KEY (id_service) REFERENCES service(id_service)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS lit (
  id_lit INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  numero VARCHAR(50) NOT NULL,
  statut VARCHAR(50) NOT NULL DEFAULT 'Libre',
  id_service INT UNSIGNED NOT NULL,
  UNIQUE KEY uq_lit_service (numero,id_service),
  CONSTRAINT fk_lit_service FOREIGN KEY (id_service) REFERENCES service(id_service)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE TABLE IF NOT EXISTS equipement (
  id_equipement INT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
  id_service INT UNSIGNED NOT NULL,
  nom VARCHAR(150) NOT NULL,
  type VARCHAR(100) NOT NULL,
  etat VARCHAR(60) NOT NULL DEFAULT 'Opérationnel',
  date_derniere_maintenance DATE NULL,
  CONSTRAINT fk_equipement_service FOREIGN KEY (id_service) REFERENCES service(id_service)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

CREATE OR REPLACE VIEW ref_patient AS
  SELECT id_patient, CONCAT(nom,' ',prenom,' (#',id_patient,')') AS libelle FROM patient;
CREATE OR REPLACE VIEW ref_service AS
  SELECT id_service, CONCAT(nom,' — ',type) AS libelle FROM service;
CREATE OR REPLACE VIEW ref_personnel AS
  SELECT id_personnel, CONCAT(nom,' ',prenom,' — ',specialite,' (#',id_personnel,')') AS libelle FROM personnel;
CREATE OR REPLACE VIEW ref_parcours AS
  SELECT r.id_parcours, CONCAT(p.nom,' ',p.prenom,' — Parcours #',r.id_parcours,' (',r.statut,')') AS libelle
  FROM parcours r JOIN patient p ON p.id_patient=r.id_patient;
