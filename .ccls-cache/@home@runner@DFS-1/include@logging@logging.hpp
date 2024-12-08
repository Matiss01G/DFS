#pragma once
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

namespace dfs {
namespace logging {

// Initialize logging with basic console and file output
inline void init_logging(const std::string &component) {
  // Add common attributes like TimeStamp, LineID, ProcessID and ThreadID
  boost::log::add_common_attributes();

  // Setup console output
  auto console = boost::log::add_console_log();
  console->set_formatter(
      boost::log::expressions::stream
      << "["
      << boost::log::expressions::attr<boost::posix_time::ptime>("TimeStamp")
      << "] [" << component << "] [" << boost::log::trivial::severity << "] "
      << boost::log::expressions::smessage);

  // Setup file output
  auto file = boost::log::add_file_log(
      boost::log::keywords::file_name = "logs/" + component + "_%N.log",
      boost::log::keywords::rotation_size = 10 * 1024 * 1024, // 10MB
      boost::log::keywords::auto_flush = true);

  file->set_formatter(
      boost::log::expressions::stream
      << "["
      << boost::log::expressions::attr<boost::posix_time::ptime>("TimeStamp")
      << "] [" << component << "] [" << boost::log::trivial::severity << "] "
      << boost::log::expressions::smessage);
}

// Control logging severity level
inline void set_log_level(boost::log::trivial::severity_level level) {
  boost::log::core::get()->set_filter(boost::log::trivial::severity >= level);
}

// Enable/disable logging
inline void enable_logging() {
  boost::log::core::get()->set_logging_enabled(true);
}

inline void disable_logging() {
  boost::log::core::get()->set_logging_enabled(false);
}

} // namespace logging
} // namespace dfs

// Convenience macros
#define LOG_TRACE BOOST_LOG_TRIVIAL(trace)
#define LOG_DEBUG BOOST_LOG_TRIVIAL(debug)
#define LOG_INFO BOOST_LOG_TRIVIAL(info)
#define LOG_WARN BOOST_LOG_TRIVIAL(warning)
#define LOG_ERROR BOOST_LOG_TRIVIAL(error)
#define LOG_FATAL BOOST_LOG_TRIVIAL(fatal)