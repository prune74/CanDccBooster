/*
 * CanDccBoosterHardware.h
 *
 * Rôle :
 *   Interface abstraite du matériel Booster.
 *   Cette interface définit TOUT ce que la bibliothèque CanDccBooster
 *   doit pouvoir demander au hardware.
 *
 *   Le but est de rendre la bibliothèque totalement portable :
 *     - ESP32, STM32, RP2040, AVR…
 *     - DRV8874, BTS7960, pont en H discret…
 *     - ADC HF interne ou externe
 *
 *   Le matériel réel (EXSA, Booster autonome, etc.)
 *   fournira une classe dérivée qui implémente ces fonctions.
 *
 * IMPORTANT :
 *   Le RailCom HF est OBLIGATOIRE dans Discovery 2026.
 *   → Le hardware doit fournir un échantillon HF brut.
 *   → Le cutout doit être parfaitement maîtrisé.
 */

#pragma once

#include <stdint.h>

/*
 * Classe abstraite représentant le matériel du booster.
 * Toutes les fonctions sont virtuelles pures (= interface).
 *
 * La bibliothèque ne connaît PAS :
 *   - les GPIO
 *   - les timers
 *   - les ADC
 *   - le driver moteur
 *   - la plateforme (ESP32, STM32…)
 *
 * Elle ne fait qu'appeler ces fonctions.
 */
class CanDccBoosterHardware
{
public:
    virtual ~CanDccBoosterHardware() = default;

    /* ------------------------------------------------------------
     * applyDcc()
     * ------------------------------------------------------------
     * Applique physiquement un bit DCC sur la voie.
     *
     * data[0] = polarité du bit (0 ou 1)
     * len     = longueur du buffer (toujours >= 1)
     *
     * Le hardware doit :
     *   - mettre à jour la polarité (PH)
     *   - activer la puissance (EN)
     *
     * Cette fonction est appelée à CHAQUE bit DCC reçu via CAN.
     */
    virtual void applyDcc(const uint8_t *data, uint8_t len) = 0;

    /* ------------------------------------------------------------
     * enableCutout() / disableCutout()
     * ------------------------------------------------------------
     * Active ou désactive la fenêtre RailCom.
     *
     * Le cutout coupe la puissance (EN=0) mais NE change PAS la polarité.
     *
     * EXIGENCE :
     *   - Ces fonctions doivent être extrêmement rapides.
     *   - Elles peuvent être appelées dans une ISR.
     *   - La précision du cutout conditionne la qualité du RailCom HF.
     */
    virtual void enableCutout() = 0;
    virtual void disableCutout() = 0;

    /* ------------------------------------------------------------
     * enableOutput() / disableOutput()
     * ------------------------------------------------------------
     * Active ou coupe complètement la sortie voie.
     *
     * Utilisé pour :
     *   - sécurité (court-circuit, surchauffe)
     *   - commande ON/OFF (F5)
     *   - défaut matériel
     */
    virtual void enableOutput() = 0;
    virtual void disableOutput() = 0;

    /* ------------------------------------------------------------
     * isFaultActive()
     * ------------------------------------------------------------
     * Retourne true si le driver matériel signale un défaut :
     *   - surchauffe
     *   - surintensité
     *   - erreur interne
     *
     * La bibliothèque utilise cette info pour couper la voie.
     */
    virtual bool isFaultActive() = 0;

    /* ------------------------------------------------------------
     * readCurrent_mA()
     * ------------------------------------------------------------
     * Retourne le courant voie en milliampères.
     *
     * Le hardware peut utiliser :
     *   - ADC + IPROPI (DRV8874)
     *   - shunt + amplificateur
     *   - INA219 / INA226
     *   - n’importe quelle méthode
     */
    virtual uint16_t readCurrent_mA() = 0;

    /* ------------------------------------------------------------
     * readVoltage_mV()
     * ------------------------------------------------------------
     * Retourne la tension voie en millivolts.
     *
     * Utilisé pour :
     *   - télémétrie
     *   - sécurité (sous-tension)
     */
    virtual uint16_t readVoltage_mV() = 0;

    /* ------------------------------------------------------------
     * readRailcomSample()
     * ------------------------------------------------------------
     * Retourne un échantillon brut RailCom HF.
     *
     * EXIGENCE IMPORTANTE :
     *   - Le RailCom est OBLIGATOIRE dans Discovery 2026.
     *   - Cette fonction DOIT retourner un échantillon HF réel.
     *   - Elle peut être appelée dans une ISR.
     *   - Elle doit être extrêmement rapide (aucun calcul lourd).
     *
     * Le module RailCom de la bibliothèque se charge :
     *   - du filtrage
     *   - de la détection du start bit
     *   - du décodage CH1 / CH2
     *   - de la validation
     */
    virtual int16_t readRailcomSample() = 0;
};
