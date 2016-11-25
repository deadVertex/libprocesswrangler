#include <greatest/greatest.h>

#include "process_wrangler.h"

/* Test cases */
TEST InitializationIsSuccessful()
{
  ASSERT_EQm( "PW_Initialize failed", 0, PW_Initialize() );
  PASS();
}

/* Test runner */
GREATEST_MAIN_DEFS();

int main( int argc, char **argv )
{
  GREATEST_MAIN_BEGIN();

  /* Run test cases and test suites */
  RUN_TEST( InitializationIsSuccessful );

  GREATEST_MAIN_END();
}