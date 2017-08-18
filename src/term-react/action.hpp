#pragma once
#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/tuple.hpp>
#include <boost/preprocessor/variadic.hpp>

namespace termreact {
namespace details {

template <typename T, T> class EnumValue;
template <typename T>
class ToEnumValue {
public:
  template <T V>
  using Type = EnumValue<T, V>;
};
  
enum class BuiltinAction {
  InitStore,
  UpdateWindowWidth,
  UpdateWindowHeight
};

}

#define ACTION_OP(s, d, field) \
  ::termreact::details::ToEnumValue<decltype(field)>::Type<field>
#define ACTION(...) \
  BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_TRANSFORM(ACTION_OP, _, BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)))

}
