
#ifdef WIN32
#include "ortp-config-win32.h"
#else
#include "ortp-config.h"
#endif
#include "ortp/ortp.h"	

typedef struct __STRUCT_SHARED_DATA__
{
	DWORD				m_nReference;
	DWORD				m_dwStartTime;
	BOOL				m_bInitialize;

} SHARED_DATA, * LPSHARED_DATA;

#ifdef EXTERNAL_LOGGER
#include "logger.h"
#else
#define	RegisterLog(logVar, logString);
#define	UnregisterLog(logVar, logString);
#endif

extern DWORD dwoRTPLogLevel;

#define	SHMEMSIZE	sizeof(SHARED_DATA)

static	LPSHARED_DATA	lpSharedData;
static  HANDLE			hMapObject	 = NULL;  // handle to file mapping

BOOL WINAPI DllMain( 
					 HINSTANCE hinstDLL,	// handle to DLL module
					 DWORD fdwReason,		// reason for calling function
					 LPVOID lpReserved		// reserved
				   )  
{
	BOOL	fInit = FALSE;
	WORD	wVersionRequested;
	WSADATA wsaData;

    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:

			OutputDebugString("--> dll_entry.c - oRTP.dll - DLL_PROCESS_ATTACH()\n");
		 
			wVersionRequested = MAKEWORD( 1, 0 );

			if (WSAStartup(wVersionRequested,&wsaData)!=0) 
			{
				return FALSE;
			}

            // Create a named file mapping object. 
            hMapObject = CreateFileMapping( INVALID_HANDLE_VALUE,	// use paging file
											NULL,					// default security attributes
											PAGE_READWRITE,			// read/write access
											0,						// size: high 32-bits
											SHMEMSIZE,				// size: low 32-bits
											"oRTPSharedMemory");  // name of map object

            if (hMapObject == NULL) 
                return FALSE; 
 
            // The first process to attach initializes memory. 
            fInit = (GetLastError() != ERROR_ALREADY_EXISTS); 
 
            // Get a pointer to the file-mapped shared memory.
 
            lpSharedData = (LPSHARED_DATA) MapViewOfFile(   hMapObject,     // object to map view of
														   	FILE_MAP_WRITE, // read/write access
															0,              // high offset:  map from
															0,              // low offset:   beginning
															0);             // default: map entire file
            if (lpSharedData == NULL) 
                return FALSE; 
 
            // Initialize memory if this is the first process.
 
            if (fInit) 
			{
				OutputDebugString("--> dll_entry.c - oRTP.dll - Initializing module\n");

				lpSharedData->m_dwStartTime	= GetTickCount();
				lpSharedData->m_nReference	= 1;
				lpSharedData->m_bInitialize = FALSE;

				// Register the log
				RegisterLog(&dwoRTPLogLevel, "LOG_ORTP");
			}
			else
			{
				OutputDebugString("--> dll_entry.c - oRTP.dll - Binding\n");
				lpSharedData->m_nReference++;
			}
            break;

        case DLL_THREAD_ATTACH:

			if (lpSharedData != NULL)
			{
				if (lpSharedData->m_bInitialize == FALSE)
				{
					// Initialize oRTP
					ortp_init();

					// Start the scheduler
					//ortp_scheduler_init();

					lpSharedData->m_bInitialize = TRUE;
				}
			}
            break;

        case DLL_THREAD_DETACH:
			break;

        case DLL_PROCESS_DETACH:

			if (lpSharedData != NULL)
			{			
				OutputDebugString("--> dll_entry.c - oRTP.dll - Binding\n");
				lpSharedData->m_nReference--;

				if (lpSharedData->m_nReference == 0)
				{
					OutputDebugString("--> dll_entry.c - oRTP.dll - Detaching\n");

					ortp_exit();
					UnregisterLog(&dwoRTPLogLevel, "LOG_ORTP");


					// Unmap shared memory from the process's address space. 
					UnmapViewOfFile(lpSharedData);
					lpSharedData = NULL;
	 
					// Close the process's handle to the file-mapping object.
					CloseHandle(hMapObject); 
					hMapObject = INVALID_HANDLE_VALUE;
				}
			}
            break;
    }

    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
