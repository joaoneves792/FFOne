//
// Created by joao on 9/30/18.
//

#include "PluginObjects.h"
#include "InternalsPlugin.h"
#include <cstdio>                  // for logging
#include <windows.h>

ExternalPlugin::ExternalPlugin(){

}

ExternalPlugin::~ExternalPlugin(){

}

int ExternalPlugin::loadExternalPlugin() {
    HINSTANCE hGetProcIDDLL = LoadLibrary("C:\\Documents and Settings\\User\\Desktop\\test.dll");

    if (!hGetProcIDDLL) {
        return EXIT_FAILURE;
    }

    f_funci funci = (f_funci) GetProcAddress(hGetProcIDDLL, "funci");
    if (!funci) {
        return EXIT_FAILURE;
    }
}