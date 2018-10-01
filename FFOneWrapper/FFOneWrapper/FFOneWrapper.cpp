#include "FFOneWrapper.h"
#include <windows.h>

extern "C" __declspec( dllexport )
const char * __cdecl GetPluginName()                   { return( "FFOne - 2008.02.13" ); }

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

	_loadSuccessfull = false;
	_loadAttempted = false;

	_logFile = nullptr;
	_logPath = nullptr;

	_exStartup = nullptr;
	_exShutdown = nullptr;
	_exLoad = nullptr;
	_exUnload = nullptr;
	_exStartSession = nullptr;
	_exEndSession = nullptr;
	_exEnterRealtime = nullptr;
	_exExitRealtime = nullptr;

	_exUpdateScoring = nullptr;
	_exUpdateTelemetry = nullptr;
	_exUpdateGraphics = nullptr;
	_exSetEnvironment = nullptr;



}


FFOneWrapper::~FFOneWrapper(void)
{
	if(_loadSuccessfull){
		FreeLibrary(_secondaryModule);
	}
	delete[] _logPath;
	delete[] _modulePath;
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
            strcat_s( fullpath, fullLen, "Log\\FFOnePluginLog.txt" );

            fopen_s( &_logFile, fullpath, "w" );
            delete [] fullpath;
        }
    }

    if(_logFile != NULL){
        fprintf( _logFile, "\n--- FFONE Session started ---\n" );
		fprintf( _logFile, (_loadAttempted)?"Attempted to load second stage at %s\n":"LOAD NOT ATTEMPTED at %s! (Should not happen)\n", _modulePath);
		fprintf( _logFile, "Errors: %d\n", _errorcode);
		fprintf( _logFile, (_loadSuccessfull)?"Successfully loaded second stage\n":"FAILED to load second stage\n");
		if(_loadSuccessfull){
			fprintf( _logFile, (_exStartup)?"Startup present\n":"Startup missing\n");
			fprintf( _logFile, (_exShutdown)?"Shutdown present\n":"Shutdown missing\n");
			fprintf( _logFile, (_exLoad)?"Load present\n":"Load missing\n");
			fprintf( _logFile, (_exUnload)?"Unload present\n":"Unload missing\n");
			fprintf( _logFile, (_exStartSession)?"StartSession present\n":"StartSession missing\n");
			fprintf( _logFile, (_exEndSession)?"EndSession present\n":"EndSession missing\n");
			fprintf( _logFile, (_exEnterRealtime)?"EnterRealtime present\n":"EnterRealtime missing\n");
			fprintf( _logFile, (_exExitRealtime)?"ExitRealtime present\n":"ExitRealtime missing\n");
			fprintf( _logFile, (_exUpdateScoring)?"UpdateScoring present\n":"UpdateScoring missing\n");
			fprintf( _logFile, (_exUpdateTelemetry)?"UpdateTelemetry present\n":"UpdateTelemetry missing\n");
			fprintf( _logFile, (_exUpdateGraphics)?"UpdateGraphics present\n":"UpdateGraphics missing\n");
			fprintf( _logFile, (_exSetEnvironment)?"SetEnvironment present\n":"SetEnvironment missing\n");
		}
	}
	//Pass along to 2nd stage
	if(_exStartup){
		_exStartup(version);
	}
}

void FFOneWrapper::Shutdown(){
	if(_exShutdown){
		_exShutdown();
	}
}
 
void FFOneWrapper::Load() {
	if(_exLoad){
		_exLoad();
	}
}

void FFOneWrapper::Unload(){
	if(_exUnload){
		_exUnload();
	}
}
  
void FFOneWrapper::StartSession(){
	
	//Pass along to second stage
	if(_exStartSession){
		_exStartSession();
	}
}
  
void FFOneWrapper::EndSession(){
	if(_exEndSession){
		_exEndSession();
	}
}

void FFOneWrapper::EnterRealtime(){
	if(_exEnterRealtime){
		_exEnterRealtime();
	}
}

void FFOneWrapper::ExitRealtime(){
	if(_exExitRealtime){
		_exExitRealtime();
	}
}

 
void FFOneWrapper::UpdateScoring( const ScoringInfoV01 &info ) {
	if(_exUpdateScoring){
		_exUpdateScoring((void*)&info);
	}
}

void FFOneWrapper::UpdateTelemetry( const TelemInfoV01 &info ) {
	if(_exUpdateTelemetry){
		_exUpdateTelemetry((void*)&info);
	}
}

void FFOneWrapper::UpdateGraphics( const GraphicsInfoV02 &info ){
	if(_exUpdateGraphics){
		_exUpdateGraphics((void*)&info);
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

	if(!_loadSuccessfull){
		//const char relative_path[] = "..\\Bin32\\Plugins\\libFFOne.dll";
		const wchar_t relative_path[] = TEXT("FFOne.dll");
		_modulePath = new char[2];
		_modulePath[0] = 'A';
		_modulePath[1] = 0;
		/*size_t pathLen = strlen( _logPath );
        if( pathLen > 0 ){
            const size_t fullLen = pathLen + 50;
            char *fullpath = new char[ fullLen ];
            strcpy_s( fullpath, fullLen, _logPath );
            if( fullpath[ pathLen - 1 ] != '\\' )
				strcat_s( fullpath, fullLen, "\\" );
            strcat_s( fullpath, fullLen, relative_path );

			
			pathLen = strlen(fullpath) + 1;
			_modulePath = new char[ pathLen ];
			strcpy_s( _modulePath, pathLen, fullpath );*/

			//_secondaryModule = LoadLibrary((LPCWSTR)simple_path);
			//LPVOID lpMsgBuf;
			//_secondaryModule = LoadLibrary((LPCWSTR)fullpath);
			_loadAttempted = true;        
			_secondaryModule = LoadLibrary(relative_path);
			
			if(NULL == _secondaryModule){
				_errorcode = GetLastError();
			}
			/*delete [] fullpath;*/

			/*if(NULL == _secondaryModule){
				DWORD dw = GetLastError(); 
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
							dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL );
				size_t messageLen = strlen((char*)lpMsgBuf);
				delete[] _errorMessage;
				_errorMessage = new char[messageLen];
				strcpy_s(_errorMessage, messageLen, (char*)lpMsgBuf);
			}*/
			_loadSuccessfull = (_secondaryModule != NULL);
			
			
	}
	if(_loadSuccessfull){
			// resolve function address here	
			_exStartup = (exStartup)GetProcAddress(_secondaryModule, "Startup");
			_exShutdown = (exShutdown)GetProcAddress(_secondaryModule, "Shutdown");
			_exLoad = (exLoad)GetProcAddress(_secondaryModule, "Load");
			_exUnload = (exUnload)GetProcAddress(_secondaryModule, "Unload");
			_exStartSession = (exStartSession)GetProcAddress(_secondaryModule, "StartSession");
			_exEndSession = (exEndSession)GetProcAddress(_secondaryModule, "EndSession");
			_exEnterRealtime = (exEnterRealtime)GetProcAddress(_secondaryModule, "EnterRealtime");
			_exExitRealtime = (exExitRealtime)GetProcAddress(_secondaryModule, "ExitRealtime");

			_exUpdateScoring = (exUpdateScoring)GetProcAddress(_secondaryModule, "UpdateScoring");
			_exUpdateTelemetry = (exUpdateTelemetry)GetProcAddress(_secondaryModule, "UpdateTelemetry");
			_exUpdateGraphics = (exUpdateGraphics)GetProcAddress(_secondaryModule, "UpdateGraphics");
			_exSetEnvironment = (exSetEnvironment)GetProcAddress(_secondaryModule, "SetEnvironment");
		
	}

    //Pass along to second stage
	if(_exSetEnvironment){
		_exSetEnvironment((void*)&info);
	}
}
