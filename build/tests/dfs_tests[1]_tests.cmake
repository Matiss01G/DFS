add_test([=[FileServerTest.StoreFileAcrossNetwork]=]  /home/runner/DFS-1/build/tests/dfs_tests [==[--gtest_filter=FileServerTest.StoreFileAcrossNetwork]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[FileServerTest.StoreFileAcrossNetwork]=]  PROPERTIES WORKING_DIRECTORY /home/runner/DFS-1/build/tests SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==])
set(  dfs_tests_TESTS FileServerTest.StoreFileAcrossNetwork)
