//
// Created by joao on 9/30/18.
//

#include "PluginObjects.h"
#include "InternalsPlugin.h"
#include <cstdio>                  // for logging
#include <cmath>

//#include <windows.h>
// plugin information


FILE*					g_logFile = nullptr;
char*					g_logPath = nullptr;
bool					g_realtime = false;


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
}

extern "C" __declspec(dllexport)
void __cdecl ExitRealtime(){
	g_realtime = false;
}

extern "C" __declspec(dllexport)
void __cdecl InitScreen(void* info){
	ScreenInfoV01* screenInfo = (ScreenInfoV01*)info;
	if (g_logFile != NULL) {
		fprintf(g_logFile, "InitScreen: w%ld h%ld\n", screenInfo->mWidth, screenInfo->mHeight);
	}
}

extern "C" __declspec(dllexport)
void __cdecl RenderAfterOverlays(void* info){
	ScreenInfoV01* screenInfo = (ScreenInfoV01*)info;
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


