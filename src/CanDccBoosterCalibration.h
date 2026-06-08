#pragma once

#include <stdint.h>
#include "CanDccBoosterTypes.h"
#include "CanDccBoosterConfig.h"
#include "CanDccBoosterHardware.h"

/*
 * CanDccBoosterCalibration.h
 *
 * Calibration automatique Discovery 2026.
 *
 * Objectifs :
 *   - mesurer le courant à vide (IPROPI)
 *   - mesurer la tension réelle
 *   - déterminer les seuils de sécurité
 *   - produire un BoosterConfig calibré
 *
 * La calibration est entièrement configurable :
 *   - nombre d’échantillons
 *   - marge de sécurité courant
 *   - marge de sécurité tension
 */

class CanDccBoosterCalibration
{
public:
    struct CalibrationConfig
    {
        uint16_t sampleCount;
        uint16_t currentMargin_mA;
        float voltageFactor;

        CalibrationConfig()
            : sampleCount(200),
              currentMargin_mA(2000),
              voltageFactor(0.70f)
        {
        }
    };

    enum class State : uint8_t
    {
        IDLE = 0,
        MEASURE_CURRENT,
        MEASURE_VOLTAGE,
        DONE
    };

    /*
     * Constructeur
     *
     * hw  = abstraction matérielle
     * cfg = configuration initiale du booster
     * cal = configuration de la calibration
     */
    CanDccBoosterCalibration(CanDccBoosterHardware &hw,
                             const BoosterConfig &initialCfg,
                             const CalibrationConfig &calCfg = CalibrationConfig())
        : hardware(hw),
          baseConfig(initialCfg),
          calConfig(calCfg)
    {
    }

    /* ------------------------------------------------------------
     * start()
     * ------------------------------------------------------------
     * Lance la calibration.
     */
    void start()
    {
        state = State::MEASURE_CURRENT;

        samplesCurrent = 0;
        samplesVoltage = 0;
        sumCurrent = 0;
        sumVoltage = 0;

        calibratedConfig = baseConfig; // on part de la config existante
    }

    /* ------------------------------------------------------------
     * update()
     * ------------------------------------------------------------
     * Fonction appelée régulièrement pendant la calibration.
     */
    void update()
    {
        switch (state)
        {

        case State::MEASURE_CURRENT:
            collectCurrent();
            break;

        case State::MEASURE_VOLTAGE:
            collectVoltage();
            break;

        case State::DONE:
        case State::IDLE:
        default:
            break;
        }
    }

    /* ------------------------------------------------------------
     * isFinished()
     * ------------------------------------------------------------
     */
    bool isFinished() const
    {
        return state == State::DONE;
    }

    /* ------------------------------------------------------------
     * getCalibratedConfig()
     * ------------------------------------------------------------
     */
    const BoosterConfig &getCalibratedConfig() const
    {
        return calibratedConfig;
    }

private:
    CanDccBoosterHardware &hardware;

    State state = State::IDLE;

    /* Configurations */
    BoosterConfig baseConfig;
    BoosterConfig calibratedConfig;
    CalibrationConfig calConfig;

    /* Accumulateurs */
    uint32_t sumCurrent = 0;
    uint32_t sumVoltage = 0;
    uint16_t samplesCurrent = 0;
    uint16_t samplesVoltage = 0;

    /* ------------------------------------------------------------
     * collectCurrent()
     * ------------------------------------------------------------
     * Mesure le courant à vide pour déterminer le bruit IPROPI.
     */
    void collectCurrent()
    {
        uint16_t current = hardware.readCurrent_mA();
        sumCurrent += current;
        samplesCurrent++;

        if (samplesCurrent >= calConfig.sampleCount)
        {

            uint16_t avg = sumCurrent / samplesCurrent;

            calibratedConfig.maxCurrent_mA =
                avg + calConfig.currentMargin_mA;

            state = State::MEASURE_VOLTAGE;
        }
    }

    /* ------------------------------------------------------------
     * collectVoltage()
     * ------------------------------------------------------------
     * Mesure la tension réelle pour déterminer le seuil de sous-tension.
     */
    void collectVoltage()
    {
        uint16_t voltage = hardware.readVoltage_mV();
        sumVoltage += voltage;
        samplesVoltage++;

        if (samplesVoltage >= calConfig.sampleCount)
        {

            uint16_t avg = sumVoltage / samplesVoltage;

            calibratedConfig.minVoltage_mV =
                static_cast<uint16_t>(avg * calConfig.voltageFactor);

            state = State::DONE;
        }
    }
};
