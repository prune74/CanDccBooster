#pragma once

#include <stdint.h>

/*
 * CanDccBoosterConfig.h
 *
 * Rôle :
 *   Définir la configuration RailCom HF.
 */

struct RailComConfig
{
    int16_t thresholdStart = 50;
    uint8_t samplesPerBit  = 10;
    uint8_t sampleCenter   = 5;
};
