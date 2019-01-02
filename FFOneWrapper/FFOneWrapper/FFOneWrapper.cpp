#include "FFOneWrapper.h"
#include <windows.h>
#include <tchar.h>
#include "uthash/utlist.h"

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
	module* m, mtmp;
	LL_FOREACH_SAFE(_secondaryModules, m, mtmp) {
		FreeLibrary(m->m);
		delete[] m->name;
		LL_DELETE(_secondaryModule, m);
		free(m);
	}

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
		module* m;
		LL_FOREACH(_secondaryModules, m){
			_ftprintf(_logFile, TEXT("Module: %s\n"), m->name);
		}
		fprintf( _logFile, (_exStartup)?"Startup present\n":"");
		fprintf( _logFile, (_exShutdown)?"Shutdown present\n":"");
		fprintf( _logFile, (_exLoad)?"Load present\n":"");
		fprintf( _logFile, (_exUnload)?"Unload present\n":"");
		fprintf( _logFile, (_exStartSession)?"StartSession present\n":"");
		fprintf( _logFile, (_exEndSession)?"EndSession present\n":"");
		fprintf( _logFile, (_exEnterRealtime)?"EnterRealtime present\n":"");
		fprintf( _logFile, (_exExitRealtime)?"ExitRealtime present\n":"");
		fprintf( _logFile, (_exUpdateScoring)?"UpdateScoring present\n":"");
		fprintf( _logFile, (_exUpdateTelemetry)?"UpdateTelemetry present\n":"");
		fprintf( _logFile, (_exUpdateGraphics)?"UpdateGraphics present\n":"");
		fprintf( _logFile, (_exSetEnvironment)?"SetEnvironment present\n":"");
		fprintf( _logFile, (_exInitScreen)?"SetEnvironment present\n":"");
		fprintf( _logFile, (_exRenderAfterOverlays)?"SetEnvironment present\n":"");

	}
	//Pass along to 2nd stage
    func* f;
    LL_FOREACH(_exStartup, f){
		((exStartup)f)(version);
    }
}

void FFOneWrapper::Shutdown(){
	func* f;
	LL_FOREACH(_exShutdown, f){
		((exShutdown)f)();
	}
}
 
void FFOneWrapper::Load() {
	func* f;
	LL_FOREACH(_exLoad, f){
		((exLoad)f)();
	}
}

void FFOneWrapper::Unload(){
	func* f;
	LL_FOREACH(_exUnload, f){
		((exUnload)f)();
	}
}
  
void FFOneWrapper::StartSession(){
	func* f;
	LL_FOREACH(_exStartSession, f){
		((exStartSession)f)();
	}
}
  
void FFOneWrapper::EndSession(){
	func* f;
	LL_FOREACH(_exEndSession, f){
		((exEndSession)f)();
	}
}

void FFOneWrapper::EnterRealtime(){
	func* f;
	LL_FOREACH(_exEnterRealtime, f){
		((exEnterRealtime)f)();
	}
}

void FFOneWrapper::ExitRealtime(){
	func* f;
	LL_FOREACH(_exExitRealtime, f){
		((exExitRealtime)f)();
	}
}

 
void FFOneWrapper::UpdateScoring( const ScoringInfoV01 &info ) {
	func* f;
	LL_FOREACH(_exUpdateScoring, f){
		((exUpdateScoring)f)((void*)&info);
	}
}

void FFOneWrapper::UpdateTelemetry( const TelemInfoV01 &info ) {
	func* f;
	LL_FOREACH(_exUpdateTelemetry, f){
		((exUpdateTelemetry)f)((void*)&info);
	}
}

void FFOneWrapper::UpdateGraphics( const GraphicsInfoV02 &info ){
	func* f;
	LL_FOREACH(_exUpdateGraphics, f){
		((exUpdateGraphics)f)((void*)&info);
	}
}

void FFOneWrapper::InitScreen(const ScreenInfoV01 &info) {
	func* f;
	LL_FOREACH(_exInitScreen, f){
		((exInitScreen)f)((void*)&info);
	}
}

void FFOneWrapper::RenderScreenAfterOverlays(const ScreenInfoV01 &info) {
	func* f;
	LL_FOREACH(_exRenderAfterOverlays, f){
		((exRenderAfterOverlays)f)((void*)&info);
	}
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


	if(!_secondaryModules) {
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
			MultiByteToWideChar(CP_ACP, 0, _logPath, -1, searchPath, w_pathLen);

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
						LL_APPEND(_secondaryModules, m)
					}
					delete[] modulePath;

				} while (FindNextFile(hFind, &FindFileData));
				FindClose(hFind);
			}
			delete[] searchPath;
			delete[] searchPattern;
		}



		// resolve function addresses here
		module *m;
		LL_FOREACH(_secondaryModules, m) {
			void *f;
			f = (void *) GetProcAddress(m->m, "Startup");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exStartup, newFunc)}
			f = (void *) GetProcAddress(m->m, "Shutdown");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exShutdown, newFunc)}
			f = (void *) GetProcAddress(m->m, "Load");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exLoad, newFunc)}
			f = (void *) GetProcAddress(m->m, "Unload");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exUnload, newFunc)}
			f = (void *) GetProcAddress(m->m, "StartSession");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exStartSession, newFunc)}
			f = (void *) GetProcAddress(m->m, "EndSession");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exEndSession, newFunc)}
			f = (void *) GetProcAddress(m->m, "EnterRealtime");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exEnterRealtime, newFunc)}
			f = (void *) GetProcAddress(m->m, "ExitRealtime");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exExitRealtime, newFunc)}

			f = (void *) GetProcAddress(m->m, "UpdateScoring");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exUpdateScoring, newFunc)}
			f = (void *) GetProcAddress(m->m, "UpdateTelemetry");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exUpdateTelemetry, newFunc)}
			f = (void *) GetProcAddress(m->m, "UpdateGraphics");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exUpdateGraphics, newFunc)}
			f = (void *) GetProcAddress(m->m, "SetEnvironment");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exSetEnvironment, newFunc)}
			f = (void *) GetProcAddress(m->m, "InitScreen");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exInitScreen, newFunc)}
			f = (void *) GetProcAddress(m->m, "RenderAfterOverlays");
			if (f) {func *newFunc = new func; newFunc->f = f; LL_APPEND(_exRenderAfterOverlays, newFunc)}
		}
	}


	func* f;
	LL_FOREACH(_exSetEnvironment, f){
		((exSetEnvironment)f)((void*)&info);
	}
}
