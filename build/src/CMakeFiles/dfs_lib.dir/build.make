# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.29

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /nix/store/q1nssraba326p2kp6627hldd2bhg254c-cmake-3.29.2/bin/cmake

# The command to remove a file.
RM = /nix/store/q1nssraba326p2kp6627hldd2bhg254c-cmake-3.29.2/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/runner/DFS-1

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/runner/DFS-1/build

# Include any dependencies generated for this target.
include src/CMakeFiles/dfs_lib.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include src/CMakeFiles/dfs_lib.dir/compiler_depend.make

# Include the progress variables for this target.
include src/CMakeFiles/dfs_lib.dir/progress.make

# Include the compile flags for this target's objects.
include src/CMakeFiles/dfs_lib.dir/flags.make

src/CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.o: src/CMakeFiles/dfs_lib.dir/flags.make
src/CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.o: /home/runner/DFS-1/src/crypto/crypto_utils.cpp
src/CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.o: src/CMakeFiles/dfs_lib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/runner/DFS-1/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object src/CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.o"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.o -MF CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.o.d -o CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.o -c /home/runner/DFS-1/src/crypto/crypto_utils.cpp

src/CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.i"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/runner/DFS-1/src/crypto/crypto_utils.cpp > CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.i

src/CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.s"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/runner/DFS-1/src/crypto/crypto_utils.cpp -o CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.s

src/CMakeFiles/dfs_lib.dir/storage/path_key.cpp.o: src/CMakeFiles/dfs_lib.dir/flags.make
src/CMakeFiles/dfs_lib.dir/storage/path_key.cpp.o: /home/runner/DFS-1/src/storage/path_key.cpp
src/CMakeFiles/dfs_lib.dir/storage/path_key.cpp.o: src/CMakeFiles/dfs_lib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/runner/DFS-1/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object src/CMakeFiles/dfs_lib.dir/storage/path_key.cpp.o"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/dfs_lib.dir/storage/path_key.cpp.o -MF CMakeFiles/dfs_lib.dir/storage/path_key.cpp.o.d -o CMakeFiles/dfs_lib.dir/storage/path_key.cpp.o -c /home/runner/DFS-1/src/storage/path_key.cpp

src/CMakeFiles/dfs_lib.dir/storage/path_key.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/dfs_lib.dir/storage/path_key.cpp.i"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/runner/DFS-1/src/storage/path_key.cpp > CMakeFiles/dfs_lib.dir/storage/path_key.cpp.i

src/CMakeFiles/dfs_lib.dir/storage/path_key.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/dfs_lib.dir/storage/path_key.cpp.s"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/runner/DFS-1/src/storage/path_key.cpp -o CMakeFiles/dfs_lib.dir/storage/path_key.cpp.s

src/CMakeFiles/dfs_lib.dir/storage/store.cpp.o: src/CMakeFiles/dfs_lib.dir/flags.make
src/CMakeFiles/dfs_lib.dir/storage/store.cpp.o: /home/runner/DFS-1/src/storage/store.cpp
src/CMakeFiles/dfs_lib.dir/storage/store.cpp.o: src/CMakeFiles/dfs_lib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/runner/DFS-1/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Building CXX object src/CMakeFiles/dfs_lib.dir/storage/store.cpp.o"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/dfs_lib.dir/storage/store.cpp.o -MF CMakeFiles/dfs_lib.dir/storage/store.cpp.o.d -o CMakeFiles/dfs_lib.dir/storage/store.cpp.o -c /home/runner/DFS-1/src/storage/store.cpp

src/CMakeFiles/dfs_lib.dir/storage/store.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/dfs_lib.dir/storage/store.cpp.i"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/runner/DFS-1/src/storage/store.cpp > CMakeFiles/dfs_lib.dir/storage/store.cpp.i

src/CMakeFiles/dfs_lib.dir/storage/store.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/dfs_lib.dir/storage/store.cpp.s"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/runner/DFS-1/src/storage/store.cpp -o CMakeFiles/dfs_lib.dir/storage/store.cpp.s

src/CMakeFiles/dfs_lib.dir/network/message.cpp.o: src/CMakeFiles/dfs_lib.dir/flags.make
src/CMakeFiles/dfs_lib.dir/network/message.cpp.o: /home/runner/DFS-1/src/network/message.cpp
src/CMakeFiles/dfs_lib.dir/network/message.cpp.o: src/CMakeFiles/dfs_lib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/runner/DFS-1/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "Building CXX object src/CMakeFiles/dfs_lib.dir/network/message.cpp.o"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/dfs_lib.dir/network/message.cpp.o -MF CMakeFiles/dfs_lib.dir/network/message.cpp.o.d -o CMakeFiles/dfs_lib.dir/network/message.cpp.o -c /home/runner/DFS-1/src/network/message.cpp

src/CMakeFiles/dfs_lib.dir/network/message.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/dfs_lib.dir/network/message.cpp.i"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/runner/DFS-1/src/network/message.cpp > CMakeFiles/dfs_lib.dir/network/message.cpp.i

src/CMakeFiles/dfs_lib.dir/network/message.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/dfs_lib.dir/network/message.cpp.s"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/runner/DFS-1/src/network/message.cpp -o CMakeFiles/dfs_lib.dir/network/message.cpp.s

src/CMakeFiles/dfs_lib.dir/network/decoder.cpp.o: src/CMakeFiles/dfs_lib.dir/flags.make
src/CMakeFiles/dfs_lib.dir/network/decoder.cpp.o: /home/runner/DFS-1/src/network/decoder.cpp
src/CMakeFiles/dfs_lib.dir/network/decoder.cpp.o: src/CMakeFiles/dfs_lib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/runner/DFS-1/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "Building CXX object src/CMakeFiles/dfs_lib.dir/network/decoder.cpp.o"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/dfs_lib.dir/network/decoder.cpp.o -MF CMakeFiles/dfs_lib.dir/network/decoder.cpp.o.d -o CMakeFiles/dfs_lib.dir/network/decoder.cpp.o -c /home/runner/DFS-1/src/network/decoder.cpp

src/CMakeFiles/dfs_lib.dir/network/decoder.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/dfs_lib.dir/network/decoder.cpp.i"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/runner/DFS-1/src/network/decoder.cpp > CMakeFiles/dfs_lib.dir/network/decoder.cpp.i

src/CMakeFiles/dfs_lib.dir/network/decoder.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/dfs_lib.dir/network/decoder.cpp.s"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/runner/DFS-1/src/network/decoder.cpp -o CMakeFiles/dfs_lib.dir/network/decoder.cpp.s

src/CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.o: src/CMakeFiles/dfs_lib.dir/flags.make
src/CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.o: /home/runner/DFS-1/src/network/tcp_peer.cpp
src/CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.o: src/CMakeFiles/dfs_lib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/runner/DFS-1/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "Building CXX object src/CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.o"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.o -MF CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.o.d -o CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.o -c /home/runner/DFS-1/src/network/tcp_peer.cpp

src/CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.i"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/runner/DFS-1/src/network/tcp_peer.cpp > CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.i

src/CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.s"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/runner/DFS-1/src/network/tcp_peer.cpp -o CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.s

src/CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.o: src/CMakeFiles/dfs_lib.dir/flags.make
src/CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.o: /home/runner/DFS-1/src/network/tcp_transport.cpp
src/CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.o: src/CMakeFiles/dfs_lib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/runner/DFS-1/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Building CXX object src/CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.o"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.o -MF CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.o.d -o CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.o -c /home/runner/DFS-1/src/network/tcp_transport.cpp

src/CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.i"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/runner/DFS-1/src/network/tcp_transport.cpp > CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.i

src/CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.s"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/runner/DFS-1/src/network/tcp_transport.cpp -o CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.s

src/CMakeFiles/dfs_lib.dir/server/file_server.cpp.o: src/CMakeFiles/dfs_lib.dir/flags.make
src/CMakeFiles/dfs_lib.dir/server/file_server.cpp.o: /home/runner/DFS-1/src/server/file_server.cpp
src/CMakeFiles/dfs_lib.dir/server/file_server.cpp.o: src/CMakeFiles/dfs_lib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --progress-dir=/home/runner/DFS-1/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Building CXX object src/CMakeFiles/dfs_lib.dir/server/file_server.cpp.o"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT src/CMakeFiles/dfs_lib.dir/server/file_server.cpp.o -MF CMakeFiles/dfs_lib.dir/server/file_server.cpp.o.d -o CMakeFiles/dfs_lib.dir/server/file_server.cpp.o -c /home/runner/DFS-1/src/server/file_server.cpp

src/CMakeFiles/dfs_lib.dir/server/file_server.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Preprocessing CXX source to CMakeFiles/dfs_lib.dir/server/file_server.cpp.i"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/runner/DFS-1/src/server/file_server.cpp > CMakeFiles/dfs_lib.dir/server/file_server.cpp.i

src/CMakeFiles/dfs_lib.dir/server/file_server.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green "Compiling CXX source to assembly CMakeFiles/dfs_lib.dir/server/file_server.cpp.s"
	cd /home/runner/DFS-1/build/src && /nix/store/9bv7dcvmfcjnmg5mnqwqlq2wxfn8d7yi-gcc-wrapper-13.2.0/bin/g++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/runner/DFS-1/src/server/file_server.cpp -o CMakeFiles/dfs_lib.dir/server/file_server.cpp.s

# Object files for target dfs_lib
dfs_lib_OBJECTS = \
"CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.o" \
"CMakeFiles/dfs_lib.dir/storage/path_key.cpp.o" \
"CMakeFiles/dfs_lib.dir/storage/store.cpp.o" \
"CMakeFiles/dfs_lib.dir/network/message.cpp.o" \
"CMakeFiles/dfs_lib.dir/network/decoder.cpp.o" \
"CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.o" \
"CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.o" \
"CMakeFiles/dfs_lib.dir/server/file_server.cpp.o"

# External object files for target dfs_lib
dfs_lib_EXTERNAL_OBJECTS =

src/libdfs_lib.a: src/CMakeFiles/dfs_lib.dir/crypto/crypto_utils.cpp.o
src/libdfs_lib.a: src/CMakeFiles/dfs_lib.dir/storage/path_key.cpp.o
src/libdfs_lib.a: src/CMakeFiles/dfs_lib.dir/storage/store.cpp.o
src/libdfs_lib.a: src/CMakeFiles/dfs_lib.dir/network/message.cpp.o
src/libdfs_lib.a: src/CMakeFiles/dfs_lib.dir/network/decoder.cpp.o
src/libdfs_lib.a: src/CMakeFiles/dfs_lib.dir/network/tcp_peer.cpp.o
src/libdfs_lib.a: src/CMakeFiles/dfs_lib.dir/network/tcp_transport.cpp.o
src/libdfs_lib.a: src/CMakeFiles/dfs_lib.dir/server/file_server.cpp.o
src/libdfs_lib.a: src/CMakeFiles/dfs_lib.dir/build.make
src/libdfs_lib.a: src/CMakeFiles/dfs_lib.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color "--switch=$(COLOR)" --green --bold --progress-dir=/home/runner/DFS-1/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_9) "Linking CXX static library libdfs_lib.a"
	cd /home/runner/DFS-1/build/src && $(CMAKE_COMMAND) -P CMakeFiles/dfs_lib.dir/cmake_clean_target.cmake
	cd /home/runner/DFS-1/build/src && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/dfs_lib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
src/CMakeFiles/dfs_lib.dir/build: src/libdfs_lib.a
.PHONY : src/CMakeFiles/dfs_lib.dir/build

src/CMakeFiles/dfs_lib.dir/clean:
	cd /home/runner/DFS-1/build/src && $(CMAKE_COMMAND) -P CMakeFiles/dfs_lib.dir/cmake_clean.cmake
.PHONY : src/CMakeFiles/dfs_lib.dir/clean

src/CMakeFiles/dfs_lib.dir/depend:
	cd /home/runner/DFS-1/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/runner/DFS-1 /home/runner/DFS-1/src /home/runner/DFS-1/build /home/runner/DFS-1/build/src /home/runner/DFS-1/build/src/CMakeFiles/dfs_lib.dir/DependInfo.cmake "--color=$(COLOR)"
.PHONY : src/CMakeFiles/dfs_lib.dir/depend

