#include "gtest/gtest.h"
#include <type_traits>
#include "../src/term-react/utils/select-overload.hpp"

using namespace termreact::details;

int f1(choice<0>) { return 0; }
int f1(choice<1>) { return 1; }

TEST(SelectOverloadTest, free_functions) {
  EXPECT_EQ(f1(select_overload_t{}), 0);
}

struct S {
  int f2(choice<0>) { return 0; }
  int f2(choice<1>) { return 1; }
};

TEST(SelectOverloadTest, methods) {
  S s;
  EXPECT_EQ(s.f2(select_overload_t{}), 0);
}

template <int N, typename = std::enable_if_t<N % 15 == 0>>
int f3(choice<0>) { return 15; }

template <int N, typename = std::enable_if_t<N % 5 == 0>>
int f3(choice<1>) { return 5; }

template <int N, typename = std::enable_if_t<N % 3 == 0>>
int f3(choice<2>) { return 3; }

template <int N>
int f3(otherwise) { return 0; }


TEST(SelectOverloadTest, function_templates) {
  EXPECT_EQ(f3<5>(select_overload_t{}), 5);
  EXPECT_EQ(f3<3>(select_overload_t{}), 3);
  EXPECT_EQ(f3<15>(select_overload_t{}), 15);
  EXPECT_EQ(f3<123>(select_overload_t{}), 3);
  EXPECT_EQ(f3<45>(select_overload_t{}), 15);
  EXPECT_EQ(f3<11>(select_overload_t{}), 0);
}
