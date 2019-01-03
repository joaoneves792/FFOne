#include "FFOneWrapper.h"
#include <windows.h>
#include <tchar.h>
#include <list>

extern "C" __declspec( dllexport )
const char * __cdecl GetPluginName()                   { return( "WRAPPER - 2008.02.13" ); }

extern "C" __declspec( dllexport )
PluginObjectType __cdecl GetPluginType()               { return( PO_INTERNALS ); }

extern "C" __declspec( dllexport )
int __cdecl GetPluginVersion()                         { return( 7 ); }  // InternalsPluginV07 functionality (if you change this return value, you must derive from the appropriate class!)

extern "C" __declspec( dllexport )
PluginObject * __cdecl CreatePluginObject()            { return( static_cast< PluginObject * >( new FFOneWrapper ) ); }

extern "C" __declspec( dllexport )
void __cdecl DestroyPluginObject( PluginObject *obj )  { delete( static_cast< FFOneWrapper * >( obj ) ); }

FFOneWrapper::FFOneWrapper(void)
{
	_errorcode = 0;

	_logFile = nullptr;
	_logPath = nullptr;
}


FFOneWrapper::~FFOneWrapper(void)
{
	for(module *m : _secondaryModules){
		FreeLibrary(m->m);
		delete[] m->name;
	}
	_secondaryModules.clear();

	_exStartup.clear();
	_exShutdown.clear();
	_exLoad.clear();
	_exUnload.clear();
	_exStartSession.clear();
	_exEndSession.clear();
	_exEnterRealtime.clear();
	_exExitRealtime.clear();

	_exUpdateScoring.clear();
	_exUpdateTelemetry.clear();
	_exUpdateGraphics.clear();
	_exSetEnvironment.clear();
	_exInitScreen.clear();
	_exRenderAfterOverlays.clear();

	delete[] _logPath;

	fclose(_logFile);
}

void FFOneWrapper::Startup( long version){
	if(!_logFile && _logPath){
        const size_t pathLen = strlen( _logPath );
        if( pathLen > 0 ){
            const size_t fullLen = pathLen + 50;
            char *fullpath = new char[ fullLen ];
            strcpy_s( fullpath, fullLen, _logPath );
            if( fullpath[ pathLen - 1 ] != '\\' )
				strcat_s( fullpath, fullLen, "\\" );
            strcat_s( fullpath, fullLen, "Log\\Wrapper.txt" );

            fopen_s( &_logFile, fullpath, "w" );
            delete [] fullpath;
        }
    }

    if(_logFile != NULL) {
		fprintf(_logFile, "\n--- Started plugin wrapper ---\n");
		if (_errorcode) {
			fprintf(_logFile, "Error : %d\n", _errorcode);
		}
		for(module *m : _secondaryModules){
				_ftprintf(_logFile, TEXT("Module: %s\n"), m->name);
		}
		fprintf( _logFile, (!_exStartup.empty())?"Startup present\n":"");
		fprintf( _logFile, (!_exShutdown.empty())?"Shutdown present\n":"");
		fprintf( _logFile, (!_exLoad.empty())?"Load present\n":"");
		fprintf( _logFile, (!_exUnload.empty())?"Unload present\n":"");
		fprintf( _logFile, (!_exStartSession.empty())?"StartSession present\n":"");
		fprintf( _logFile, (!_exEndSession.empty())?"EndSession present\n":"");
		fprintf( _logFile, (!_exEnterRealtime.empty())?"EnterRealtime present\n":"");
		fprintf( _logFile, (!_exExitRealtime.empty())?"ExitRealtime present\n":"");
		fprintf( _logFile, (!_exUpdateScoring.empty())?"UpdateScoring present\n":"");
		fprintf( _logFile, (!_exUpdateTelemetry.empty())?"UpdateTelemetry present\n":"");
		fprintf( _logFile, (!_exUpdateGraphics.empty())?"UpdateGraphics present\n":"");
		fprintf( _logFile, (!_exSetEnvironment.empty())?"SetEnvironment present\n":"");
		fprintf( _logFile, (!_exInitScreen.empty())?"InitScreen present\n":"");
		fprintf( _logFile, (!_exRenderAfterOverlays.empty())?"RenderAfterOverlays present\n":"");

	}
	//Pass along to 2nd stage
	for(exStartup f : _exStartup)
		(f)(version);

}

void FFOneWrapper::Shutdown(){
	for(exShutdown f : _exShutdown)
		(f)();
}
 
void FFOneWrapper::Load() {
	for(exLoad f : _exLoad)
		(f)();
}

void FFOneWrapper::Unload(){
	for(exUnload f : _exUnload)
		(f)();
}
  
void FFOneWrapper::StartSession(){
	for(exStartSession f : _exStartSession)
		(f)();
}
  
void FFOneWrapper::EndSession(){
	for(exEndSession f : _exEndSession)
		(f)();
}

void FFOneWrapper::EnterRealtime(){
	for(exEnterRealtime f : _exEnterRealtime)
		(f)();
}

void FFOneWrapper::ExitRealtime(){
	for(exExitRealtime f : _exExitRealtime)
		(f)();
}

 
void FFOneWrapper::UpdateScoring( const ScoringInfoV01 &info ) {
	for(exUpdateScoring f : _exUpdateScoring)
		(f)((void*)&info);
}

void FFOneWrapper::UpdateTelemetry( const TelemInfoV01 &info ) {
	for(exUpdateTelemetry f : _exUpdateTelemetry)
		(f)((void*)&info);
}

void FFOneWrapper::UpdateGraphics( const GraphicsInfoV02 &info ){
	for(exUpdateGraphics f : _exUpdateGraphics)
		(f)((void*)&info);
}

void FFOneWrapper::InitScreen(const ScreenInfoV01 &info) {
	for(exInitScreen f : _exInitScreen)
		(f)((void*)&info);
}

void FFOneWrapper::RenderScreenAfterOverlays(const ScreenInfoV01 &info) {
	for(exRenderAfterOverlays f : _exRenderAfterOverlays)
		(f)((void*)&info);
}


void FFOneWrapper::SetEnvironment( const EnvironmentInfoV01 &info ){
	// If we already have it, but it's wrong, delete it.
    if(_logPath){
        if( 0 != _stricmp( _logPath, info.mPath[ 0 ] ) )
        {
            delete [] _logPath;
            _logPath = NULL;
        }
    }

    // If we don't already have it now, then copy it.
    if(!_logPath){
        const size_t pathLen = strlen( info.mPath[ 0 ] ) + 1;
        _logPath = new char[ pathLen ];
        strcpy_s( _logPath, pathLen, info.mPath[ 0 ] );
    }


	if(_secondaryModules.empty()) {
#if _WIN64
		const wchar_t relative_path[] = TEXT("..\\Bin64\\Plugins\\Wrapped\\");
#else
		const wchar_t relative_path[] = TEXT("..\\Bin32\\Plugins\\Wrapped\\");
#endif

		const wchar_t search_wildcard[] = TEXT("*.dll");


		size_t pathLen = strlen(_logPath);
		if (pathLen > 0) {
			//Convert logPath to unicode
			size_t w_pathLen = MultiByteToWideChar(CP_ACP, 0, _logPath, -1, NULL, 0);
			const size_t fullLen = w_pathLen + 100 * sizeof(wchar_t);
			wchar_t *searchPath = new wchar_t[fullLen];
			MultiByteToWideChar(CP_ACP, 0, _logPath, -1, searchPath, (int)w_pathLen);

			//append and extra \ if necessary
			if (searchPath[w_pathLen - 1] != L'\\')
				_tcscat_s(searchPath, fullLen, TEXT("\\"));
			//append the rest of the relative path
			_tcscat_s(searchPath, fullLen, relative_path);

			//Create the search wildcard
			wchar_t *searchPattern = new wchar_t[fullLen];
			_tcsncpy_s(searchPattern, fullLen, searchPath, _tcslen(searchPath) + 1);
			_tcscat_s(searchPattern, fullLen, search_wildcard);

			//Go looking for dlls
			WIN32_FIND_DATA FindFileData;
			HANDLE hFind;
			hFind = FindFirstFile(searchPattern, &FindFileData);
			if (hFind != INVALID_HANDLE_VALUE) {
				do {
					wchar_t *modulePath = new wchar_t[fullLen];
					_tcsncpy_s(modulePath, fullLen, searchPath, _tcslen(searchPath) + 1);
					_tcscat_s(modulePath, fullLen, FindFileData.cFileName);

					//Load the modules
					HINSTANCE mod = LoadLibrary(modulePath);
					if (NULL == mod) {
						_errorcode = GetLastError();
					} else {
						size_t nameLen = _tcslen(FindFileData.cFileName);
						wchar_t *name = new wchar_t[nameLen + 1];
						_tcsncpy_s(name, nameLen + 1, FindFileData.cFileName, nameLen + 1);
						module *m = new module;
						m->name = name;
						m->m = mod;
						_secondaryModules.push_back(m);
					}
					delete[] modulePath;

				} while (FindNextFile(hFind, &FindFileData));
				FindClose(hFind);
			}
			delete[] searchPath;
			delete[] searchPattern;
		}



		// resolve function addresses here
		for(module *m : _secondaryModules){
			void *f;
			f = (void *) GetProcAddress(m->m, "Startup");
			if (f) {_exStartup.push_back((exStartup)f);}
			f = (void *) GetProcAddress(m->m, "Shutdown");
			if (f) {_exShutdown.push_back((exShutdown)f);}
			f = (void *) GetProcAddress(m->m, "Load");
			if (f) {_exLoad.push_back((exLoad)f);}
			f = (void *) GetProcAddress(m->m, "Unload");
			if (f) {_exUnload.push_back((exUnload)f);}
			f = (void *) GetProcAddress(m->m, "StartSession");
			if (f) {_exStartSession.push_back((exStartSession)f);}
			f = (void *) GetProcAddress(m->m, "EndSession");
			if (f) {_exEndSession.push_back((exEndSession)f);}
			f = (void *) GetProcAddress(m->m, "EnterRealtime");
			if (f) {_exEnterRealtime.push_back((exEnterRealtime)f);}
			f = (void *) GetProcAddress(m->m, "ExitRealtime");
			if (f) {_exExitRealtime.push_back((exExitRealtime)f);}
			
			f = (void *) GetProcAddress(m->m, "UpdateScoring");
			if (f) {_exUpdateScoring.push_back((exUpdateScoring)f);}
			f = (void *) GetProcAddress(m->m, "UpdateTelemetry");
			if (f) {_exUpdateTelemetry.push_back((exUpdateTelemetry)f);}
			f = (void *) GetProcAddress(m->m, "UpdateGraphics");
			if (f) {_exUpdateGraphics.push_back((exUpdateGraphics)f);}
			f = (void *) GetProcAddress(m->m, "SetEnvironment");
			if (f) {_exSetEnvironment.push_back((exSetEnvironment)f);}
			f = (void *) GetProcAddress(m->m, "InitScreen");
			if (f) {_exInitScreen.push_back((exInitScreen)f);}
			f = (void *) GetProcAddress(m->m, "RenderAfterOverlays");
			if (f) {_exRenderAfterOverlays.push_back((exRenderAfterOverlays)f);}
		}
	}

	for(exSetEnvironment f : _exSetEnvironment)
		(f)((void*)&info);
}
