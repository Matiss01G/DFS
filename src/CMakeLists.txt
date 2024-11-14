# Add all source files
add_library(dfs_lib
    crypto/crypto_utils.cpp
    network/tcp_peer.cpp
    network/tcp_transport.cpp
    network/message.cpp
    network/decoder.cpp
    storage/path_key.cpp
    storage/store.cpp
    server/file_server.cpp
)

# Link libraries
target_link_libraries(dfs_lib
    PUBLIC
    Boost::system
    cryptopp
    pthread
)