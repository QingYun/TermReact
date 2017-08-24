#include "gtest/gtest.h"
#include <memory>
#include <type_traits>
#include "../src/term-react/utils/immutable-struct.hpp"

using namespace termreact::details;

struct C1 {
  std::string str_;
  C1(std::string s) : str_(s) { }
  std::string greet() const {
    return "Hello! " + str_;
  }
};

IMMUTABLE_STRUCT(S1, ((int, number))((std::unique_ptr<int>, pointer)));
IMMUTABLE_STRUCT(S2, ((C1, obj)));
IMMUTABLE_STRUCT(S3, ((int, number)));
IMMUTABLE_STRUCT(S4, ((C1, obj, C1{"abc"}))((std::shared_ptr<int>, pointer)));

TEST(ImmutableStructTest, trivial_construction) {
  S2 s{TrivalConstruction_t{}};
  EXPECT_FALSE(s);
}

TEST(ImmutableStructTest, default_construction) {
  // default construct all fields
  S1 s;
  EXPECT_FALSE(s.get<S1::Field::pointer>());
}

TEST(ImmutableStructTest, initializor) {
  // initialize with given values
  auto pt = std::make_unique<int>(123);
  S1 s{std::make_tuple(234, std::move(pt))};
  EXPECT_EQ(s.get<S1::Field::number>(), 234);
  EXPECT_TRUE(s.get<S1::Field::pointer>());
  EXPECT_EQ(*s.get<S1::Field::pointer>(), 123);
  EXPECT_FALSE(pt);
}

TEST(ImmutableStructTest, copy_construction) {
  // copy constructor
  auto pt = std::make_unique<int>(123);
  S1 s1{std::make_tuple(234, std::move(pt))};
  S1 s2 = s1;

  EXPECT_FALSE(pt);
  EXPECT_TRUE(s1);
  EXPECT_TRUE(s2);

  EXPECT_EQ(s1.get<S1::Field::number>(), 234);
  EXPECT_TRUE(s1.get<S1::Field::pointer>());
  EXPECT_EQ(*s1.get<S1::Field::pointer>(), 123);

  EXPECT_EQ(s2.get<S1::Field::number>(), 234);
  EXPECT_TRUE(s2.get<S1::Field::pointer>());
  EXPECT_EQ(*s2.get<S1::Field::pointer>(), 123);
}

TEST(ImmutableStructTest, move_construction) {
  // move constructor
  auto pt = std::make_unique<int>(123);
  S1 s1{std::make_tuple(234, std::move(pt))};
  S1 s2 = std::move(s1);

  EXPECT_FALSE(pt);
  EXPECT_FALSE(s1);

  EXPECT_EQ(s2.get<S1::Field::number>(), 234);
  EXPECT_TRUE(s2.get<S1::Field::pointer>());
  EXPECT_EQ(*s2.get<S1::Field::pointer>(), 123);
}

TEST(ImmutableStructTest, copy_assignment) {
  auto pt = std::make_unique<int>(123);
  S1 s1{std::make_tuple(234, std::move(pt))}, s2;

  EXPECT_FALSE(s2.get<S1::Field::pointer>());

  s2 = s1;

  EXPECT_TRUE(s1);
  EXPECT_TRUE(s2);
  EXPECT_EQ(s1, s2);
}

TEST(ImmutableStructTest, move_assignment) {
  auto pt = std::make_unique<int>(123);
  S1 s1{std::make_tuple(234, std::move(pt))}, s2;

  EXPECT_FALSE(s2.get<S1::Field::pointer>());

  s2 = std::move(s1);

  EXPECT_FALSE(s1);
  EXPECT_TRUE(s2);
  EXPECT_EQ(s2.get<S1::Field::number>(), 234);
  EXPECT_EQ(*s2.get<S1::Field::pointer>(), 123);
}

TEST(ImmutableStructTest, non_default_constructible) {
  // not default-constructible
  EXPECT_FALSE(std::is_default_constructible<S2>::value);
  S2 s{C1{"abc"}};
  EXPECT_EQ(s.get<S2::Field::obj>().greet(), "Hello! abc");
}

TEST(ImmutableStructTest, shared_ownership) {
  S3 s1;
  S3 s2 = s1;
  EXPECT_EQ(s1, s2);
}

TEST(ImmutableStructTest, no_change_on_same_updating_value) {
  S3 s1{123};
  S3 s2 = s1;

  EXPECT_EQ(s1, s2);
  EXPECT_EQ(s1.get<S3::Field::number>(), 123);
  EXPECT_EQ(s2.get<S3::Field::number>(), 123);

  s1.update<S3::Field::number>(123);
  EXPECT_EQ(s1, s2);
  EXPECT_EQ(s1.get<S3::Field::number>(), 123);
  EXPECT_EQ(s2.get<S3::Field::number>(), 123);

  s2.update<S3::Field::number>(123);
  EXPECT_EQ(s1, s2);
  EXPECT_EQ(s1.get<S3::Field::number>(), 123);
  EXPECT_EQ(s2.get<S3::Field::number>(), 123);
}

TEST(ImmutableStructTest, new_on_update) {
  S3 s1{123};
  S3 s2 = s1;

  EXPECT_EQ(s1, s2);
  EXPECT_EQ(s1.get<S3::Field::number>(), 123);
  EXPECT_EQ(s2.get<S3::Field::number>(), 123);

  s1.update<S3::Field::number>(456);
  EXPECT_NE(s1, s2);
  EXPECT_EQ(s1.get<S3::Field::number>(), 456);
  EXPECT_EQ(s2.get<S3::Field::number>(), 123);
}

TEST(ImmutableStructTest, not_equal_on_value) {
  S3 s1{123}, s2{123};
  EXPECT_NE(s1, s2);
  EXPECT_EQ(s1.get<S3::Field::number>(), 123);
  EXPECT_EQ(s2.get<S3::Field::number>(), 123);

  S3 s3 = s1;
  EXPECT_NE(s3, s2);
  s3.update<S3::Field::number>(456);
  s3.update<S3::Field::number>(123);
  EXPECT_NE(s2, s3);
  EXPECT_NE(s1, s3);
}

TEST(ImmutableStructTest, default_value) {
  EXPECT_TRUE(std::is_default_constructible<S4>::value);
  S4 s{};
  EXPECT_TRUE(s);
  EXPECT_FALSE(s.get<S4::Field::pointer>());
  EXPECT_EQ(s.get<S4::Field::obj>().greet(), "Hello! abc");
}

TEST(ImmutableStructTest, chained_update) {
  S4 s;
  s
    .update<S4::Field::obj>(C1{"abc"})
    .update<S4::Field::pointer>(std::make_shared<int>(456));

  EXPECT_EQ(s.get<S4::Field::obj>().greet(), "Hello! abc");
  EXPECT_EQ(*s.get<S4::Field::pointer>(), 456);
}

TEST(ImmutableStructTest, assume_incomparable_values_differ) {
  S2 s1{C1{"abc"}};
  S2 s2 = s1;

  EXPECT_EQ(s1, s2);
  EXPECT_EQ(s1.get<S2::Field::obj>().greet(), "Hello! abc");

  s1.update<S2::Field::obj>(C1{"abc"});
  EXPECT_NE(s1, s2);
  EXPECT_EQ(s1.get<S2::Field::obj>().greet(), "Hello! abc");
}
