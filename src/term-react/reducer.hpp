#pragma once
#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/tuple.hpp>
#include <boost/preprocessor/variadic.hpp>
#include "./action.hpp"

namespace termreact {
namespace details {

template <typename T, T V> class EnumValue {};

}

#define REDUCER(name, action_tags, ...) \
  template <typename...> struct name; \
  template <> struct name<ACTION(BOOST_PP_SEQ_ENUM(action_tags))> { \
    constexpr static bool valid = true; \
    static auto reduce BOOST_PP_TUPLE_ENUM((__VA_ARGS__)) \
  }

#define INIT_REDUCER(name, ...) \
  template <typename...> struct name { \
    constexpr static bool valid = false; \
  }; \
  template <> struct name<ACTION(::termreact::details::BuiltinAction::InitStore)> { \
    constexpr static bool valid = true; \
    static auto reduce BOOST_PP_TUPLE_ENUM((__VA_ARGS__)) \
  }

namespace details {
  
INIT_REDUCER(windowWidthReducer, () { return -1; });
REDUCER(windowWidthReducer, (BuiltinAction::UpdateWindowWidth), (int, int new_width) {
  return new_width;
});

INIT_REDUCER(windowHeightReducer, () { return -1; });
REDUCER(windowHeightReducer, (BuiltinAction::UpdateWindowHeight), (int, int new_height) {
  return new_height;
});

}

}