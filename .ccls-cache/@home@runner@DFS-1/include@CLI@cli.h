#pragma once

#include "server/file_server.h"
#include <memory>
#include <string>
#include <vector>

namespace dfs {
namespace cli {

// CLIHandler class for interacting with the file server through command-line
// commands
class CLIHandler {
public:
  explicit CLIHandler(std::shared_ptr<server::FileServer> server);
  void start(); // Starts the CLI loop
  void stop();  // Stops the CLI and server

private:
  // Command handlers for various commands
  bool handleAddFile(const std::vector<std::string> &args);
  bool handleRemoveFile(const std::vector<std::string> &args);
  bool handleListDir(const std::vector<std::string> &args);
  bool handleChangeDir(const std::vector<std::string> &args);
  bool handleDeleteDir(const std::vector<std::string> &args);
  void handleHelp(); // Displays available commands

  // Helper methods
  std::vector<std::string>
  splitCommand(const std::string &cmd); // Splits input command into arguments
  std::string getCurrentPath() const;   // Returns the current directory path

  std::shared_ptr<server::FileServer> server_; // Reference to the file server
  std::string currentPath_; // Current working directory for the CLI
  bool running_;            // CLI running state
};

} // namespace cli
} // namespace dfs