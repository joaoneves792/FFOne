//
// Created by joao on 9/30/18.
//

#ifndef B_FFONEPLUGIN_H
#define B_FFONEPLUGIN_H

#include "PluginObjects.h"
#include "InternalsPlugin.h"
#include <cstdio>                  // for logging

class FFOnePlugin : public InternalsPluginV07 {
protected:
    // logging
    bool mLogging;                        // whether we want logging enabled
    char *mLogPath;                       // path to write logs to
    FILE *mLogFile;                       // the file pointer

public:

    //
    FFOnePlugin();
    ~FFOnePlugin();

    //
    void StartSession();
    void EndSession();                                  // session ended
    void SetEnvironment( const EnvironmentInfoV01 &info ); // may be called whenever the environment changes


    //virtual long WantsTelemetryUpdates() { return( 1 ); }        // whether we want telemetry updates (0=no 1=player-only 2=all vehicles)
    //virtual void UpdateTelemetry( const TelemInfoV01 &info ) {}  // update plugin with telemetry info

};


#endif //B_FFONEPLUGIN_H
