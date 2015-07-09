// sample.cpp : Defines the entry point for the DLL application.
//

#if defined (_WIN32)
#include "StdAfx.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}
#endif

