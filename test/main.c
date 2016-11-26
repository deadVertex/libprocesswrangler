#include <greatest/greatest.h>

#include "process_wrangler.h"

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

  GREATEST_MAIN_END();
}