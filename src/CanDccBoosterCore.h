#pragma once

#include <stdint.h>
#include "CanDccBoosterTypes.h"
#include "CanDccBoosterConfig.h"
#include "CanDccBoosterHardware.h"
#include "CanDccBoosterCAN.h"

/*
 * CanDccBoosterCore.h
 *
 * Rôle :
 *   Cœur du booster Discovery 2026.
 *
 *   Il gère :
 *     - la réception DCC logique (via CanDccBoosterCAN)
 *     - l’application du DCC physique (via CanDccBoosterHardware)
 *     - le cutout RailCom
 *     - la sécurité (courant, tension, fautes)
 *     - la télémétrie
 *
 *   Le Core NE connaît PAS :
 *     - les GPIO
 *     - les ADC
 *     - le driver moteur
 *     - la plateforme (ESP32, STM32…)
 *
 *   Il ne manipule que :
 *     - l’interface hardware
 *     - la configuration
 *     - les types communs
 *     - les données CAN
 */

class CanDccBoosterCore
{
public:

    /*
     * Constructeur
     *
     * hw  = abstraction matérielle
     * can = interface CAN (DCC logique + cutout)
     * cfg = configuration complète du booster
     */
    CanDccBoosterCore(CanDccBoosterHardware &hw,
                      CanDccBoosterCAN &can,
                      const BoosterConfig &cfg)
        : hardware(hw),
          canInterface(can),
          config(cfg)
    {}

    /* ------------------------------------------------------------
     * update()
     * ------------------------------------------------------------
     * Fonction principale du booster.
     * Appelée régulièrement (ex : toutes les 100–200 µs).
     *
     * Elle gère :
     *   - lecture CAN
     *   - application DCC
     *   - gestion cutout
     *   - sécurité
     *   - télémétrie
     */
    void update()
    {
        handleDccFromCan();
        handleCutoutFromCan();
        handleSafety();
        updateTelemetry();
    }

    /* ------------------------------------------------------------
     * getTelemetry()
     * ------------------------------------------------------------
     * Retourne la télémétrie actuelle du booster.
     */
    const BoosterTelemetry &getTelemetry() const {
        return telemetry;
    }

    /* ------------------------------------------------------------
     * getConfig() / setConfig()
     * ------------------------------------------------------------
     * Accès à la configuration du booster.
     */
    const BoosterConfig &getConfig() const { return config; }

    void setConfig(const BoosterConfig &cfg)
    {
        config = cfg;
    }

private:

    /* Références vers les modules externes */
    CanDccBoosterHardware &hardware;
    CanDccBoosterCAN &canInterface;

    /* État interne */
    BoosterTelemetry telemetry;
    BoosterConfig config;

    bool cutoutActive = false;

    /* ============================================================
     * Gestion du DCC logique → DCC physique
     * ============================================================ */
    void handleDccFromCan()
    {
        uint8_t bit, phase;

        if (canInterface.getDccBit(bit, phase)) {

            // Si le booster est en défaut → on ignore le DCC
            if (telemetry.state == BoosterState::FAULT)
                return;

            uint8_t data[2] = { bit, phase };
            hardware.applyDcc(data, 2);

            if (!cutoutActive)
                telemetry.state = BoosterState::ON;
        }
    }

    /* ============================================================
     * Gestion du cutout RailCom
     * ============================================================ */
    void handleCutoutFromCan()
    {
        bool newCutout;

        if (canInterface.getCutout(newCutout)) {

            cutoutActive = newCutout;

            if (cutoutActive) {
                hardware.enableCutout();
                telemetry.state = BoosterState::CUTOUT;
            } else {
                hardware.disableCutout();
                telemetry.state = BoosterState::ON;
            }
        }
    }

    /* ============================================================
     * Gestion de la sécurité
     * ============================================================ */
    void handleSafety()
    {
        // Défaut matériel
        if (hardware.isFaultActive()) {
            telemetry.error = BoosterError::HARDWARE_FAULT;
            hardware.disableOutput();
            telemetry.state = BoosterState::FAULT;
            return;
        }

        // Surintensité
        uint16_t current = hardware.readCurrent_mA();
        if (current > config.maxCurrent_mA) {
            telemetry.error = BoosterError::OVERCURRENT;
            hardware.disableOutput();
            telemetry.state = BoosterState::FAULT;
            return;
        }

        // Sous-tension
        uint16_t voltage = hardware.readVoltage_mV();
        if (voltage < config.minVoltage_mV) {
            telemetry.error = BoosterError::UNDERVOLTAGE;
            hardware.disableOutput();
            telemetry.state = BoosterState::FAULT;
            return;
        }

        // Aucun problème
        telemetry.error = BoosterError::NONE;
    }

    /* ============================================================
     * Mise à jour de la télémétrie
     * ============================================================ */
    void updateTelemetry()
    {
        if (!config.telemetryEnabled)
            return;

        telemetry.current_mA = hardware.readCurrent_mA();
        telemetry.voltage_mV = hardware.readVoltage_mV();
    }
};
