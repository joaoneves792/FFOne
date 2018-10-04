//
// Created by joao on 9/28/18.
//

#ifndef B_RF2EFFECT_H
#define B_RF2EFFECT_H

#define DIRECTINPUT_VERSION 0x0800
#include "dinput.h"

class rf2Effect {
#define DEFAULT_PERIOD 35127000
#define ENGINE_MIN_MAG 200
#define ENGINE_MAX_MAG 2500
#define FL_ANGLE 13500
#define FR_ANGLE 22500
#define RL_ANGLE  8000
#define RR_ANGLE 28000
#define L_ANGLE   9000
#define R_ANGLE  27000
#define F_ANGLE  18000
#define B_ANGLE      0
#define WHEEL_MAGNITUDE 500
#define W_FL 0x1
#define W_FR 0x2
#define W_RL 0x4
#define W_RR 0x8
protected:
    DWORD rgdwAxes[2] = { DIJOFS_X, DIJOFS_Y };
    LONG rglDirection[2] = {B_ANGLE, 0};
    DIEFFECT eff;
    DIPERIODIC peff;
    LPDIRECTINPUTEFFECT self;
    DWORD engineMagnitude = 0;
    UCHAR slidingWheels = 0x0;
    double slideFactor = 0.0f;
    double enginePercent = 0.0f;
private:
    void loadDefaults();
    void recomputeDirection();
public:
    rf2Effect(LPDIRECTINPUTDEVICE8 dev);
    ~rf2Effect();
    void setRPM(double rpm, double max);
    void slideWheels(UCHAR wheels, double factor);
    int play();
    int stop();
};
#endif //B_RF2EFFECT_H
