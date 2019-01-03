#pragma once
#include "PluginObjects.hpp"
#include "InternalsPlugin.hpp"
#include <cstdio>                  // for logging
#include <list>


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
typedef __declspec(dllimport) void (__cdecl *exInitScreen)(const void*);
typedef __declspec(dllimport) void (__cdecl *exRenderAfterOverlays)(const void*);


typedef struct module{
	wchar_t* name;
	HINSTANCE m;
}module;

class FFOneWrapper: public InternalsPluginV07{
private:
	// logging
    char* _logPath;                       // path to write logs to
    FILE* _logFile;                       // the file pointer
	DWORD _errorcode;
	

	//Second stage module
	std::list<module*> _secondaryModules;


	std::list<exStartup> _exStartup;
	std::list<exShutdown>  _exShutdown;
	std::list<exLoad> _exLoad;
	std::list<exUnload> _exUnload;
	std::list<exStartSession> _exStartSession;
	std::list<exEndSession> _exEndSession;
	std::list<exEnterRealtime> _exEnterRealtime;
	std::list<exExitRealtime> _exExitRealtime;

	std::list<exUpdateScoring> _exUpdateScoring;
	std::list<exUpdateTelemetry> _exUpdateTelemetry;
	std::list<exUpdateGraphics> _exUpdateGraphics;
	std::list<exSetEnvironment> _exSetEnvironment;
	std::list<exInitScreen> _exInitScreen;
	std::list<exRenderAfterOverlays> _exRenderAfterOverlays;

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

  void InitScreen(const ScreenInfoV01 &info);
  void RenderScreenAfterOverlays( const ScreenInfoV01 &info ); // after rFactor overlays
};

