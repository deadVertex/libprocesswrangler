cmake_minimum_required( VERSION 2.8 )
project( libprocesswrangler )

include_directories( "src" "include" "thirdparty" )

if( MSVC )
  add_executable( unit_tests "test/main.c" )
  add_library( libprocesswrangler SHARED "src/process_wrangler.c" )
  target_link_libraries( libprocesswrangler Pdh )
  target_link_libraries( unit_tests Pdh )
endif()
