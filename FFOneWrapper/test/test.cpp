// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <Windows.h>
#include <tchar.h>

int _tmain(int argc, _TCHAR* argv[]){
	const wchar_t relative_path[] = TEXT("..\\FFOneWrapper\\Release\\");
	const wchar_t search_wildcard[] = TEXT("*.dll");
	const char _logPath[] = "C:\\Users\\joao\\Documents\\Visual Studio 2012\\Projects\\FFOneWrapper";
	
	size_t pathLen = strlen( _logPath );
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
	_tprintf(TEXT("searchPath: %s\n"), searchPath);
	
	//Create the search wildcard
	wchar_t* searchPattern = new wchar_t[fullLen];
	_tcsncpy_s(searchPattern, fullLen, searchPath, _tcslen(searchPath)+1);
	_tcscat_s(searchPattern, fullLen, search_wildcard);
	_tprintf(TEXT("searchPattern: %s\n"), searchPattern);
			
	//Go looking for dlls
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
	hFind = FindFirstFile(searchPattern, &FindFileData);
	if(hFind != INVALID_HANDLE_VALUE){
		do{
			_tprintf(TEXT("\n%s\n"), FindFileData.cFileName);
			wchar_t* modulePath = new wchar_t[fullLen];
			_tcsncpy_s(modulePath, fullLen, searchPath, _tcslen(searchPath)+1);
			//_tcscat_s( modulePath, fullLen, searchPath );
			_tcscat_s( modulePath, fullLen, FindFileData.cFileName);
			_tprintf(TEXT("\n%s\n"), modulePath);
			delete [] modulePath;

		}while(FindNextFile(hFind, &FindFileData));
     	FindClose(hFind);
	}
	delete[] searchPath;
	delete[] searchPattern;
	
	int i;
	scanf_s("%d", &i);
	return 0;
}

