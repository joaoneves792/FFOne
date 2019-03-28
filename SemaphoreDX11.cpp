//
// Created by joao on 9/30/18.
//

#include "PluginObjects.h"
#include "InternalsPlugin.h"
#include <cstdio>                  // for logging
#include <cmath>
#include <dxgi.h>
#include <d3dcommon.h>
#include <d3d11.h>

//#include <windows.h>
// plugin information


FILE*					g_logFile = nullptr;
char*					g_logPath = nullptr;
bool					g_realtime = false;
long                    g_width = -1;
long                    g_height = -1;


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
			strcat_s(fullpath, fullLen, "Log\\SemaphoreDX11.txt");

			fopen_s(&g_logFile, fullpath, "w");
			delete[] fullpath;
		}
	}
}

extern "C" __declspec(dllexport)
void __cdecl EnterRealtime(){
	g_realtime = true;
	if (g_logFile != NULL) {
		fprintf(g_logFile, "EnterScreen: w%ld h%ld\n", g_width, g_height);
	}
}

extern "C" __declspec(dllexport)
void __cdecl ExitRealtime(){
	g_realtime = false;
    if (g_logFile != NULL) {
        fprintf(g_logFile, "ExitScreen: w%ld h%ld\n", g_width, g_height);
    }
}

extern "C" __declspec(dllexport)
void __cdecl InitScreen(void* info){
	ScreenInfoV01* screenInfo = (ScreenInfoV01*)info;
	g_width = screenInfo->mWidth;
	g_height = screenInfo->mHeight;
}

extern "C" __declspec(dllexport)
void __cdecl RenderAfterOverlays(void* info){
	ScreenInfoV01* screenInfo = (ScreenInfoV01*)info;
	g_width = screenInfo->mWidth;
	g_height = screenInfo->mHeight;
	return;
}

extern "C" __declspec(dllexport)
void __cdecl Shutdown(){

	fclose(g_logFile);
}

extern "C" __declspec(dllexport)
void __cdecl UpdateTelemetry(void* info) {
	TelemInfoV01* telem = (TelemInfoV01*)info;
}

extern "C" __declspec(dllexport)
void __cdecl StartSession(){
	INT createDeviceFlags = 0;

	D3D_DRIVER_TYPE driverTypes[] = { D3D_DRIVER_TYPE_HARDWARE,
									  D3D_DRIVER_TYPE_REFERENCE };
	UINT numDriverTypes = 2;

	D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_0,
										  D3D_FEATURE_LEVEL_10_1,
										  D3D_FEATURE_LEVEL_10_0 };
	UINT numFeatureLevels = 3;

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof(sd) );
	sd.BufferCount                        = 2;
	sd.BufferDesc.Width                   = 1920;
	sd.BufferDesc.Height                  = 1080;
	sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator   = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.Flags                              = m_Fullscreen ? DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH : 0;
	sd.OutputWindow                       = windowHandle;
	sd.SampleDesc.Count                   = 1;
	sd.SampleDesc.Quality                 = 0;
	sd.Windowed                           = TRUE;
	sd.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;

	HRESULT hr = S_OK;

	for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
	{
		m_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(          // fails when using the editor's window
				nullptr,
				m_driverType,
				nullptr,
				createDeviceFlags,
				featureLevels,
				numFeatureLevels,
				D3D11_SDK_VERSION,
				&sd,
				&m_pSwapChain,
				&m_pD3Ddevice,
				&m_featureLevel,
				&m_pImmediateContext );

		if( SUCCEEDED(hr) ) break;
	}

	if( FAILED(hr) )
	{
		SHOW_ERROR( hr );      // 0x887A0001: DXGI_ERROR_INVALID_CALL
		return hr;
	}

	return S_OK;
}



