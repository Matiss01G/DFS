// #include "cli/cli_handler.h"
// #include "server/file_server.h"
// #include <iostream>
// #include <memory>
// #include <vector>

// // Function to initialize a FileServer and its CLI (if needed)
// std::shared_ptr<dfs::server::FileServer>
// initializeFileServer(const std::string &id, const std::string &storageRoot,
//                      bool setupCLI = false) {
//   // Create server options
//   dfs::server::FileServerOpts opts;
//   opts.id = id;                   // Unique identifier for this server
//   opts.storageRoot = storageRoot; // Root directory for file storage

//   // Create and start the file server
//   auto server = std::make_shared<dfs::server::FileServer>(opts);
//   if (!server->Start()) {
//     std::cerr << "Failed to start FileServer: " << id << "\n";
//     return nullptr;
//   }

//   // Optionally set up a CLI
//   if (setupCLI) {
//     dfs::cli::CLIHandler cli(server);
//     cli.start(); // Start CLI for user interaction
//   }

//   std::cout << "Started FileServer: " << id << "\n";
//   return server;
// }

// // Function to setup bootstrap nodes and their corresponding file servers
// void initializeEnvironment(const std::string &storageRoot) {
//   // Define the bootstrap nodes and their properties
//   std::vector<std::string> bootstrapAddresses = {
//       "localhost:8001", "localhost:8002", "localhost:8003"};

//   // Initialize dummy bootstrap nodes
//   for (const auto &address : bootstrapAddresses) {
//     initializeFileServer("bootstrap-" + address, storageRoot);
//   }
// }

// int main(int argc, char *argv[]) {
//   // Define the storage root directory
//   std::string storageRoot = "./storage";

//   // Initialize the environment (dummy bootstrap nodes)
//   initializeEnvironment(storageRoot);

//   // Initialize the actual FileServer we will interact with
//   auto mainServer = initializeFileServer("main-server", storageRoot, true);

//   if (mainServer) {
//     // Create and start CLI for user interaction
//     dfs::cli::CLIHandler cli(mainServer);
//     cli.start(); // Start CLI loop for input commands
//   }

//   return 0; // Successful exit
// }

#include <iostream>

int main() {
    std::cout << "DFS Server starting..." << std::endl;
    return 0;
}