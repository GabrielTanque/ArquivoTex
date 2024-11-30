#ifndef CSMP3011_H
#define CSMP3011_H

#pragma once

#include <cstdint>
#include "cifI2C.h"

class cSMP3011
{
private:
    float temperature;
    float pressure;
    float pressure_bar;
    float pressure_psi;
    cifI2C *ptI2C;

public:
    cSMP3011();
    ~cSMP3011();
    void init(cifI2C& I2CDriver);
    float getTemperature();
    float getPressure();
    float getPressureBar();
    float getPressurePsi();
    void poll();
};

#endif