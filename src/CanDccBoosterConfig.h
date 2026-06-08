#pragma once

#include <stdint.h>

/*
 * CanDccBoosterConfig.h
 *
 * Rôle :
 *   Définir toute la configuration du booster Discovery 2026.
 *
 * Pourquoi un fichier séparé ?
 *   - Pour suivre l’architecture CanUniversal
 *   - Pour éviter les valeurs en dur dans le code
 *   - Pour permettre à l’application (EXSA, autonome…) de modifier
 *     les paramètres RailCom, sécurité, télémétrie, timings…
 */

/* ============================================================
 * Configuration RailCom HF
 * ============================================================
 *
 * Paramètres liés au décodage RailCom :
 *   - seuil de détection du start bit
 *   - nombre d’échantillons par bit
 *   - position d’échantillonnage
 *
 * Ces valeurs dépendent :
 *   - du montage
 *   - du driver (DRV8874, BTS7960…)
 *   - de la fréquence ADC
 */
struct RailComConfig
{
    int16_t thresholdStart = 50;   // seuil start bit (µV → ADC)
    uint8_t samplesPerBit  = 10;   // nb d’échantillons par bit
    uint8_t sampleCenter   = 5;    // position d’échantillonnage
};

/* ============================================================
 * Configuration générale du booster
 * ============================================================
 *
 * Paramètres de sécurité et d’options :
 *   - seuils courant/tension
 *   - activation RailCom
 *   - activation télémétrie
 *   - configuration RailCom HF
 */
struct BoosterConfig
{
    uint16_t maxCurrent_mA = 3000;   // seuil de surintensité
    uint16_t minVoltage_mV = 10000;  // seuil de sous-tension

    bool railcomEnabled = true;      // RailCom obligatoire Discovery 2026
    bool telemetryEnabled = true;    // envoi télémétrie CAN

    RailComConfig railcom;           // configuration RailCom HF
};
