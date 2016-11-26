#include "pw_error.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define PW_PLATFORM_ERROR_MESSAGE_LENGTH 200
#define PW_ERROR_QUEUE_LENGTH 8

static uint32_t g_errorCount = 0;
static uint32_t g_errorQueueHead = 0;
static uint32_t g_errorQueueTail = 0;
static PW_Error g_errorQueue[ PW_ERROR_QUEUE_LENGTH ];

uint32_t PW_GetErrorCount()
{
  return g_errorCount;
}

#define PW_PushError( ERROR_CODE, FMT, ... ) PW_PushError_(ERROR_CODE, FMT, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

static void PW_PushError_( PW_ErrorCode errorCode, const char *fmt, const char *file, uint32_t line, const char *function, ... )
{
  PW_Error *nextError = g_errorQueue + g_errorQueueTail;
  nextError->code = errorCode;
  nextError->file = file;
  nextError->line = line;
  nextError->function = function;

  va_list args;
  va_start( args, function );
  if ( vsnprintf( nextError->message, sizeof(nextError->message), fmt, args ) < 0 )
  {
    strncpy( nextError->message, "FAILED TO GENERATE ERROR MESSAGE", sizeof( nextError->message ) );
  }
  va_end( args );

  g_errorQueueTail = ( g_errorQueueTail + 1 ) % PW_ERROR_QUEUE_LENGTH;
  g_errorCount++;
}

int PW_GetError( PW_Error *error )
{
  if ( error == NULL )
  {
    return PW_ERROR_INVALID_ARGUMENT;
  }

  if ( g_errorCount > 0 )
  {
    *error = g_errorQueue[ g_errorQueueHead ];
    g_errorQueueHead = ( g_errorQueueHead + 1 ) % PW_ERROR_QUEUE_LENGTH;
    g_errorCount--;
  }
  else
  {
    memset( error, 0, sizeof( *error ) );
  }
  return PW_ERROR_NONE;
}

void PW_ClearErrors()
{
  g_errorCount = 0;
  g_errorQueueHead = 0;
  g_errorQueueTail = 0;
}

static void RemoveCarriageReturnAndNewLine( char *str )
{
  while ( *str != '\0' )
  {
    if ( *str == '\r' || *str == '\n' )
    {
      *str = '\0';
      break;
    }
    str++;
  }
}

static const char* PW_GetErrorMessageFromPlatform()
{
  static char errorMessage[ PW_PLATFORM_ERROR_MESSAGE_LENGTH + 1 ];

  DWORD errorId = GetLastError();
  if ( errorId != 0 )
  {
    LPSTR messageBuffer = NULL;
    size_t size = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                 FORMAT_MESSAGE_FROM_SYSTEM     | 
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorId, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                 ( LPSTR )&messageBuffer, 0, NULL );

    size = min( size, PW_PLATFORM_ERROR_MESSAGE_LENGTH );
    strncpy( errorMessage, messageBuffer, size );
    errorMessage[ size ] = '\0';
    RemoveCarriageReturnAndNewLine( errorMessage );
    LocalFree( messageBuffer );

    return errorMessage;
  }

  return "";
}