#include "gtest/gtest.h"
#include <tuple>
#include <memory>
#include "../src/term-react/reducer.hpp"
#include "../src/term-react/action.hpp"
#include "../src/term-react/utils/apply-tuple.hpp"

using namespace termreact::details;

int f1(int i) { return i; }

TEST(ApplyTupleTest, free_function) {
  auto t = std::make_tuple(123);
  EXPECT_EQ(ApplyTuple<>::apply(f1, t), 123);
}

int f2(int a, int b) { return a - b; }

TEST(ApplyTupleTest, with_rest_parameters) {
  auto t = std::make_tuple(123);
  EXPECT_EQ(ApplyTuple<int>::apply(f2, t, 125), 2);
}

std::unique_ptr<int> f3(std::unique_ptr<int> p) { return p; }
int f4(std::unique_ptr<int> p1, std::unique_ptr<int> p2) { return *p1 + *p2; }

TEST(ApplyTupleTest, forwards_parameters) {
  auto pp1 = new int(123);
  auto p1 = ApplyTuple<>::apply(f3, std::make_tuple(std::unique_ptr<int>{ pp1 }));
  EXPECT_EQ(p1.get(), pp1);

  EXPECT_EQ(
    ApplyTuple<std::unique_ptr<int>>::apply(f4, 
      std::make_tuple(std::make_unique<int>(1)), 
      std::make_unique<int>(2)
    ), 3);
}

struct C1 {
  static int f5(std::unique_ptr<int> p1, std::unique_ptr<int> p2) { return *p1 + *p2; }
};

TEST(ApplyTupleTest, static_method) {
  EXPECT_EQ(
    ApplyTuple<std::unique_ptr<int>>::apply(
      C1::f5, 
      std::make_tuple(std::make_unique<int>(1)), 
      std::make_unique<int>(2)
    ), 
    3
  );
}

enum class ApplyTupleTestAction {
  echoCounter,
  increaseCounterBy
};

INIT_REDUCER(counterReducer, () { return 0; });
REDUCER(counterReducer, (ApplyTupleTestAction::echoCounter), (int prev_counter) {
  return prev_counter;
});
REDUCER(counterReducer, (ApplyTupleTestAction::increaseCounterBy), (int prev_counter, int by) {
  return prev_counter + by;
});

TEST(ApplyTupleTest, reducer_with_param) {
  auto result = ApplyTuple<int>::apply(
    counterReducer<ACTION(ApplyTupleTestAction::increaseCounterBy)>::reduce,
    std::make_tuple(2),
    1
  );
  EXPECT_EQ(result, 3);
}

TEST(ApplyTupleTest, reducer_no_param) {
  auto r1 = ApplyTuple<>::apply(
    counterReducer<ACTION(ApplyTupleTestAction::echoCounter)>::reduce,
    std::make_tuple(2)
  );
  EXPECT_EQ(r1, 2);

  auto r2 = ApplyTuple<int>::apply(
    counterReducer<ACTION(ApplyTupleTestAction::echoCounter)>::reduce,
    std::make_tuple(),
    2
  );
  EXPECT_EQ(r2, 2);
}
