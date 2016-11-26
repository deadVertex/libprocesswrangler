#include "process_wrangler.h"

#include <stdint.h>

#include "pw_error.c"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>

typedef struct 
{
  uint64_t workingSetSize;
  HANDLE handle;
  uint32_t id;
  uint32_t numThreads;
  char name[ PW_PROCESS_NAME_LENGTH ];
} PW_ProcessInfo;

static PW_ProcessInfo g_processes[ PW_MAX_PROCESSES ];
static uint32_t g_numProcesses;

int PW_Initialize(void)
{
  return 0;
}

static void DestroyProcessInfo( PW_ProcessInfo *processInfo )
{
  if ( processInfo->handle != NULL )
  {
    if ( CloseHandle( processInfo->handle ) == FALSE )
    {
      PW_PushError( PW_ERROR_INTERNAL, "CloseHandle failed. %s", PW_GetErrorMessageFromPlatform() );
    }
  }
}

static void InitializeProcessInfo( PW_ProcessInfo *processInfo, PROCESSENTRY32 *processEntry )
{
  memset( processInfo, 0, sizeof( *processInfo ) );
  processInfo->id = processEntry->th32ProcessID;
  processInfo->numThreads = processEntry->cntThreads;
  strncpy( processInfo->name, processEntry->szExeFile, sizeof( processInfo->name ) );
  if ( processInfo->id != 0 )
  {
    processInfo->handle = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_DUP_HANDLE, FALSE, processInfo->id );
    if ( processInfo->handle != NULL )
    {
      PROCESS_MEMORY_COUNTERS_EX memoryCounters;
      // TODO: Check for error
      GetProcessMemoryInfo( processInfo->handle, &memoryCounters, sizeof( memoryCounters ) );
      processInfo->workingSetSize = memoryCounters.WorkingSetSize;
    }
    else
    {
      // Most likely a system process that we don't have access to
    }
  }
}

int PW_UpdateProcessList( void )
{
  // TODO: Clear process list
  PW_ClearProcessList();

  int count = 0;
  HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
  if ( snapshot == INVALID_HANDLE_VALUE )
  {
    PW_PushError( PW_ERROR_INTERNAL, "CreateToolhelp32Snapshot failed. %s", PW_GetErrorMessageFromPlatform() );
    return -1;
  }
  PROCESSENTRY32 processEntry;
  memset( &processEntry, 0, sizeof( processEntry ) );
  processEntry.dwSize = sizeof( processEntry );

  if ( Process32First( snapshot, &processEntry ) == TRUE )
  {
    InitializeProcessInfo( &g_processes[ count ], &processEntry );
    count++;
    while ( Process32Next( snapshot, &processEntry ) == TRUE )
    {
      if ( count == PW_MAX_PROCESSES )
      {
        //SetErrorMessage( "Too many processes" );
        break;
      }
      InitializeProcessInfo( g_processes + count, &processEntry );
      count++;
    }
  }
  CloseHandle( snapshot );

  g_numProcesses = count;

  // Returns the number of processes in the list or -1 on error.
  return count;
}

// TODO: UTF8 support
int PW_GetProcessList( PW_Process *processes, uint32_t count )
{
  count = min( count, g_numProcesses );
  for ( uint32_t i = 0; i < count; ++i )
  {
    processes[ i ].id = g_processes[ i ].id;
    strncpy( processes[ i ].name, g_processes[ i ].name, sizeof( processes[ i ].name ) );
    processes[ i ].numThreads = g_processes[ i ].numThreads;
    processes[ i ].workingSetSize = g_processes[ i ].workingSetSize;
  }

  return count;
}

void PW_ClearProcessList()
{
  for ( uint32_t i = 0; i < g_numProcesses; ++i )
  {
    DestroyProcessInfo( g_processes + i );
  }
  g_numProcesses = 0;
}

