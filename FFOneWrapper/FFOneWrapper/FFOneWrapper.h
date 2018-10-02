#pragma once
#include "PluginObjects.hpp"
#include "InternalsPlugin.hpp"
#include <cstdio>                  // for logging

typedef __declspec(dllimport) void (__cdecl *exStartup)(long); 
typedef __declspec(dllimport) void (__cdecl *exShutdown)(void);
typedef __declspec(dllimport) void (__cdecl *exLoad)(void);
typedef __declspec(dllimport) void (__cdecl *exUnload)(void);
typedef __declspec(dllimport) void (__cdecl *exStartSession)(void);
typedef __declspec(dllimport) void (__cdecl *exEndSession)(void);
typedef __declspec(dllimport) void (__cdecl *exEnterRealtime)(void);
typedef __declspec(dllimport) void (__cdecl *exExitRealtime)(void);

typedef __declspec(dllimport) void (__cdecl *exUpdateScoring)(const void*);
typedef __declspec(dllimport) void (__cdecl *exUpdateTelemetry)(const void*);
typedef __declspec(dllimport) void (__cdecl *exUpdateGraphics)(const void*);
typedef __declspec(dllimport) void (__cdecl *exSetEnvironment)(const void*);

#define MAX_PLUGINS 10

class FFOneWrapper: public InternalsPluginV07{
private:
	// logging
    char* _logPath;                       // path to write logs to
    FILE* _logFile;                       // the file pointer
	DWORD _errorcode;
	

	//Second stage module
	wchar_t* _moduleNames[MAX_PLUGINS];
	HINSTANCE _secondaryModule[MAX_PLUGINS];

	exStartup _exStartup[MAX_PLUGINS];
	exShutdown _exShutdown[MAX_PLUGINS];
	exLoad _exLoad[MAX_PLUGINS];
	exUnload _exUnload[MAX_PLUGINS];
	exStartSession _exStartSession[MAX_PLUGINS];
	exEndSession _exEndSession[MAX_PLUGINS];
	exEnterRealtime _exEnterRealtime[MAX_PLUGINS];
	exExitRealtime _exExitRealtime[MAX_PLUGINS];

	exUpdateScoring _exUpdateScoring[MAX_PLUGINS];
	exUpdateTelemetry _exUpdateTelemetry[MAX_PLUGINS];
	exUpdateGraphics _exUpdateGraphics[MAX_PLUGINS];
	exSetEnvironment _exSetEnvironment[MAX_PLUGINS];

public:
	FFOneWrapper(void);
	~FFOneWrapper(void);

  void Startup( long version );                      // sim startup with version * 1000
  void Shutdown();                                   // sim shutdown

  void Load();                                       // scene/track load
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
  void UpdateGraphics( const GraphicsInfoV02 &info ); // update plugin with extended graphics info

  void SetEnvironment( const EnvironmentInfoV01 &info ); // may be called whenever the environment changes
};

