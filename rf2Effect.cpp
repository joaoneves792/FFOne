//
// Created by joao on 9/28/18.
//
#include <windows.h>
#include "rf2Effect.h"
#define DIRECTINPUT_VERSION 0x0800
#include "dinput.h"

int rf2Effect::play() {
    static DWORD lastTime = 0;
    DWORD curTime = GetTickCount();
    if((curTime-lastTime) > 25) {
        lastTime = curTime;

        recomputeDirection();

#define ENGINE_MAGNITUDE ( ((enginePercent*ENGINE_MAX_MAG)<ENGINE_MIN_MAG)?ENGINE_MIN_MAG:(enginePercent*ENGINE_MAX_MAG))

        peff.dwMagnitude = ENGINE_MAGNITUDE + slideFactor*WHEEL_MAGNITUDE;
        FAILED(self->SetParameters(&eff, DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_START | DIEP_DURATION));
    }
    return 0;
}

int rf2Effect::stop() {
    return FAILED(self->Stop());
}

void rf2Effect::loadDefaults() {
    eff.dwSize = sizeof( DIEFFECT );
    eff.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE;
    eff.dwSamplePeriod = 0;
    eff.dwGain = DI_FFNOMINALMAX;
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval = 0;
    eff.cAxes = 2;
    eff.rgdwAxes = rgdwAxes;
    eff.rglDirection = rglDirection;
    eff.lpEnvelope = 0;
    eff.cbTypeSpecificParams = sizeof( DIPERIODIC );
    eff.lpvTypeSpecificParams = &peff;
    eff.dwStartDelay = 0;

    peff.dwMagnitude = 0;
    peff.dwPeriod = DEFAULT_PERIOD;
    peff.dwPhase = 0;
    peff.lOffset = 0;
}
rf2Effect::rf2Effect(LPDIRECTINPUTDEVICE8 dev) {
    loadDefaults();
    peff.dwMagnitude = ENGINE_MIN_MAG;
    rglDirection[0] = 0;
    FAILED(dev->CreateEffect( GUID_Sine, &eff, &self, nullptr ) );
}

rf2Effect::~rf2Effect() {
    if(self){
        (self)->Release();
        (self)=nullptr;
    }
}

void rf2Effect::setRPM(double rpm, double max) {
    enginePercent = rpm/max;
}

void rf2Effect::recomputeDirection() {
    //count how many wheels are sliding
    int count = 0;
    if(slidingWheels & W_FL) count++;
    if(slidingWheels & W_FR) count++;
    if(slidingWheels & W_RL) count++;
    if(slidingWheels & W_RR) count++;

    //If we have 3 or more wheels sliding set to front (to activate triggers)
    if(count > 2) {
        rglDirection[0] = F_ANGLE;
        return;
    }
    if(count == 1){
        rglDirection[0] = (slidingWheels & W_FL)?L_ANGLE:rglDirection[0];
        rglDirection[0] = (slidingWheels & W_FR)?R_ANGLE:rglDirection[0];
        rglDirection[0] = (slidingWheels & W_RL)?RL_ANGLE:rglDirection[0];
        rglDirection[0] = (slidingWheels & W_RR)?RR_ANGLE:rglDirection[0];
        return;
    }
    if(count == 2){
        //both front
        rglDirection[0] = (slidingWheels == (W_FL | W_FR))?F_ANGLE:rglDirection[0];
        //both back
        rglDirection[0] = (slidingWheels == (W_RL | W_RR))?B_ANGLE:rglDirection[0];
        //both left
        rglDirection[0] = (slidingWheels == (W_FL | W_RL))?L_ANGLE:rglDirection[0];
        //both right
        rglDirection[0] = (slidingWheels == (W_FR | W_RR))?R_ANGLE:rglDirection[0];
        //cross
        rglDirection[0] = (slidingWheels == (W_FR | W_RL))?F_ANGLE:rglDirection[0];
        rglDirection[0] = (slidingWheels == (W_FL | W_RR))?F_ANGLE:rglDirection[0];
        return;
    }
    //If none than set to regular
    rglDirection[0] = B_ANGLE;
}

void rf2Effect::slideWheels(UCHAR wheels, double factor) {
    slidingWheels = wheels;
    slideFactor = factor;
}
