#pragma once
#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/tuple.hpp>
#include <boost/preprocessor/variadic.hpp>
#include "./store.hpp"
#include "./action.hpp"

namespace termreact {
namespace details {

template <typename T, T V> class EnumValue {};

}

#define REDUCER(name, action_tags, func_body) \
  template <typename...> class name; \
  template <> class name<ACTION(BOOST_PP_SEQ_ENUM(action_tags))> { \
  public: \
    static auto reduce func_body \
  }
  
#define DEFAULT_INIT_REDUCER \
  static StateField reduce() { return StateField{}; }
#define INIT_REDUCER(...) \
  template <typename...> class BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__); \
  template <> class BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)<ACTION(::termreact::details::BuiltinAction::InitStore)> { \
  public: \
    template <typename StateField> \
    BOOST_PP_IF(BOOST_PP_EQUAL(2, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)), \
      static auto reduce BOOST_PP_VARIADIC_ELEM(1, __VA_ARGS__), \
      DEFAULT_INIT_REDUCER) \
  }
  
#define DEFAULT_PASS_REDUCER \
  template <typename T> \
  static T reduce(T state, ...) { return state; }
#define PASS_REDUCER(...) \
  template <typename...> class BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__) { \
  public: \
    BOOST_PP_IF(BOOST_PP_EQUAL(2, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)), \
      static auto reduce BOOST_PP_VARIADIC_ELEM(1, __VA_ARGS__), \
      DEFAULT_PASS_REDUCER) \
  }

namespace details {
  
PASS_REDUCER(windowWidthReducer);
INIT_REDUCER(windowWidthReducer);
REDUCER(windowWidthReducer, (BuiltinAction::UpdateWindowWidth), (int, int new_width) {
  return new_width;
});

PASS_REDUCER(windowHeightReducer);
INIT_REDUCER(windowHeightReducer);
REDUCER(windowHeightReducer, (BuiltinAction::UpdateWindowHeight), (int, int new_height) {
  return new_height;
});

}

}