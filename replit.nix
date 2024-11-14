{ pkgs }: {
	deps = [
   pkgs.cmake
   pkgs.gtest
   pkgs.cryptopp
   pkgs.boost
		pkgs.clang
		pkgs.ccls
		pkgs.gdb
		pkgs.gnumake
	];
}