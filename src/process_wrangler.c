#include "process_wrangler.h"

#include <stdint.h>

#include "pw_error.c"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <Pdh.h>

#define PWI_MAX_CORES 256

typedef struct 
{
  PDH_HQUERY cpuQuery;
  PDH_HCOUNTER cpuUsageCounter;
  PDH_HCOUNTER coreUsageCounter[ PWI_MAX_CORES ];

  uint32_t numCores;
  uint64_t totalPhysicalMemory;
  uint64_t usedPhysicalMemory;
  double cpuUsage;
  double coreUsage[ PWI_MAX_CORES ];
} SystemInfo;

typedef struct
{
  uint64_t workingSetSize;
  HANDLE handle;
  uint32_t id;
  uint32_t numThreads;
  char name[ PW_PROCESS_NAME_LENGTH ];
} PW_ProcessInfo;

static SystemInfo g_systemInfo;
static PW_ProcessInfo g_processes[ PW_MAX_PROCESSES ];
static uint32_t g_numProcesses;

static int InitializeSystemInfo();

int PW_Initialize( void )
{
  InitializeSystemInfo();
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

// http://www.drdobbs.com/a-safer-alternative-to-terminateprocess/
static BOOL SafeTerminateProcess( HANDLE processHandle, UINT exitCode)
{
  DWORD threadId, currentExitCode;
  HANDLE duplicatedProcessHandle = INVALID_HANDLE_VALUE;
  HANDLE threadHandle = NULL;
  HINSTANCE kernelHandle = GetModuleHandle( "Kernel32" );
  BOOL result = FALSE;

  BOOL wasDuplicationSuccessful = DuplicateHandle( GetCurrentProcess(),
                                                   processHandle,
                                                   GetCurrentProcess(),
                                                   &duplicatedProcessHandle,
                                                   PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION,
                                                   FALSE,
                                                   0 );
  if ( wasDuplicationSuccessful )
  {
    if ( GetExitCodeProcess( duplicatedProcessHandle, &currentExitCode ) )
    {
      if ( currentExitCode == STILL_ACTIVE )
      {
        FARPROC exitFunction;
        exitFunction = GetProcAddress( kernelHandle, "ExitProcess" );
        if ( exitFunction )
        {
          threadHandle = CreateRemoteThread( duplicatedProcessHandle, NULL, 0, ( LPTHREAD_START_ROUTINE )exitFunction, ( PVOID )exitCode, 0, &threadId );
          if ( threadHandle )
          {
            // TODO: Fallback to TerminateProcess after period of time
            WaitForSingleObject( duplicatedProcessHandle, INFINITE );
            CloseHandle( threadHandle );
            result = TRUE;
          }
          else
          {
            PW_PushError( PW_ERROR_INTERNAL, "CreateRemoteThread failed. %s", PW_GetErrorMessageFromPlatform() );
          }
        }
        else
        {
          PW_PushError( PW_ERROR_INTERNAL, "GetProcessAddress failed. %s", PW_GetErrorMessageFromPlatform() );
        }
      }
    }
    else
    {
      PW_PushError( PW_ERROR_INTERNAL, "GetExitCodeProcess failed. %s", PW_GetErrorMessageFromPlatform() );
    }
    CloseHandle( duplicatedProcessHandle );
  }
  else
  {
    PW_PushError( PW_ERROR_INTERNAL, "Failed to duplicate process. %s", PW_GetErrorMessageFromPlatform() );
  }
  return result;
}

int PW_KillProcesses( uint32_t *processIds, uint32_t count )
{
  int found = 0;
  int result = 0;
  for ( uint32_t pidIdx = 0; pidIdx < count; ++pidIdx )
  {
    found = 0;
    for ( uint32_t processIdx = 0; processIdx < g_numProcesses; ++processIdx )
    {
      if ( g_processes[ processIdx ].id == processIds[ pidIdx ] )
      {
        found = 1;
        if ( SafeTerminateProcess( g_processes[ processIdx ].handle, 0 ) )
        {
          result++;
        }
        else
        {
          PW_PushError( PW_ERROR_INTERNAL, "Failed to safely terminate process %d.", processIds[ pidIdx ] );
        }
        break;
      }
    }
    if ( !found )
    {
      PW_PushError( PW_ERROR_INVALID_ARGUMENT, "Unable to find process with PID %d.", processIds[ pidIdx ] );
    }
  }

  return result;
}

static void DeinitStats()
{
  PdhCloseQuery( g_systemInfo.cpuQuery );
  for ( uint32_t i = 0; i < g_systemInfo.numCores; ++i )
  {
    PdhCloseQuery( g_systemInfo.coreUsageCounter[ i ] );
  }
}

static int InitializeSystemInfo()
{
  SYSTEM_INFO systemInfo;
  GetSystemInfo( &systemInfo );
  g_systemInfo.numCores = systemInfo.dwNumberOfProcessors;

  // TODO: Check for failure
  PdhOpenQuery( NULL, NULL, &g_systemInfo.cpuQuery );
  // TODO: Handle localization issues
  if ( PdhAddCounter( g_systemInfo.cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &g_systemInfo.cpuUsageCounter ) != ERROR_SUCCESS )
  {
    PW_PushError( PW_ERROR_INTERNAL, "Failed to add counter for CPU usage");
    DeinitStats();
    return PW_ERROR_INTERNAL;
  }
  for ( uint32_t i = 0; i < g_systemInfo.numCores; ++i )
  {
    char buffer[ 120 ];
    sprintf( buffer, "\\Processor(%u)\\%% Processor Time", i );
    if ( PdhAddCounter( g_systemInfo.cpuQuery, buffer, NULL, &g_systemInfo.coreUsageCounter[ i ] ) != ERROR_SUCCESS )
    {
      PW_PushError( PW_ERROR_INTERNAL, "Failed to add counter for core %u", i );
      DeinitStats();
      return PW_ERROR_INTERNAL;
    }
  }
  PdhCollectQueryData( g_systemInfo.cpuQuery );

  return PW_ERROR_NONE;
}

static void UpdateSystemInfo()
{
  MEMORYSTATUSEX memInfo;
  memInfo.dwLength = sizeof( MEMORYSTATUSEX );
  GlobalMemoryStatusEx( &memInfo );
  g_systemInfo.totalPhysicalMemory = memInfo.ullTotalPhys;
  g_systemInfo.usedPhysicalMemory = memInfo.ullTotalPhys - memInfo.ullAvailPhys;

  // Apparently we should wait a minimum of 1 second between Pdh samples
  PDH_FMT_COUNTERVALUE value;
  PdhCollectQueryData( g_systemInfo.cpuQuery );
  PdhGetFormattedCounterValue( g_systemInfo.cpuUsageCounter, PDH_FMT_DOUBLE, NULL, &value );
  g_systemInfo.cpuUsage = value.doubleValue;
  for ( uint32_t i = 0; i < g_systemInfo.numCores; ++i )
  {
    PDH_FMT_COUNTERVALUE coreValue;
    PdhGetFormattedCounterValue( g_systemInfo.coreUsageCounter[ i ], PDH_FMT_DOUBLE, NULL, &coreValue );
    g_systemInfo.coreUsage[ i ] = coreValue.doubleValue;
  }
}

int PW_GetSystemInfo( PW_SystemInfo *systemInfo )
{
  int result = PW_ERROR_NONE;
  if ( systemInfo )
  {
    UpdateSystemInfo();
    systemInfo->cpuUsage = g_systemInfo.cpuUsage;
    systemInfo->numCores = g_systemInfo.numCores;
    systemInfo->totalPhysicalMemory = g_systemInfo.totalPhysicalMemory;
    systemInfo->usedPhysicalMemory = g_systemInfo.usedPhysicalMemory;
  }
  else
  {
    PW_PushError( PW_ERROR_INVALID_ARGUMENT, "Pointer to systemInfo is NULL." );
    result = PW_ERROR_INVALID_ARGUMENT;
  }
  return result;
}
