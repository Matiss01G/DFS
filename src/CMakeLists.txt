# Add all source files
add_library(dfs_lib
    crypto/crypto_utils.cpp
    storage/path_key.cpp
    storage/store.cpp
    network/message.cpp
    network/decoder.cpp
    network/tcp_peer.cpp
    network/tcp_transport.cpp
    server/file_server.cpp
)

# Find required Boost components
find_package(Boost REQUIRED COMPONENTS 
    system
    log
    log_setup
    thread
)

# Link libraries
target_link_libraries(dfs_lib
    PUBLIC
    Boost::system
    Boost::log
    Boost::log_setup
    Boost::thread
    cryptopp
    pthread
)

# Add Boost.Log dynamic linking definition
target_compile_definitions(dfs_lib 
    PUBLIC
    BOOST_LOG_DYN_LINK
)