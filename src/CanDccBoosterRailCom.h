#pragma once

#include <stdint.h>
#include "CanDccBoosterHardware.h"
#include "CanDccBoosterConfig.h"
#include "CanDccBoosterTypes.h"

/*
 * CanDccBoosterRailCom.h
 *
 * Rôle :
 *   Décode le RailCom HF pendant la fenêtre de cutout.
 *
 *   Compatible Locoduino :
 *     - CH1 = adresse courte (1–127)
 *     - CH2 = extension adresse longue
 *       adresse_longue = (CH2 << 7) | CH1
 *
 * Fonctionnement :
 *   Le Core appelle :
 *     - onCutoutStart() → début du cutout
 *     - feedSample()    → chaque échantillon ADC HF
 *     - onCutoutEnd()   → fin du cutout
 *
 *   Le module RailCom :
 *     - détecte le start bit
 *     - lit CH1 (8 bits)
 *     - lit CH2 (8 bits)
 *     - valide la trame
 *     - reconstruit l’adresse courte ou longue
 *
 *   Le hardware doit fournir :
 *     - readRailcomSample() → échantillon HF brut
 */

class CanDccBoosterRailCom
{
public:

    /*
     * Constructeur
     *
     * hw  = abstraction matérielle (ADC HF)
     * cfg = configuration RailCom (seuils, timing…)
     */
    CanDccBoosterRailCom(CanDccBoosterHardware &hw,
                         const RailComConfig &cfg)
        : hardware(hw),
          config(cfg)
    {}

    /* ------------------------------------------------------------
     * onCutoutStart()
     * ------------------------------------------------------------
     * Réinitialise l’état interne du décodeur.
     */
    void onCutoutStart()
    {
        sampleIndex      = 0;
        startBitDetected = false;
        valid            = false;

        ch1              = 0;
        ch2              = 0;
        bitIndex         = 0;
        channel          = 1;
    }

    /* ------------------------------------------------------------
     * feedSample()
     * ------------------------------------------------------------
     * Appelé à chaque échantillon HF pendant le cutout.
     *
     * Le Core doit appeler cette fonction :
     *   - dans une ISR timer
     *   - ou dans une boucle très rapide
     */
    void feedSample()
    {
        int16_t s = hardware.readRailcomSample();

        // Détection du start bit (front descendant)
        if (!startBitDetected) {
            if (s < -config.thresholdStart) {
                startBitDetected = true;
                bitIndex = 0;
                channel  = 1;
            }
            sampleIndex++;
            return;
        }

        // Échantillonnage au centre du bit
        if (sampleIndex % config.samplesPerBit == config.sampleCenter) {

            bool bit = (s > 0);

            if (channel == 1) {
                ch1 |= (bit << bitIndex);
            }
            else if (channel == 2) {
                ch2 |= (bit << bitIndex);
            }

            bitIndex++;

            // Passage au canal suivant
            if (bitIndex >= 8) {
                bitIndex = 0;
                channel++;

                if (channel > 2)
                    valid = true;
            }
        }

        sampleIndex++;
    }

    /* ------------------------------------------------------------
     * onCutoutEnd()
     * ------------------------------------------------------------
     * Retourne l’adresse courte ou longue détectée.
     *
     * 0 = aucune adresse valide.
     */
    uint16_t onCutoutEnd()
    {
        if (!startBitDetected || !valid)
            return BoosterConstants::RAILCOM_NO_ADDRESS;

        // CH1 invalide
        if (ch1 == 0 || ch1 == 0xFF)
            return BoosterConstants::RAILCOM_NO_ADDRESS;

        // Adresse courte
        if (ch2 == 0)
            return ch1;

        // Adresse longue Locoduino
        uint16_t addr = (static_cast<uint16_t>(ch2) << 7) | ch1;

        // Filtrage simple
        if (addr < 128 || addr > 10239)
            return BoosterConstants::RAILCOM_NO_ADDRESS;

        return addr;
    }

private:
    CanDccBoosterHardware &hardware;
    RailComConfig config;

    /* État interne */
    bool     startBitDetected = false;
    bool     valid            = false;
    uint16_t sampleIndex      = 0;

    uint8_t  ch1              = 0;
    uint8_t  ch2              = 0;
    uint8_t  bitIndex         = 0;
    uint8_t  channel          = 1;
};
