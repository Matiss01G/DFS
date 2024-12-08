#include "cli/cli_handler.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace dfs {
namespace cli {

// Constructor to initialize CLIHandler with a file server reference
CLIHandler::CLIHandler(std::shared_ptr<server::FileServer> server)
    : server_(server), currentPath_("/"), running_(false) {}

void CLIHandler::start() {
  running_ = true; // Set running state to true
  std::cout << "DFS CLI Started. Type 'help' for commands.\n";

  while (running_) {
    // Prompt for user input
    std::cout << "dfs:" << getCurrentPath() << "$ ";

    std::string input;
    std::getline(std::cin, input); // Get user input

    auto args = splitCommand(input); // Split input into command arguments
    if (args.empty())
      continue; // Skip empty inputs

    const auto &cmd = args[0]; // Get the command

    // Process commands
    if (cmd == "add") {
      handleAddFile(args);
    } else if (cmd == "rm") {
      handleRemoveFile(args);
    } else if (cmd == "ls") {
      handleListDir(args);
    } else if (cmd == "cd") {
      handleChangeDir(args);
    } else if (cmd == "rmdir") {
      handleDeleteDir(args);
    } else if (cmd == "help") {
      handleHelp(); // Show help
    } else if (cmd == "exit") {
      stop(); // Exit CLI
    } else {
      std::cout << "Unknown command. Type 'help' for available commands.\n";
    }
  }
}

void CLIHandler::stop() {
  running_ = false; // Set running state to false
  server_->Stop();  // Stop the file server
}

bool CLIHandler::handleAddFile(const std::vector<std::string> &args) {
  // Ensure correct usage
  if (args.size() < 2) {
    std::cout << "Usage: add <file_path>\n";
    return false;
  }

  std::ifstream file(args[1], std::ios::binary); // Open the file in binary mode
  if (!file) {
    std::cout << "Error: Cannot open file " << args[1] << "\n";
    return false; // Exit if the file could not be opened
  }

  std::string key =
      currentPath_ + "/" +
      std::filesystem::path(args[1]).filename().string(); // Create key for file
  if (server_->Store(key, file)) { // Attempt to store the file
    std::cout << "File added successfully\n";
    return true;
  }

  std::cout << "Error adding file\n";
  return false;
}

bool CLIHandler::handleListDir(const std::vector<std::string> &args) {
  // Note: This would need to be implemented in the FileServer class
  // to actually list files in the DFS
  std::cout << "Listing directory " << currentPath_ << "\n";
  return true;
}

bool CLIHandler::handleChangeDir(const std::vector<std::string> &args) {
  // Ensure correct usage
  if (args.size() < 2) {
    std::cout << "Usage: cd <directory>\n";
    return false;
  }

  // Handle changing to parent directory
  if (args[1] == "..") {
    auto path = std::filesystem::path(currentPath_);
    currentPath_ = path.parent_path().string();
    if (currentPath_.empty())
      currentPath_ = "/";         // Reset to root if empty
  } else if (args[1][0] == '/') { // Absolute path
    currentPath_ = args[1];
  } else { // Relative path
    currentPath_ += "/" + args[1];
  }

  return true;
}

std::vector<std::string> CLIHandler::splitCommand(const std::string &cmd) {
  std::vector<std::string> args;
  std::stringstream ss(cmd); // Stream for splitting command
  std::string arg;

  // Split command by whitespace
  while (ss >> arg) {
    args.push_back(arg);
  }

  return args;
}

std::string CLIHandler::getCurrentPath() const {
  return currentPath_; // Return current path
}

void CLIHandler::handleHelp() {
  // Display available commands
  std::cout << "Available commands:\n"
            << "  add <file_path>  - Add a file to DFS\n"
            << "  rm <file_path>   - Remove a file from DFS\n"
            << "  ls               - List contents of current directory\n"
            << "  cd <dir>         - Change current directory\n"
            << "  rmdir <dir>      - Delete directory and contents\n"
            << "  exit             - Exit the CLI\n"
            << "  help             - Show this help message\n";
}

} // namespace cli
} // namespace dfs