/*
 * CanDccBoosterCAN.h
 *
 * Rôle :
 *   Ce module gère la réception des trames CAN destinées au booster.
 *
 *   Dans Discovery 2026, le booster reçoit deux types de messages :
 *
 *     1) ID 0x100 → bit DCC + phase
 *        - data[0] = bit DCC (0 ou 1)
 *        - data[1] = phase (0 ou 1)
 *
 *     2) ID 0x101 → cutout
 *        - data[0] = 1 → activer cutout
 *        - data[0] = 0 → désactiver cutout
 *
 *   IMPORTANT :
 *     - Le RailCom NE vient PAS du CAN.
 *     - Le RailCom est lu sur les rails via l’ADC HF.
 *
 *   Ce module ne fait que stocker les informations reçues.
 *   Le Core les consommera ensuite pour piloter le hardware.
 */

#pragma once

#include <stdint.h>
#include "CanDccBoosterTypes.h"

/*
 * Avant-propos pédagogique :
 *
 * Ce module ne dépend PAS de CanUniversal directement.
 * Pourquoi ?
 *   → Pour garder la bibliothèque totalement portable.
 *
 * L’application (EXSA, autonome…) doit appeler :
 *
 *     canInterface.onCanMessage(id, data, len);
 *
 * à chaque réception CAN.
 *
 * Ce module se contente de :
 *   - décoder les IDs
 *   - stocker les valeurs
 *   - fournir un accès simple au Core
 */
class CanDccBoosterCAN
{
public:
    CanDccBoosterCAN() = default;

    /* ------------------------------------------------------------
     * onCanMessage()
     * ------------------------------------------------------------
     * Fonction appelée par l’application à chaque trame CAN reçue.
     *
     * id   = identifiant CAN (11 bits)
     * data = tableau d’octets
     * len  = taille du tableau
     *
     * Cette fonction NE fait AUCUN traitement lourd.
     * Elle se contente de stocker les informations.
     */
    void onCanMessage(uint16_t id, const uint8_t *data, uint8_t len)
    {
        if (id == DCC_BIT_ID && len >= 2) {
            // Trame DCC logique
            lastDccBit = data[0];
            lastDccPhase = data[1];
            dccBitAvailable = true;
        }
        else if (id == CUTOUT_ID && len >= 1) {
            // Trame cutout
            cutoutActive = (data[0] != 0);
            cutoutAvailable = true;
        }
    }

    /* ------------------------------------------------------------
     * getDccBit()
     * ------------------------------------------------------------
     * Retourne le dernier bit DCC reçu.
     * Le Core consomme cette valeur à chaque cycle.
     */
    bool getDccBit(uint8_t &bit, uint8_t &phase)
    {
        if (!dccBitAvailable)
            return false;

        bit = lastDccBit;
        phase = lastDccPhase;
        dccBitAvailable = false; // consommé
        return true;
    }

    /* ------------------------------------------------------------
     * getCutout()
     * ------------------------------------------------------------
     * Retourne l’état du cutout.
     * Le Core l’utilise pour activer/désactiver la fenêtre RailCom.
     */
    bool getCutout(bool &active)
    {
        if (!cutoutAvailable)
            return false;

        active = cutoutActive;
        cutoutAvailable = false; // consommé
        return true;
    }

private:
    /* IDs CAN utilisés par le booster */
    static constexpr uint16_t DCC_BIT_ID = 0x100;
    static constexpr uint16_t CUTOUT_ID  = 0x101;

    /* Dernier bit DCC reçu */
    uint8_t lastDccBit = 0;
    uint8_t lastDccPhase = 0;
    bool dccBitAvailable = false;

    /* Dernier état cutout reçu */
    bool cutoutActive = false;
    bool cutoutAvailable = false;
};
