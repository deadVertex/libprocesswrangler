#include <greatest/greatest.h>

#include "process_wrangler.h"
#include "process_wrangler.c"

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

TEST GetSystemNumCoresAndMemory()
{
  PW_SystemInfo systemInfo;
  ASSERT_EQm( "Failed to retrieve system info", PW_ERROR_NONE, PW_GetSystemInfo( &systemInfo ) );
  ASSERT( systemInfo.numCores > 0 );
  ASSERT( systemInfo.totalPhysicalMemory > 0 );
  ASSERT( systemInfo.usedPhysicalMemory > 0 );
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
  RUN_TEST( GetSystemNumCoresAndMemory );

  GREATEST_MAIN_END();
}