//
// Created by joao on 9/30/18.
//

#ifndef MESON_TEST_EXTERNALPLUGIN_H
#define MESON_TEST_EXTERNALPLUGIN_H

class ExternalPlugin : public InternalsPluginV07 {
protected:
    char *mPluginPath;                       // path to look for plugins

private:
    int loadExternalPlugin(char* path);

public:

    //
    ExternalPlugin();
    ~ExternalPlugin();

    //
    void Startup( long version );                     // sim startup with version * 1000
    void Shutdown();                                   // sim shutdown

    void Load();                                      // scene/track load
    void Unload();                                     // scene/track unload

    void StartSession();                               // session started
    void EndSession();                                 // session ended

    void EnterRealtime();                              // entering realtime (where the vehicle can be driven)
    void ExitRealtime();                               // exiting realtime

    // SCORING OUTPUT
    bool WantsScoringUpdates() { return( true ); }      // whether we want scoring updates
    void UpdateScoring( const ScoringInfoV01 &info );  // update plugin with scoring info (approximately five times per second)

    // GAME OUTPUT
    long WantsTelemetryUpdates() { return( 1 ); }        // whether we want telemetry updates (0=no 1=player-only 2=all vehicles)
    void UpdateTelemetry( const TelemInfoV01 &info );  // update plugin with telemetry info

    bool WantsGraphicsUpdates() { return( true ); }     // whether we want graphics updates
    void UpdateGraphics( const GraphicsInfoV01 &info );// update plugin with graphics info

    virtual bool GetCustomVariable( long i, CustomVariableV01 &var )   { return( false ); } // At startup, this will be called with increasing index (starting at zero) until false is returned. Feel free to add/remove/rearrange the variables when updating your plugin; the index does not have to be consistent from run to run.
    virtual void AccessCustomVariable( CustomVariableV01 &var )        {}                   // This will be called at startup, shutdown, and any time that the variable is changed (within the UI).
    virtual void GetCustomVariableSetting( CustomVariableV01 &var, long i, CustomSettingV01 &setting ) {} // This gets the name of each possible setting for a given variable.

};
#endif //MESON_TEST_EXTERNALPLUGIN_H
