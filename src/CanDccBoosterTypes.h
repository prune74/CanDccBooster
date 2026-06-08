/*
 * CanDccBoosterTypes.h
 *
 * Rôle :
 *   Définir tous les types communs utilisés par la bibliothèque CanDccBooster :
 *     - états du booster
 *     - erreurs
 *     - configuration
 *     - télémétrie
 *     - constantes
 *
 * Pourquoi un fichier séparé ?
 *   → Pour éviter les dépendances circulaires.
 *   → Pour rendre le code plus clair.
 *   → Pour que tous les modules partagent les mêmes types.
 */
#pragma once

#include <stdint.h>
#include "CanDccBoosterConfig.h"

/* ============================================================
 * États possibles du booster
 * ============================================================
 *
 * Le booster peut être dans plusieurs états :
 *   - OFF : sortie coupée (commande ou sécurité)
 *   - ON  : sortie active
 *   - FAULT : défaut matériel (surchauffe, surintensité…)
 *   - CUTOUT : fenêtre RailCom active
 *
 * Le Core gère ces états automatiquement.
 */
enum class BoosterState : uint8_t {
    OFF = 0,
    ON,
    CUTOUT,
    FAULT
};

/* ============================================================
 * Types d’erreurs possibles
 * ============================================================
 *
 * Ces erreurs sont renvoyées dans la télémétrie CAN Service Booster.
 * Elles permettent au système maître (EXSA, LaBox…) de réagir.
 */
enum class BoosterError : uint8_t {
    NONE = 0,
    OVERCURRENT,     // surintensité
    OVERTEMP,        // surchauffe
    UNDERVOLTAGE,    // tension trop basse
    HARDWARE_FAULT,  // défaut driver
    RAILCOM_ERROR    // erreur RailCom (rare)
};

/* ============================================================
 * Structure de télémétrie
 * ============================================================
 *
 * Cette structure contient toutes les informations que le booster
 * renverra sur le CAN Service Booster :
 *   - courant
 *   - tension
 *   - état
 *   - erreurs
 *   - adresse RailCom (si détectée)
 *
 * Le Core met à jour cette structure à chaque cycle.
 */
struct BoosterTelemetry {
    uint16_t current_mA = 0;     // courant voie
    uint16_t voltage_mV = 0;     // tension voie
    BoosterState state = BoosterState::OFF;
    BoosterError error = BoosterError::NONE;
    uint16_t railcomAddress = 0; // 0 = aucune adresse détectée
};

/* ============================================================
 * Configuration du booster
 * ============================================================
 *
 * Cette structure contient les paramètres configurables :
 *   - seuils de sécurité
 *   - activation RailCom
 *   - activation télémétrie
 *
 * Elle peut être modifiée par l’application (EXSA, autonome…)
 */
struct BoosterConfig {
    uint16_t maxCurrent_mA = 3000;   // seuil de surintensité
    uint16_t minVoltage_mV = 10000;  // seuil de sous-tension
    bool railcomEnabled = true;      // RailCom obligatoire dans Discovery 2026
    bool telemetryEnabled = true;    // envoi télémétrie CAN
    RailComConfig railcom;
};

/* ============================================================
 * Constantes générales
 * ============================================================
 *
 * Ces constantes sont utilisées dans toute la bibliothèque.
 */
namespace BoosterConstants {
    constexpr uint16_t RAILCOM_NO_ADDRESS = 0; // aucune adresse détectée
    constexpr uint8_t DCC_MAX_LEN = 8;         // taille max d’un paquet DCC logique
}