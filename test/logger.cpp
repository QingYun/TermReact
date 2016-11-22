#include "gtest/gtest.h"
#include "../example/util/logger.h"
#include "../example/util/logger.cpp"

#include <string>
#include <fstream>
#include <cstdio>

class LoggerTest: public ::testing::Test {
protected:
  std::ifstream fi;
  virtual void SetUp() {
    std::string name = std::tmpnam(nullptr);
    Logger::init(std::ofstream(name.c_str()));
    fi = std::ifstream(name.c_str());
  }
};

TEST_F(LoggerTest, write_to_stream) {
  std::string msg;
  logger() << "test1\ntest2";

  fi >> msg;
  EXPECT_EQ(msg, "test1");

  fi >> msg;
  EXPECT_EQ(msg, "test2");

  EXPECT_EQ(fi.get(), '\n');
}

TEST_F(LoggerTest, delimiter) {
  std::string msg;
  logger(",") << "test1" << "test2";

  fi >> msg;
  EXPECT_EQ(msg, "test1,test2");
}
