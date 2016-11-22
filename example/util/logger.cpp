#include <memory>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include "./logger.h"

std::ofstream Logger::global_dest_terminal_;

void Logger::init(std::ofstream&& dest_terminal) {
  Logger::global_dest_terminal_ = std::move(dest_terminal);
}

Logger Logger::createLogger(const std::string& delimiter) {
  return Logger(global_dest_terminal_, delimiter);
}

std::ofstream Logger::createTerminal() {
  std::string cmd = "x-terminal-emulator -e cat ";
  std::unique_ptr<char> name(tempnam(nullptr, nullptr));
  cmd += name.get();
  mkfifo(name.get(), 0777);
  if (fork() == 0) {
    system(cmd.c_str());
    exit(0);
  }
  return std::ofstream(name.get());
}

Logger::Logger(std::ofstream& dest_terminal, const std::string& delimiter)
  : dest_terminal_(dest_terminal), delimiter_(delimiter), first_(true) {}

Logger::~Logger() {
  dest_terminal_ << std::endl;
}

Logger logger(const std::string& delimiter) {
  return Logger::createLogger(delimiter);
}
