#pragma once

#include <fstream>
#include <string>
#include <type_traits>

class Logger {
private:
  static std::ofstream global_dest_terminal_;

  std::ofstream& dest_terminal_;
  std::string delimiter_;
  bool first_;

  Logger(std::ofstream& dest_terminal, const std::string& delimiter);
public:
  static void init(std::ofstream&& dest_terminal);
  static std::ofstream createTerminal();
  static Logger createLogger(const std::string& delimiter);

  ~Logger();

  template <typename T>
  Logger& operator << (const T& log) {
    if (!first_) {
      dest_terminal_ << delimiter_;
    } else {
      first_ = false;
    }
    dest_terminal_ << log;
    return *this;
  }

  using ManipFunc = std::ostream&(*)(std::ostream&);
  Logger& operator << (ManipFunc manip) {
    manip(dest_terminal_);
    return *this;
  }
};

Logger logger(const std::string& delimiter = "");
