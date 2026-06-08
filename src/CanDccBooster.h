#pragma once

#include "CanDccBoosterTypes.h"
#include "CanDccBoosterConfig.h"
#include "CanDccBoosterHardware.h"
#include "CanDccBoosterCAN.h"
#include "CanDccBoosterCore.h"
#include "CanDccBoosterCalibration.h"
#include "CanDccBoosterRailCom.h"

/*
 * CanDccBooster.h
 *
 * Façade de la bibliothèque :
 *   - regroupe tous les modules
 *   - injecte la configuration
 *   - expose une API simple à l’application
 *
 * L’application doit :
 *   - créer une implémentation de CanDccBoosterHardware (DRV8874, etc.)
 *   - créer un BoosterConfig
 *   - créer un CanDccBooster(hw, cfg)
 *   - appeler update() régulièrement
 *   - appeler onCutoutStart()/feedRailcomSample()/onCutoutEnd() pendant le cutout
 */

class CanDccBooster
{
public:

    /*
     * Constructeur principal
     *
     * hw  = implémentation matérielle (DRV8874, BTS7960…)
     * cfg = configuration complète du booster (RailCom, sécurité, télémétrie…)
     */
    CanDccBooster(CanDccBoosterHardware &hw, const BoosterConfig &cfg)
        : hardware(hw),
          config(cfg),
          canInterface(),
          core(hardware, canInterface, config),
          calibration(hardware, config),
          railcom(hardware, config.railcom)
    {}

    /* ------------------------------------------------------------
     * update()
     * ------------------------------------------------------------
     * Boucle principale du booster.
     * Appelée régulièrement par l’application.
     */
    void update()
    {
        // Si calibration en cours → priorité
        if (calibrationRunning) {
            calibration.update();

            if (calibration.isFinished()) {
                // On récupère la config calibrée
                config = calibration.getCalibratedConfig();
                core.setConfig(config);

                calibrationRunning = false;
                calibrationDone = true;
            }
        }

        // Logique principale du booster
        core.update();
    }

    /* ------------------------------------------------------------
     * Réception CAN
     * ------------------------------------------------------------
     * Appelée par l’application à chaque trame CAN reçue.
     */
    void onCanMessage(uint16_t id, const uint8_t *data, uint8_t len)
    {
        canInterface.onCanMessage(id, data, len);
    }

    /* ------------------------------------------------------------
     * Calibration
     * ------------------------------------------------------------
     * L’application peut lancer une calibration automatique.
     */
    void startCalibration()
    {
        calibration.start();
        calibrationRunning = true;
        calibrationDone = false;
    }

    bool isCalibrationFinished() const
    {
        return calibrationDone;
    }

    const BoosterConfig &getCalibratedConfig() const
    {
        return calibration.getCalibratedConfig();
    }

    /* ------------------------------------------------------------
     * RailCom HF
     * ------------------------------------------------------------
     * Hooks appelés par l’application pendant le cutout.
     */
    void onCutoutStart()
    {
        if (config.railcomEnabled)
            railcom.onCutoutStart();
    }

    void feedRailcomSample()
    {
        if (config.railcomEnabled)
            railcom.feedSample();
    }

    uint16_t onCutoutEnd()
    {
        if (!config.railcomEnabled)
            return BoosterConstants::RAILCOM_NO_ADDRESS;

        uint16_t addr = railcom.onCutoutEnd();

        if (addr != BoosterConstants::RAILCOM_NO_ADDRESS) {
            // On met l’adresse dans la télémétrie
            BoosterTelemetry &t = const_cast<BoosterTelemetry&>(core.getTelemetry());
            t.railcomAddress = addr;
        }

        return addr;
    }

    /* ------------------------------------------------------------
     * Accès télémétrie / configuration
     * ------------------------------------------------------------
     */
    const BoosterTelemetry &getTelemetry() const
    {
        return core.getTelemetry();
    }

    const BoosterConfig &getConfig() const
    {
        return config;
    }

    void setConfig(const BoosterConfig &cfg)
    {
        config = cfg;
        core.setConfig(cfg);
    }

private:
    BoosterConfig config;

    CanDccBoosterHardware &hardware;
    CanDccBoosterCAN       canInterface;
    CanDccBoosterCore      core;
    CanDccBoosterCalibration calibration;
    CanDccBoosterRailCom   railcom;

    bool calibrationRunning = false;
    bool calibrationDone    = false;
};
