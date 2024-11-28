# Find GTest package
find_package(GTest REQUIRED)

# Add test executable
add_executable(dfs_tests
    crypto_tests.cpp
    store_tests.cpp
    message_tests.cpp
    decoder_tests.cpp
    channel_tests.cpp
)

# Link libraries
target_link_libraries(dfs_tests
    PRIVATE
    dfs_lib
    GTest::GTest
    GTest::Main
    cryptopp
    pthread
)

# Add tests
include(GoogleTest)
gtest_discover_tests(dfs_tests)