//
// Created by joao on 9/30/18.
//

#include "PluginObjects.h"
#include "InternalsPlugin.h"
#include <cstdio>                  // for logging
#include <cmath>
#define DIRECTINPUT_VERSION 0x0800
#include "dinput.h"
#include "rf2Effect.h"

//#include <windows.h>
// plugin information

#define SLIDE_VELOCITY_THRESHOLD 1.2f //meters/second
#define MAX_SLIDE_FORCE_AT 7.0f //meters/second
#define WHEEL_LOCK_BELOW_PERCENTAGE 0.6f //percentage

#define SAFE_DELETE(p)  { if(p) { delete (p);	 (p)=nullptr; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }
FILE*					g_logFile = nullptr;
char*					g_logPath = nullptr;
bool					g_realtime = false;
LPDIRECTINPUT8			g_pDI = nullptr;
LPDIRECTINPUTDEVICE8	g_pDevice = nullptr;
rf2Effect* 				g_pEffect = nullptr;
const char*             g_errorMessage = nullptr;


//-----------------------------------------------------------------------------
// Name: EnumFFDevicesCallback()
// Desc: Called once for each enumerated force feedback device. If we find
//	   one, create a device interface on it so we can play with it.
// Taken from DX SDK
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumFFDevicesCallback( const DIDEVICEINSTANCE* pInst,
                                     VOID* pContext )
{
    LPDIRECTINPUTDEVICE8 pDevice;
    HRESULT hr;

    // Obtain an interface to the enumerated force feedback device.
    hr = g_pDI->CreateDevice( pInst->guidInstance, &pDevice, nullptr );

    // If it failed, then we can't use this device for some
    // bizarre reason.  (Maybe the user unplugged it while we
    // were in the middle of enumerating it.)  So continue enumerating
    if( FAILED( hr ) )
        return DIENUM_CONTINUE;

    // We successfully created an IDirectInputDevice8.  So stop looking
    // for another one.
    g_pDevice = pDevice;

    return DIENUM_STOP;

}


extern "C" __declspec( dllexport )
void __cdecl SetEnvironment(void* info) {
	EnvironmentInfoV01 *env = (EnvironmentInfoV01 *) info;
	size_t pathLen = strlen(env->mPath[0]) + 1;
	if (!g_logPath) {
		g_logPath = new char[pathLen];
		strcpy_s(g_logPath, pathLen, env->mPath[0]);
	}
	if (!g_logFile && g_logPath) {
		pathLen = strlen(g_logPath);
		if (pathLen > 0) {
			const size_t fullLen = pathLen + 50;
			char *fullpath = new char[fullLen];
			strcpy_s(fullpath, fullLen, g_logPath);
			if (fullpath[pathLen - 1] != '\\')
				strcat_s(fullpath, fullLen, "\\");
			strcat_s(fullpath, fullLen, "Log\\FFOne.txt");

			fopen_s(&g_logFile, fullpath, "w");
			delete[] fullpath;
		}
	}
}


extern "C" __declspec(dllexport)
void __cdecl ExitRealtime(){
	g_realtime = false;
	g_pEffect->stop();

	if( g_pDevice )
		g_pDevice->Unacquire();

	// Release any DirectInput objects.
	delete g_pEffect;
	SAFE_RELEASE( g_pDevice );
	SAFE_RELEASE( g_pDI );
}

extern "C" __declspec(dllexport)
	void __cdecl EnterRealtime(){
	HRESULT hr;
	DIPROPDWORD dipdw;

	if( FAILED( hr = DirectInput8Create( GetModuleHandle( nullptr ), DIRECTINPUT_VERSION,
										 IID_IDirectInput8, ( VOID** )&g_pDI, nullptr ) ) ) {
		g_errorMessage = "Failed to get DirectInput8 handle";
		return;
	}

	if( FAILED( hr = g_pDI->EnumDevices( DI8DEVCLASS_GAMECTRL, EnumFFDevicesCallback, nullptr,
										 DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK ) ) ) {
		g_errorMessage = "Failed to Enumerate Devices";
		return;
	}

	if( !g_pDevice ) {
		g_errorMessage = "Failed to get a suitable device";
		return;
	}

	if( FAILED( hr = g_pDevice->SetDataFormat( &c_dfDIJoystick ) ) ) {
		g_errorMessage = "Failed to set data format to Joystick";
		return;
	}

	if( FAILED( hr = g_pDevice->SetCooperativeLevel( nullptr, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND ) ) ) {
		g_errorMessage = "Failed to set cooperative mode";
		return;
	}

	// Since we will be playing force feedback engineEffects, we should disable the
	// auto-centering spring.
	dipdw.diph.dwSize = sizeof( DIPROPDWORD );
	dipdw.diph.dwHeaderSize = sizeof( DIPROPHEADER );
	dipdw.diph.dwObj = 0;
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData = FALSE;
	if( FAILED( hr = g_pDevice->SetProperty( DIPROP_AUTOCENTER, &dipdw.diph ) ) ) {
		g_errorMessage = "Failed to set Autocenter";
		return;
	}

	g_pDevice->Acquire();
	g_pEffect = new rf2Effect(g_pDevice);

	g_realtime = true;
    if (g_logFile != NULL) {
        fprintf(g_logFile, (g_errorMessage)?"%s\n":"No errors!\n", g_errorMessage);
    }

}

extern "C" __declspec(dllexport)
void __cdecl Shutdown(){
	fclose(g_logFile);
}

extern "C" __declspec(dllexport)
void __cdecl UpdateTelemetry(void* info) {
	TelemInfoV01* telem = (TelemInfoV01*)info;
#define SLIDE(v) (std::abs(v) > SLIDE_VELOCITY_THRESHOLD)


	if(g_realtime && g_pEffect){
		g_pEffect->setRPM(telem->mEngineRPM, telem->mEngineMaxRPM);

        //Check for sliding (I dont think this is actual slide, just cornering speed...)
        char slidingWheels = (
                ((SLIDE(telem->mWheel[0].mLateralPatchVel))?W_FL:0x0) |
                ((SLIDE(telem->mWheel[1].mLateralPatchVel))?W_FR:0x0) |
                ((SLIDE(telem->mWheel[2].mLateralPatchVel))?W_RL:0x0) |
                ((SLIDE(telem->mWheel[3].mLateralPatchVel))?W_RR:0x0)
        );
        double maxSlide = 0.0f;
        for(int i=0; i<4; i++){
            if( std::abs(telem->mWheel[i].mLateralPatchVel) > maxSlide)
                maxSlide = std::abs(telem->mWheel[i].mLateralGroundVel);
        }
        double slideCoefficient = maxSlide/MAX_SLIDE_FORCE_AT; //We dont cap this on purpose

		char lockedSpinningWheels = 0x0000;
		if(std::abs(telem->mLocalVel.z) > 0.1f) {
			double lock_threshold = std::abs(telem->mLocalVel.z)*WHEEL_LOCK_BELOW_PERCENTAGE;// in m/s
			double spin_threshold = std::abs(telem->mLocalVel.z)*(2.0f-WHEEL_LOCK_BELOW_PERCENTAGE);// in m/s
			for (int i = 0; i < 4; i++) {
				double wheel_av = std::abs(telem->mWheel[i].mRotation); //in rad/s
				double radius = ((double) telem->mWheel[i].mStaticUndeflectedRadius) * 0.01f; //in meters
				double wheel_v = radius * wheel_av;
				if (wheel_v < lock_threshold || wheel_v > spin_threshold){
					lockedSpinningWheels = lockedSpinningWheels | (1 << i);
				}
			}
		}

		if(lockedSpinningWheels) {
			slideCoefficient = 2.2f;
			slidingWheels = 0x0;
		}

		//Check for rumblestrips
		char rumbleLeft = (telem->mWheel[0].mSurfaceType==5 || telem->mWheel[2].mSurfaceType==5)?(W_FL|W_RL):(0x0);
		char rumbleRight = (telem->mWheel[1].mSurfaceType==5 || telem->mWheel[3].mSurfaceType==5)?(W_FR|W_RR):(0x0);
		if(rumbleLeft||rumbleRight) {
            slideCoefficient = 0.5f;
            lockedSpinningWheels = 0x0;
            slidingWheels = 0x0;
        }

		g_pEffect->slideWheels(slidingWheels | lockedSpinningWheels | rumbleLeft | rumbleRight, slideCoefficient);

		g_pEffect->play()

	}
}


