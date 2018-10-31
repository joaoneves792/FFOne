#include "FFOneWrapper.h"
#include <windows.h>
#include <tchar.h>

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

	for(int i=0;i<MAX_PLUGINS; i++){
		_secondaryModule[i] = nullptr;

		_exStartup[i] = nullptr;
		_exShutdown[i] = nullptr;
		_exLoad[i] = nullptr;
		_exUnload[i] = nullptr;
		_exStartSession[i] = nullptr;
		_exEndSession[i] = nullptr;
		_exEnterRealtime[i] = nullptr;
		_exExitRealtime[i] = nullptr;

		_exUpdateScoring[i] = nullptr;
		_exUpdateTelemetry[i] = nullptr;
		_exUpdateGraphics[i] = nullptr;
		_exSetEnvironment[i] = nullptr;
	}


}


FFOneWrapper::~FFOneWrapper(void)
{
	int i = 0;
	while(_secondaryModule[i]){
		FreeLibrary(_secondaryModule[i]);
		delete[] _moduleNames[i];
		i++;
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

    if(_logFile != NULL){
        fprintf( _logFile, "\n--- Started plugin wrapper ---\n" );
		if(_errorcode){
			fprintf( _logFile, "Error : %d\n", _errorcode);
		}
		int m = 0;
		while(_secondaryModule[m]){
			_ftprintf( _logFile, TEXT("Module: %s\n"), _moduleNames[m]);
			fprintf( _logFile, (_exStartup[0])?"Startup present\n":"");
			fprintf( _logFile, (_exShutdown[0])?"Shutdown present\n":"");
			fprintf( _logFile, (_exLoad[0])?"Load present\n":"");
			fprintf( _logFile, (_exUnload[0])?"Unload present\n":"");
			fprintf( _logFile, (_exStartSession[0])?"StartSession present\n":"");
			fprintf( _logFile, (_exEndSession[0])?"EndSession present\n":"");
			fprintf( _logFile, (_exEnterRealtime[0])?"EnterRealtime present\n":"");
			fprintf( _logFile, (_exExitRealtime[0])?"ExitRealtime present\n":"");
			fprintf( _logFile, (_exUpdateScoring[0])?"UpdateScoring present\n":"");
			fprintf( _logFile, (_exUpdateTelemetry[0])?"UpdateTelemetry present\n":"");
			fprintf( _logFile, (_exUpdateGraphics[0])?"UpdateGraphics present\n":"");
			fprintf( _logFile, (_exSetEnvironment[0])?"SetEnvironment present\n":"");
			m++;
		}
	}
	//Pass along to 2nd stage
	int i = 0;
	while(_secondaryModule[i]){
		if(_exStartup[i]){
			(_exStartup[i])(version);
		}
		i++;
	}
}

void FFOneWrapper::Shutdown(){
	int i = 0;
	while(_secondaryModule[i]){
		if(_exShutdown[i]){
			(_exShutdown[i])();
		}
		i++;
	}
}
 
void FFOneWrapper::Load() {
	int i = 0;
	while(_secondaryModule[i]){
		if(_exLoad[i]){
			(_exLoad[i])();
		}
		i++;
	}
}

void FFOneWrapper::Unload(){
	int i = 0;
	while(_secondaryModule[i]){
		if(_exUnload[i]){
			(_exUnload[i])();
		}
		i++;
	}
}
  
void FFOneWrapper::StartSession(){
	
	//Pass along to second stage
	int i = 0;
	while(_secondaryModule[i]){
		if(_exStartSession[i]){
			(_exStartSession[i])();
		}
		i++;
	}
}
  
void FFOneWrapper::EndSession(){
	int i = 0;
	while(_secondaryModule[i]){
		if(_exEndSession[i]){
			(_exEndSession[i])();
		}
		i++;
	}
}

void FFOneWrapper::EnterRealtime(){
	int i = 0;
	while(_secondaryModule[i]){
		if(_exEnterRealtime[i]){
			(_exEnterRealtime[i])();
		}
		i++;
	}
}

void FFOneWrapper::ExitRealtime(){
	int i = 0;
	while(_secondaryModule[i]){
		if(_exExitRealtime[i]){
			(_exExitRealtime[i])();
		}
		i++;
	}
}

 
void FFOneWrapper::UpdateScoring( const ScoringInfoV01 &info ) {
	int i = 0;
	while(_secondaryModule[i]){
		if(_exUpdateScoring[i]){
			(_exUpdateScoring[i])((void*)&info);
		}
		i++;
	}
}

void FFOneWrapper::UpdateTelemetry( const TelemInfoV01 &info ) {
	int i = 0;
	while(_secondaryModule[i]){
		if(_exUpdateTelemetry[i]){
			(_exUpdateTelemetry[i])((void*)&info);
		}
		i++;
	}
}

void FFOneWrapper::UpdateGraphics( const GraphicsInfoV02 &info ){
	int i = 0;
	while(_secondaryModule[i]){
		if(_exUpdateGraphics[i]){
			(_exUpdateGraphics[i])((void*)&info);
		}
		i++;
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


	int moduleCount = 0;
	if(!_secondaryModule[0]){
#if _WIN64
		const wchar_t relative_path[] = TEXT("..\\Bin64\\Plugins\\Wrapped\\");
#else
		const wchar_t relative_path[] = TEXT("..\\Bin32\\Plugins\\Wrapped\\");
#endif

		const wchar_t search_wildcard[] = TEXT("*.dll");


		size_t pathLen = strlen( _logPath );
        if( pathLen > 0 ){
            //Convert logPath to unicode
			size_t w_pathLen = MultiByteToWideChar(CP_ACP, 0, _logPath, -1, NULL, 0);
			const size_t fullLen = w_pathLen + 100*sizeof(wchar_t);
			wchar_t* searchPath = new wchar_t[ fullLen ];
			MultiByteToWideChar(CP_ACP, 0, _logPath, -1, searchPath, w_pathLen);

			//append and extra \ if necessary
			if( searchPath[ w_pathLen - 1 ] != L'\\' )
				_tcscat_s( searchPath, fullLen, TEXT("\\") );
			//append the rest of the relative path
			_tcscat_s( searchPath, fullLen, relative_path );

			//Create the search wildcard
			wchar_t* searchPattern = new wchar_t[fullLen];
			_tcsncpy_s(searchPattern, fullLen, searchPath, _tcslen(searchPath)+1);
			_tcscat_s(searchPattern, fullLen, search_wildcard);
			
			//Go looking for dlls
			WIN32_FIND_DATA FindFileData;
			HANDLE hFind;
			hFind = FindFirstFile(searchPattern, &FindFileData);
			if(hFind != INVALID_HANDLE_VALUE){
				do{
					wchar_t* modulePath = new wchar_t[fullLen];
					_tcsncpy_s(modulePath, fullLen, searchPath, _tcslen(searchPath)+1);
					_tcscat_s( modulePath, fullLen, FindFileData.cFileName);
				
					//Load the modules
					_secondaryModule[moduleCount] = LoadLibrary(modulePath);
					if(NULL == _secondaryModule[moduleCount]){
						_errorcode = GetLastError();
					}else{
						size_t nameLen = _tcslen(FindFileData.cFileName);
						_moduleNames[moduleCount] = new wchar_t[nameLen+1];
						_tcsncpy_s(_moduleNames[moduleCount], nameLen+1, FindFileData.cFileName, nameLen+1);
						moduleCount++;
					}
					delete [] modulePath;

				}while(FindNextFile(hFind, &FindFileData));
     			FindClose(hFind);
			}
			delete[] searchPath;
			delete[] searchPattern;
		}
	}


    // resolve function addresses here	
	int i = 0;
	while(_secondaryModule[i]){

			_exStartup[i] = (exStartup)GetProcAddress(_secondaryModule[i], "Startup");
			_exShutdown[i] = (exShutdown)GetProcAddress(_secondaryModule[i], "Shutdown");
			_exLoad[i] = (exLoad)GetProcAddress(_secondaryModule[i], "Load");
			_exUnload[i] = (exUnload)GetProcAddress(_secondaryModule[i], "Unload");
			_exStartSession[i] = (exStartSession)GetProcAddress(_secondaryModule[i], "StartSession");
			_exEndSession[i] = (exEndSession)GetProcAddress(_secondaryModule[i], "EndSession");
			_exEnterRealtime[i] = (exEnterRealtime)GetProcAddress(_secondaryModule[i], "EnterRealtime");
			_exExitRealtime[i] = (exExitRealtime)GetProcAddress(_secondaryModule[i], "ExitRealtime");

			_exUpdateScoring[i] = (exUpdateScoring)GetProcAddress(_secondaryModule[i], "UpdateScoring");
			_exUpdateTelemetry[i] = (exUpdateTelemetry)GetProcAddress(_secondaryModule[i], "UpdateTelemetry");
			_exUpdateGraphics[i] = (exUpdateGraphics)GetProcAddress(_secondaryModule[i], "UpdateGraphics");
			_exSetEnvironment[i] = (exSetEnvironment)GetProcAddress(_secondaryModule[i], "SetEnvironment");
			i++;		
	}

    //Pass along to second stage
	i=0;
	while(_secondaryModule[i]){
		if(_exSetEnvironment[i]){
			(_exSetEnvironment[i])((void*)&info);
		}
		i++;
	}
}
