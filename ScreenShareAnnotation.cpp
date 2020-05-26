#ifndef UNICODE
    #define UNICODE
#endif
#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <shellscalingapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

#pragma comment( lib, "user32.lib" )
#pragma comment( lib, "gdi32.lib" )
#pragma comment( lib, "gdiplus.lib" )
#pragma comment( lib, "msimg32.lib" )

#define WINDOW_CLASS_NAME L"SymphonyScreenShareAnnotationTool"

BOOL (WINAPI *EnableNonClientDpiScalingPtr)( HWND ) = NULL; // Dynamic bound function which does not exist on win7
HRESULT (STDAPICALLTYPE* GetDpiForMonitorPtr)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT* ) = NULL;

#include "resources/resources.h"
#include "Localization.h"
#include "MakeAnnotations.h"

// Callback for closing existing instances of the annotation tool
static BOOL CALLBACK closeExistingInstance( HWND hwnd, LPARAM lparam ) {    
    wchar_t className[ 256 ] = L"";
    GetClassNameW( hwnd, className, sizeof( className ) );

    if( wcscmp( className, WINDOW_CLASS_NAME ) == 0 ) {
        PostMessageA( hwnd, WM_CLOSE, 0, 0 );
    }

    return TRUE;
}

int main( int argc, char* argv[] ) {

    // Dynamic binding of functions not available on win 7
    HMODULE user32lib = LoadLibraryA( "user32.dll" );
    if( user32lib ) {
        EnableNonClientDpiScalingPtr = (BOOL (WINAPI*)(HWND)) GetProcAddress( user32lib, "EnableNonClientDpiScaling" );

        DPI_AWARENESS_CONTEXT (WINAPI *SetThreadDpiAwarenessContextPtr)( DPI_AWARENESS_CONTEXT ) = 
            (DPI_AWARENESS_CONTEXT (WINAPI*)(DPI_AWARENESS_CONTEXT)) 
                GetProcAddress( user32lib, "SetThreadDpiAwarenessContext" );
        
        BOOL (WINAPI *SetProcessDPIAwarePtr)(VOID) = (BOOL (WINAPI*)(VOID))GetProcAddress( user32lib, "SetProcessDPIAware" );

        // Avoid DPI scaling affecting the resolution of the grabbed snippet
        if( !SetThreadDpiAwarenessContextPtr || !SetThreadDpiAwarenessContextPtr( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE ) ) {
            if( SetProcessDPIAwarePtr ) {
                SetProcessDPIAwarePtr();
            }
        }
    }

    HMODULE shcorelib = LoadLibraryA( "Shcore.dll" );
    if( shcorelib ) {
        GetDpiForMonitorPtr = 
            (HRESULT (STDAPICALLTYPE*)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT* )) 
                GetProcAddress( shcorelib, "GetDpiForMonitor" );
    }

    
    HWND foregroundWindow = GetForegroundWindow();

    HMONITOR monitor = MonitorFromWindow( foregroundWindow, MONITOR_DEFAULTTOPRIMARY );

    // Cancel screen snippet in progress
    EnumWindows( closeExistingInstance, 0 );
/*
    // If no command line parameters, this was a request to cancel in-progress snippet tool
    if( argc < 2 ) {
        if( foregroundWindow ) {
            SetForegroundWindow( foregroundWindow );
        }
        return EXIT_SUCCESS;
    }
*/
    // Find language matching command line arg
    int lang = 0; // default to 'en-US'
    if( argc == 3 ) {
        char const* lang_str = argv[ 2 ];
        for( int i = 0; i < sizeof( localization ) / sizeof( *localization ); ++i ) {
            if( _stricmp( localization[ i ].language, lang_str ) == 0 ) {
                lang = i;
                break;
            }
        }
    }


    // Start GDI+ (used for semi-transparent drawing and anti-aliased curve drawing)
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup( &gdiplusToken, &gdiplusStartupInput, NULL );


    // Let the user annotate the screen snippet with drawings
    int result = makeAnnotations( monitor, lang );
    if( result == EXIT_SUCCESS ) {
    }

//    Gdiplus::GdiplusShutdown( gdiplusToken );
    if( foregroundWindow ) {
        SetForegroundWindow( foregroundWindow );
    }

    if( user32lib ) {
        FreeLibrary( user32lib );
    }

    if( shcorelib ) {
        FreeLibrary( shcorelib );
    }

    return EXIT_SUCCESS;
}


// pass-through so the program will build with either /SUBSYSTEM:WINDOWS or /SUBSYSTEM:CONSOLE
extern "C" int __stdcall WinMain( struct HINSTANCE__* inst, struct HINSTANCE__*, char*, int ) { 
    return main( __argc, __argv ); 
}

