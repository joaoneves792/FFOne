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

#define SLIDE_THRESHOLD 0.01f

#define SAFE_DELETE(p)  { if(p) { delete (p);	 (p)=nullptr; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }
FILE*					g_logFile = nullptr;
char*					g_logPath = nullptr;
bool					g_realtime = false;
LPDIRECTINPUT8			g_pDI = nullptr;
LPDIRECTINPUTDEVICE8	g_pDevice = nullptr;
rf2Effect* 				g_pEffect = nullptr;
const char*             g_errorMessage = nullptr;
double                  maxFract = -200.0f;
double					minFract = +200.0f;
double 					maxThreshold = -200.0f;
double					minThreshold = +200.0f;
double 					longitudinalMax = -200.0f;
double					longitudinalMin = +3000.0f;
double 					maxSpeed = -1000.0f;


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
void __cdecl EnterRealtime(){
	g_realtime = true;
	if (g_logFile != NULL) {
		fprintf(g_logFile, "Enter realtime\n");
		fprintf(g_logFile, (g_errorMessage)?"%s\n":"No errors!\n", g_errorMessage);
	}
}

extern "C" __declspec(dllexport)
void __cdecl ExitRealtime(){
	g_realtime = false;
	g_pEffect->stop();
	fprintf(g_logFile, "Lateral Max: %e\n", maxThreshold);
	fprintf(g_logFile, "Lateral Min: %e\n", minThreshold);
	fprintf(g_logFile, "Long Max: %e\n", longitudinalMax);
	fprintf(g_logFile, "Long Min: %e\n", longitudinalMin);
	fprintf(g_logFile, "Max Speed: %e\n", maxSpeed);
	fprintf(g_logFile, "Max Grip: %f\n", maxFract);
	fprintf(g_logFile, "Min Grip: %f\n", minFract);


}

extern "C" __declspec(dllexport)
void __cdecl Startup(){
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

}

extern "C" __declspec(dllexport)
void __cdecl Shutdown(){
	if( g_pDevice )
		g_pDevice->Unacquire();

	// Release any DirectInput objects.
	delete g_pEffect;
	SAFE_RELEASE( g_pDevice );
	SAFE_RELEASE( g_pDI );
	fclose(g_logFile);
}

extern "C" __declspec(dllexport)
void __cdecl UpdateTelemetry(void* info) {
	TelemInfoV01* telem = (TelemInfoV01*)info;

	if(g_realtime && g_pEffect){
		g_pEffect->setRPM(telem->mEngineRPM, telem->mEngineMaxRPM);

		if(std::abs(telem->mWheel[3].mLateralGroundVel) > maxThreshold)
			maxThreshold = std::abs(telem->mWheel[3].mLateralGroundVel);

		if(std::abs(telem->mWheel[3].mLateralGroundVel) < minThreshold)
			minThreshold = std::abs(telem->mWheel[3].mLateralGroundVel);

		if(std::abs(telem->mWheel[3].mLongitudinalGroundVel) > longitudinalMax)
			longitudinalMax = std::abs(telem->mWheel[3].mLongitudinalGroundVel);

		if(std::abs(telem->mWheel[3].mLongitudinalGroundVel) < longitudinalMin)
			longitudinalMin = std::abs(telem->mWheel[3].mLongitudinalGroundVel);

		if(std::abs(telem->mWheel[3].mGripFract) > maxFract)
			maxFract = std::abs(telem->mWheel[3].mGripFract);

		if(std::abs(telem->mWheel[3].mGripFract) < minFract)
			minFract = std::abs(telem->mWheel[3].mGripFract);

		if(std::abs(telem->mLocalVel.z) > maxSpeed)
			maxSpeed = std::abs(telem->mLocalVel.z);

		g_pEffect->play();
	}
}


