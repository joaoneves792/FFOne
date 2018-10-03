#include <iostream>
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include "dinput.h"
#include <math.h>

#include "rf2Effect.h"

//-----------------------------------------------------------------------------
// Defines, constants, and global variables
//-----------------------------------------------------------------------------
#define SAFE_DELETE(p)  { if(p) { delete (p);	 (p)=nullptr; } }
#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=nullptr; } }

#define FEEDBACK_WINDOW_X	   20
#define FEEDBACK_WINDOW_Y	   60
#define FEEDBACK_WINDOW_WIDTH   200

LPDIRECTINPUT8		  g_pDI = nullptr;
LPDIRECTINPUTDEVICE8	g_pDevice = nullptr;
LPDIRECTINPUTEFFECT	 g_pEffect = nullptr;
BOOL					g_bActive = TRUE;
DWORD				   g_dwNumForceFeedbackAxis = 0;
INT					 g_nXForce;
INT					 g_nYForce;
DWORD g_dwLastEffectSet; // Time of the previous force feedback engineEffect set

VOID FreeDirectInput()
{
	// Unacquire the device one last time just in case 
	// the app tried to exit while the device is still acquired.
	if( g_pDevice )
		g_pDevice->Unacquire();

	// Release any DirectInput objects.
	SAFE_RELEASE( g_pEffect );
	SAFE_RELEASE( g_pDevice );
	SAFE_RELEASE( g_pDI );
}

//-----------------------------------------------------------------------------
// Name: EnumFFDevicesCallback()
// Desc: Called once for each enumerated force feedback device. If we find
//	   one, create a device interface on it so we can play with it.
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

//-----------------------------------------------------------------------------
// Name: EnumAxesCallback()
// Desc: Callback function for enumerating the axes on a joystick and counting
//	   each force feedback enabled axis
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumAxesCallback( const DIDEVICEOBJECTINSTANCE* pdidoi,
								VOID* pContext )
{
	auto pdwNumForceFeedbackAxis = reinterpret_cast<DWORD*>( pContext );

	if( ( pdidoi->dwFlags & DIDOI_FFACTUATOR ) != 0 )
		( *pdwNumForceFeedbackAxis )++;

	return DIENUM_CONTINUE;
}

int main(int argc, char **argv) {
	HRESULT hr;
	DIPROPDWORD dipdw;

	// Register with the DirectInput subsystem and get a pointer
	// to a IDirectInput interface we can use.
	if( FAILED( hr = DirectInput8Create( GetModuleHandle( nullptr ), DIRECTINPUT_VERSION,
										IID_IDirectInput8, ( VOID** )&g_pDI, nullptr ) ) ){
		return hr;
	}

	// Look for a force feedback device we can use
	if( FAILED( hr = g_pDI->EnumDevices( DI8DEVCLASS_GAMECTRL,
										 EnumFFDevicesCallback, nullptr,
										 DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK ) ) ){
		return hr;
	}

	if( !g_pDevice ){
		std::cout << "No compatible devices!" << std::endl;
		return -1;
	}else{
		std::cout << "Got a FF device!" << std::endl;
	}
	// Set the data format to "simple joystick" - a predefined data format. A
	// data format specifies which controls on a device we are interested in,
	// and how they should be reported.
	//
	// This tells DirectInput that we will be passing a DIJOYSTATE structure to
	// IDirectInputDevice8::GetDeviceState(). Even though we won't actually do
	// it in this sample. But setting the data format is important so that the
	// DIJOFS_* values work properly.
	if( FAILED( hr = g_pDevice->SetDataFormat( &c_dfDIJoystick ) ) ){
		std::cout << "Failed to set data Format" << std::endl;
		return hr;
	}

	// Set the cooperative level to let DInput know how this device should
	// interact with the system and with other DInput applications.
	// Exclusive access is required in order to perform force feedback.
	
	/*TODO in rFactor2 get the window handle and set like this:
	 * See: https://stackoverflow.com/questions/2620409/getting-hwnd-of-current-process*/
	
	/*if( FAILED( hr = g_pDevice->SetCooperativeLevel( nullptr, DISCL_EXCLUSIVE | DISCL_FOREGROUND ) ) ){
		return hr;
	}*/
	
	if( FAILED( hr = g_pDevice->SetCooperativeLevel( nullptr, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND ) ) ){
		std::cout << "Failed to set cooperative Level" << std::endl;
		return hr;
	}
	std::cout << "Cooperative level set!" << std::endl;

	// Since we will be playing force feedback engineEffects, we should disable the
	// auto-centering spring.
	
	dipdw.diph.dwSize = sizeof( DIPROPDWORD );
	dipdw.diph.dwHeaderSize = sizeof( DIPROPHEADER );
	dipdw.diph.dwObj = 0;
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData = FALSE;

	if( FAILED( hr = g_pDevice->SetProperty( DIPROP_AUTOCENTER, &dipdw.diph ) ) )
		return hr;
	
	std::cout << "autocenter set" << std::endl;

	// Enumerate and count the axes of the joystick 
	if( FAILED( hr = g_pDevice->EnumObjects( EnumAxesCallback, ( VOID* )&g_dwNumForceFeedbackAxis, DIDFT_AXIS ) ) ){
		return hr;

	}
	std::cout << "Got " << g_dwNumForceFeedbackAxis << " FF Axis" << std::endl;

	// This simple sample only supports one or two axis joysticks
	if( g_dwNumForceFeedbackAxis > 2 )
		g_dwNumForceFeedbackAxis = 2;

        g_pDevice->Acquire();

	// This application needs only one engineEffect: Applying raw forces.
	/*DWORD rgdwAxes[2] = { DIJOFS_X, DIJOFS_Y };
	LONG rglDirection[2] = { 0, 0 };
	

	DIPERIODIC wheelPeriodic;
	DIEFFECT wheelEff = {};
	wheelEff.dwSize = sizeof( DIEFFECT );
	wheelEff.dwFlags = DIEFF_POLAR | DIEFF_OBJECTOFFSETS;
	wheelEff.dwDuration = INFINITE;
	wheelEff.dwSamplePeriod = 0;
	wheelEff.dwGain = DI_FFNOMINALMAX;
	wheelEff.dwTriggerButton = DIEB_NOTRIGGER;
	wheelEff.dwTriggerRepeatInterval = 0;
	wheelEff.cAxes = g_dwNumForceFeedbackAxis;
	wheelEff.rgdwAxes = rgdwAxes;
	wheelEff.rglDirection = rglDirection;
	wheelEff.lpEnvelope = 0;
	wheelEff.cbTypeSpecificParams = sizeof( DIPERIODIC );
	wheelEff.lpvTypeSpecificParams = &wheelPeriodic;
	wheelEff.dwStartDelay = 0;
	// Create the prepared wheelEffect
	if( FAILED( hr = g_pDevice->CreateEffect( GUID_Sine, &wheelEff, &wheel, nullptr ) ) )
	{
		return hr;
	}

	if( !wheel ){
		std::cout << "Failed to create wheelEffect!" << std::endl;
	}else{
		std::cout << "Effect created!" << std::endl;
	}*/
	/*cf.lMagnitude = ( DWORD )sqrt( ( double )g_nXForce * ( double )g_nXForce +
									   ( double )g_nYForce * ( double )g_nYForce );*/

	/*wheelPeriodic.dwMagnitude = 1200;
	wheelPeriodic.lOffset = 0;
	wheelPeriodic.dwPhase = 0;
	wheelPeriodic.dwPeriod = 15127000;*/

	/*for(int i=0; i<=360; i=i+10){
		rglDirection[0] = i*100;
		//std::cout << rglDirection[0] << " " <<  360 - (((rglDirection[0] + 0xc000)& 0xffff) *M_PI /0x8000) * 180 / M_PI <<  std::endl;
		std::cout << rglDirection[0] << " " << std::hex << (rglDirection[0]/33)*36 + 9000 <<  std::endl;
		//rglDirection[1] = j*DI_FFNOMINALMAX;
		//wheelPeriodic.dwMagnitude = (DWORD)sqrt((double)rglDirection[0]*(double)rglDirection[0]+(double)rglDirection[1]*(double)rglDirection[1]);
		wheel->SetParameters( &wheelEff, DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_START );
		Sleep(1000);
	}*/
	/*for(int i=0; i<10; i++){
		int LEFT_TYRE = 13500;
		int RIGHT_TYRE = 22500;
		rglDirection[0] = (i%2)?LEFT_TYRE:RIGHT_TYRE;
		wheel->SetParameters( &wheelEff, DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_START );
		Sleep(1000);
	}*/

	auto ff = new rf2Effect(g_pDevice);
	int rpm=2000;
	for(int i=0; i<9;){
    	rpm=rpm+10;
    	if(rpm>12000) {
			rpm = 2000;
			ff->stop();
			Sleep(1000);
			i = i + 1;
			switch (i) {
				case 1:
					std::cout << "FL" << std::endl;
					ff->slideWheels(W_FL);
                    rpm=rpm+100;
					break;
				case 2:
					std::cout << "FR" << std::endl;
					ff->slideWheels(W_FR);
                    rpm=rpm+100;
					break;
				case 3:
					std::cout << "RL" << std::endl;
					ff->slideWheels(W_RL);
                    rpm=rpm+100;
					break;
				case 4:
					std::cout << "RR" << std::endl;
					ff->slideWheels(W_RR);
                    rpm=rpm+100;
					break;
				case 5:
					std::cout << "F" << std::endl;
					ff->slideWheels(W_FL | W_FR);
					rpm=rpm+100;
					break;
				case 6:
					std::cout << "R" << std::endl;
					ff->slideWheels(W_RL | W_RR);
					rpm=rpm+100;
					break;
				case 7:
					std::cout << "L" << std::endl;
					ff->slideWheels(W_FL | W_RL);
					rpm=rpm+100;
					break;
				case 8:
					std::cout << "R" << std::endl;
					ff->slideWheels(W_FR | W_RR);
					rpm=rpm+100;
					break;
				default:
					break;
			}
		}
		ff->setRPM(rpm, 12000);
    	ff->play();
    	Sleep(5); //Below 25 we get errors!
	}


	delete ff;

	FreeDirectInput();
	return 0;
}

