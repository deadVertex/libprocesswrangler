#include <greatest/greatest.h>

#include "process_wrangler.h"

#include <Windows.h>

/* Test cases */
TEST InitializationIsSuccessful()
{
  ASSERT_EQm( "PW_Initialize failed", 0, PW_Initialize() );
  PASS();
}

TEST UpdateProcessListReturnsNumberOfProcessesInList()
{
  ASSERT( PW_UpdateProcessList() > 0 );
  PASS();
}

TEST GetProcessListReturnsTheNumberOfProcessesRetrieved()
{
  int count = PW_UpdateProcessList();
  PW_Process *processes = ( PW_Process* )malloc( sizeof( PW_Process )* count );
  ASSERT_EQ( count, PW_GetProcessList( processes, count ) );
  free( processes );

  PW_Process process;
  ASSERT_EQ( 1, PW_GetProcessList( &process, 1 ) );
  PASS();
}

TEST ClearProcessList()
{
  int count = PW_UpdateProcessList();
  PW_Process *processes = ( PW_Process* )malloc( sizeof( PW_Process )* count );
  PW_ClearProcessList();
  ASSERT_EQ( 0, PW_GetProcessList( processes, count ) );
  free( processes );
  PASS();
}

#define PW_ERROR_MESSAGE_LENGTH 320
#define PW_PLATFORM_ERROR_MESSAGE_LENGTH 200
#define PW_ERROR_QUEUE_LENGTH 8

typedef struct
{
  int code;
  uint32_t line;
  char *file;
  char *function;
  char message[ PW_ERROR_MESSAGE_LENGTH ];
} PW_Error;

enum
{
  PW_ERROR_NONE = 0,
  PW_ERROR_INVALID_ARGUMENT = 1,
  PW_ERROR_INTERNAL,
};

static const char* PW_GetErrorMessageFromPlatform()
{
  static char errorMessage[ PW_PLATFORM_ERROR_MESSAGE_LENGTH ];

  DWORD errorId = GetLastError();
  if ( errorId != 0 )
  {
    LPSTR messageBuffer = NULL;
    size_t size = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                                 FORMAT_MESSAGE_FROM_SYSTEM     | 
                                 FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorId, MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                                 ( LPSTR )&messageBuffer, 0, NULL );

    strncpy( errorMessage, messageBuffer, min(size, PW_PLATFORM_ERROR_MESSAGE_LENGTH) );
    LocalFree( messageBuffer );

    return errorMessage;
  }

  return "";
}

static uint32_t g_errorCount = 0;
static uint32_t g_errorQueueHead = 0;
static uint32_t g_errorQueueTail = 0;
static PW_Error g_errorQueue[ PW_ERROR_QUEUE_LENGTH ];

uint32_t PW_GetErrorCount()
{
  return g_errorCount;
}

void PW_PushError( int errorCode, const char *message )
{
  PW_Error *nextError = g_errorQueue + g_errorQueueTail;
  nextError->code = errorCode;
  strncpy( nextError->message, message, sizeof( nextError->message ) );

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

TEST ErrorCountIsZeroIfNoErrorHasOccurred()
{
  ASSERT_EQ( 0, PW_GetErrorCount() );
  PASS();
}

TEST PushErrorIncreasesErrorCount()
{
  PW_PushError( PW_ERROR_INTERNAL, "Failed to do something" );
  ASSERT_EQ( 1, PW_GetErrorCount() );
  PW_ClearErrors();
  PASS();
}

TEST ClearErrorsResetsErrorCount()
{
  PW_PushError( PW_ERROR_INTERNAL, "Failed to do something" );
  ASSERT( PW_GetErrorCount() > 0 );
  PW_ClearErrors();
  ASSERT_EQ( 0, PW_GetErrorCount() );
  PASS();
}

TEST ErrorContainsErrorCode()
{
  PW_PushError( PW_ERROR_INTERNAL, "Failed to do something" );
  PW_Error error;
  PW_GetError( &error );
  ASSERT_EQ( PW_ERROR_INTERNAL, error.code );
  PASS();
}

TEST ErrorContainsErrorMessage()
{
  char *errorMessage = "Something really really bad happened";
  PW_PushError( PW_ERROR_INTERNAL, errorMessage );
  PW_Error error;
  PW_GetError( &error );
  ASSERT_STR_EQ( errorMessage, error.message );
  PASS();
}

TEST GetErrorReturnsInvalidParameterErrorCodeIfArgumentIsNullPointer()
{
  ASSERT_EQ( PW_ERROR_INVALID_ARGUMENT, PW_GetError( NULL ) );
  PASS();
}

TEST GetErrorDecrementsErrorCount()
{
  PW_PushError( PW_ERROR_INTERNAL, "Some error" );
  uint32_t errorCount = PW_GetErrorCount();
  PW_Error error;
  PW_GetError( &error );
  ASSERT_EQ( errorCount - 1, PW_GetErrorCount() );
  PASS();
}

TEST GetErrorReturnsErrorsInOrderTheyWereRaised()
{
  PW_PushError( 1, "First error" );
  PW_PushError( 2, "Second error" );

  PW_Error error;
  PW_GetError( &error );
  ASSERT_EQ_FMT( 1, error.code, "%d" );
  PW_GetError( &error );
  ASSERT_EQ_FMT( 2, error.code, "%d" );
  PASS();
}

/* Test runner */
GREATEST_MAIN_DEFS();

int main( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN();

  /* Run test cases and test suites */
  RUN_TEST( InitializationIsSuccessful );
  RUN_TEST( UpdateProcessListReturnsNumberOfProcessesInList );
  RUN_TEST( GetProcessListReturnsTheNumberOfProcessesRetrieved );
  RUN_TEST( ClearProcessList );
  RUN_TEST( ErrorCountIsZeroIfNoErrorHasOccurred );
  RUN_TEST( PushErrorIncreasesErrorCount );
  RUN_TEST( ClearErrorsResetsErrorCount );
  RUN_TEST( ErrorContainsErrorCode );
  RUN_TEST( GetErrorReturnsInvalidParameterErrorCodeIfArgumentIsNullPointer );
  RUN_TEST( ErrorContainsErrorMessage );
  RUN_TEST( GetErrorDecrementsErrorCount );
  RUN_TEST( GetErrorReturnsErrorsInOrderTheyWereRaised );

  GREATEST_MAIN_END();
}