# Add all source files
add_library(dfs_lib
    crypto/crypto_utils.cpp
    storage/path_key.cpp
    storage/store.cpp
    network/message.cpp
    network/decoder.cpp
)

# Link libraries
target_link_libraries(dfs_lib
    PUBLIC
    Boost::system
    cryptopp
    pthread
)

