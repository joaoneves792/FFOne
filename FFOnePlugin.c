//
// Created by joao on 9/30/18.
//

#include "PluginObjects.h"
#include "InternalsPlugin.h"
#include "FFOnePlugin.h"
#include <cstdio>                  // for logging

// plugin information



extern "C" __declspec( dllexport )
const char * __cdecl GetPluginName()                   { return( "FFOne - 2008.02.13" ); }

extern "C" __declspec( dllexport )
PluginObjectType __cdecl GetPluginType()               { return( PO_INTERNALS ); }

extern "C" __declspec( dllexport )
int __cdecl GetPluginVersion()                         { return( 7 ); }  // InternalsPluginV07 functionality (if you change this return value, you must derive from the appropriate class!)

extern "C" __declspec( dllexport )
PluginObject * __cdecl CreatePluginObject()            { return( static_cast< PluginObject * >( new FFOnePlugin ) ); }

extern "C" __declspec( dllexport )
void __cdecl DestroyPluginObject( PluginObject *obj )  { delete( static_cast< FFOnePlugin * >( obj ) ); }

/*
FFOnePlugin::FFOnePlugin() {
   mLogging = true;
   mLogFile = NULL;
   mLogPath = NULL;
}

FFOnePlugin::~FFOnePlugin() {

}
void FFOnePlugin::SetEnvironment(const EnvironmentInfoV01 &info) {
    // If we already have it, but it's wrong, delete it.
    if( mLogPath != NULL )
    {
        if( 0 != _stricmp( mLogPath, info.mPath[ 0 ] ) )
        {
            delete [] mLogPath;
            mLogPath = NULL;
        }
    }

    // If we don't already have it now, then copy it.
    if( mLogPath == NULL )
    {
        const size_t pathLen = strlen( info.mPath[ 0 ] ) + 1;
        mLogPath = new char[ pathLen ];
        strcpy_s( mLogPath, pathLen, info.mPath[ 0 ] );
    }
}

void FFOnePlugin::StartSession() {
    // open log file if desired (and not already open)
    if( mLogging && ( mLogFile == NULL ) && ( mLogPath != NULL ) )
    {
        const size_t pathLen = strlen( mLogPath );
        if( pathLen > 0 )
        {
            const size_t fullLen = pathLen + 50;
            char *fullpath = new char[ fullLen ];
            long count = 0;
            while( true )
            {
                strcpy_s( fullpath, fullLen, mLogPath );
                if( fullpath[ pathLen - 1 ] != '\\' )
                    strcat_s( fullpath, fullLen, "\\" );
                strcat_s( fullpath, fullLen, "Log\\FFOnePluginLog" );

                // tag.2016.02.26 - people never remember to save this file off, so just keep creating new ones ...
                char numb[ 64 ];
                sprintf_s( numb, "%d.txt", count++ );
                strcat_s( fullpath, fullLen, numb );

                fopen_s( &mLogFile, fullpath, "r" );
                if( mLogFile == NULL )
                {
                    fopen_s( &mLogFile, fullpath, "w" );
                    break;
                }
                fclose( mLogFile );
                mLogFile = NULL;
            }

            delete [] fullpath;
        }
    }

    if( mLogFile != NULL )
        fprintf( mLogFile, "\n--- NEW FFONE SESSION STARTING ---\n" );
}

void FFOnePlugin::EndSession(){

}
 */