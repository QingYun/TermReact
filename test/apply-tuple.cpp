#include "gtest/gtest.h"
#include <tuple>
#include <memory>
#include "../src/term-react/utils/apply-tuple.hpp"

using namespace termreact::details;

int f1(int i) { return i; }

TEST(ApplyTupleTest, free_function) {
  auto t = std::make_tuple(123);
  EXPECT_EQ(applyTuple(f1, t), 123);
}

int f2(int a, int b) { return a - b; }

TEST(ApplyTupleTest, with_rest_parameters) {
  auto t = std::make_tuple(123);
  EXPECT_EQ(ApplyTupleWith<int>::apply(f2, t, 125), 2);
}

std::unique_ptr<int> f3(std::unique_ptr<int> p) { return p; }
int f4(std::unique_ptr<int> p1, std::unique_ptr<int> p2) { return *p1 + *p2; }

TEST(ApplyTupleTest, forwards_parameters) {
  auto pp1 = new int(123);
  auto p1 = applyTuple(f3, std::make_tuple(std::unique_ptr<int>{ pp1 }));
  EXPECT_EQ(p1.get(), pp1);

  EXPECT_EQ(
    ApplyTupleWith<std::unique_ptr<int>>::apply(f4, 
      std::make_tuple(std::make_unique<int>(1)), 
      std::make_unique<int>(2)
    ), 3);
}

struct C1 {
  static int f5(std::unique_ptr<int> p1, std::unique_ptr<int> p2) { return *p1 + *p2; }
};

TEST(ApplyTupleTest, static_method) {
  EXPECT_EQ(
    ApplyTupleWith<std::unique_ptr<int>>::apply(
      C1::f5, 
      std::make_tuple(std::make_unique<int>(1)), 
      std::make_unique<int>(2)
    ), 
    3
  );
}