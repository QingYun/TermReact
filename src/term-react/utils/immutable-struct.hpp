#pragma once
#include <memory>
#include <tuple>
#include <type_traits>
#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/tuple.hpp>
#include "./select-overload.hpp"

struct TrivalConstruction_t {};

#define IMMUTABLE_STRUCT_GET_TYPES_OP_(s, d, tuple) BOOST_PP_TUPLE_ELEM(0, tuple)
#define IMMUTABLE_STRUCT_GET_TYPES_(tuple_seq) \
  BOOST_PP_SEQ_ENUM( \
    BOOST_PP_SEQ_TRANSFORM(IMMUTABLE_STRUCT_GET_TYPES_OP_, _, tuple_seq))

#define IMMUTABLE_STRUCT_GET_FIELDS_OP_(s, d, tuple) BOOST_PP_TUPLE_ELEM(1, tuple)
#define IMMUTABLE_STRUCT_GET_FIELDS_(tuple_seq) \
  BOOST_PP_SEQ_ENUM( \
    BOOST_PP_SEQ_TRANSFORM(IMMUTABLE_STRUCT_GET_FIELDS_OP_, _, tuple_seq))

#define IMMUTABLE_STRUCT_IS_DEFAULT_CONSTRUCTIBLE_(tuple) \
  BOOST_PP_IF(BOOST_PP_EQUAL(2, BOOST_PP_TUPLE_SIZE(tuple)), \
    std::is_default_constructible<BOOST_PP_TUPLE_ELEM(0, tuple)>::value, \
    1)
#define IMMUTABLE_STRUCT_DEFAULT_CONSTRUCTIBLITY_OP_(s, d, tuple) \
  && IMMUTABLE_STRUCT_IS_DEFAULT_CONSTRUCTIBLE_(tuple)
#define IMMUTABLE_STRUCT_DEFAULT_CONSTRUCTIBLITY_(tuple_seq) \
  IMMUTABLE_STRUCT_IS_DEFAULT_CONSTRUCTIBLE_(BOOST_PP_SEQ_HEAD(tuple_seq)) \
  BOOST_PP_SEQ_FOR_EACH(IMMUTABLE_STRUCT_DEFAULT_CONSTRUCTIBLITY_OP_, _, \
                        BOOST_PP_SEQ_TAIL(tuple_seq))

#define IMMUTABLE_STRUCT_DEFAULT_VALUES_OP_PROVIDED_(tuple) \
  (BOOST_PP_TUPLE_ELEM(2, tuple))
#define IMMUTABLE_STRUCT_DEFAULT_VALUES_OP_DEFAULT_(tuple) \
  (BOOST_PP_TUPLE_ELEM(0, tuple)())
#define IMMUTABLE_STRUCT_DEFAULT_VALUES_OP_(s, d, tuple) \
  BOOST_PP_IF(BOOST_PP_EQUAL(BOOST_PP_TUPLE_SIZE(tuple), 3), \
    IMMUTABLE_STRUCT_DEFAULT_VALUES_OP_PROVIDED_(tuple), \
    IMMUTABLE_STRUCT_DEFAULT_VALUES_OP_DEFAULT_(tuple))
#define IMMUTABLE_STRUCT_DEFAULT_VALUES_(tuple_seq) \
  BOOST_PP_SEQ_ENUM( \
    BOOST_PP_SEQ_TRANSFORM(IMMUTABLE_STRUCT_DEFAULT_VALUES_OP_, _, tuple_seq))

// Sample output for
//    IMMUTABLE_STRUCT(S1,
//      ((int, number, 123))
//      ((std::unique_ptr<int>, pointer))
//    );
//
// class S1 {
//  public:
//   using Tuple = std::tuple<int, std::unique_ptr<int>>;
//   enum class Field { number, pointer };
//
//  private:
//   std::shared_ptr<Tuple> pt_;
//
//  public:
//   template <bool B = 1 && std::is_default_constructible<std::unique_ptr<int>>::value,
//             typename = std::enable_if_t<B>>
//   S1()
//       : pt_(std::make_shared<Tuple>((123), (std::unique_ptr<int>()))) {}
//
//   S1(S1&) = default;
//   S1(S1&&) = default;
//   S1(const S1&) = default;
//
//   template <typename... Ts, typename = std::enable_if_t<sizeof...(Ts)>>
//   S1(Ts&&... args)
//       : pt_(std::make_shared<Tuple>(std::forward<Ts>(args)...)) {}
//
//   template <
//       Field F, typename T,
//       typename = std::enable_if_t<std::is_assignable<
//           std::tuple_element_t<static_cast<std::size_t>(F), Tuple>&, T>::value>>
//   void update(T&& v) {
//     constexpr std::size_t I = static_cast<std::size_t>(F);
//     if (std::get<I>(*pt_) != v) {
//       if (!pt_.unique()) {
//         pt_ = std::make_shared<Tuple>(*pt_);
//       }
//       std::get<I>(*pt_) = std::forward<T>(v);
//     }
//   }
//
//   template <Field F>
//   const std::tuple_element_t<static_cast<std::size_t>(F), Tuple>& get() const {
//     constexpr std::size_t I = static_cast<std::size_t>(F);
//     return std::get<I>(*pt_);
//   }
//
//   bool operator==(const S1& other) const { return other.pt_ == pt_; }
//   bool operator!=(const S1& other) const { return !(other == *this); }
//   explicit operator bool() const { return !!pt_; }
// };

#define IMMUTABLE_STRUCT(class_name, fields) \
  class class_name { \
  public: \
    using Tuple = std::tuple<IMMUTABLE_STRUCT_GET_TYPES_(fields)>; \
    enum class Field {IMMUTABLE_STRUCT_GET_FIELDS_(fields)}; \
  private: \
    std::shared_ptr<Tuple> pt_; \
    template <typename T, typename U> \
    bool is_equal_(T&& t, U&& u, choice<0>, decltype(t == u)* = nullptr) { \
      return t == u; \
    } \
    template <typename T, typename U> \
    bool is_equal_(T&& t, U&& u, choice<1>, decltype(t != u)* = nullptr) { \
      return !(t != u); \
    } \
    template <typename T, typename U> \
    bool is_equal_(T&&, U&&, otherwise) { \
      return false; \
    } \
  public: \
    class_name(TrivalConstruction_t) {} \
    template <bool B = IMMUTABLE_STRUCT_DEFAULT_CONSTRUCTIBLITY_(fields), \
              typename = std::enable_if_t<B>> \
    class_name() \
      : pt_(std::make_shared<Tuple>(IMMUTABLE_STRUCT_DEFAULT_VALUES_(fields))) {} \
    class_name(class_name&) = default; \
    class_name(class_name&&) = default; \
    class_name(const class_name&) = default; \
    template <typename... Ts, \
      typename = std::enable_if_t<sizeof...(Ts) && \
                                  std::is_constructible<Tuple, Ts...>::value>> \
    explicit class_name(Ts&&... args) \
      : pt_(std::make_shared<Tuple>(std::forward<Ts>(args)...)) {} \
    template <Field F, typename T, \
              typename = std::enable_if_t< \
                std::is_assignable< \
                  std::tuple_element_t<static_cast<std::size_t>(F), Tuple>&, \
                  T>::value>> \
    class_name& update(T&& v) { \
      constexpr std::size_t I = static_cast<std::size_t>(F); \
      if (!is_equal_(std::get<I>(*pt_), v, select_overload_t{})) { \
        if (!pt_.unique()) { \
          pt_ = std::make_shared<Tuple>(*pt_); \
        } \
        std::get<I>(*pt_) = std::forward<T>(v); \
      } \
      return *this; \
    } \
    template <Field F> \
    const std::tuple_element_t<static_cast<std::size_t>(F), Tuple>& get() const { \
      constexpr std::size_t I = static_cast<std::size_t>(F); \
      return std::get<I>(*pt_); \
    } \
    bool operator == (const class_name& other) const { \
      return other.pt_ == pt_; \
    } \
    bool operator != (const class_name& other) const { \
      return !(other == *this); \
    } \
    class_name& operator = (const class_name& other) { \
      pt_ = other.pt_; \
      return *this; \
    } \
    class_name& operator = (class_name&& other) { \
      pt_ = std::move(other.pt_); \
      return *this; \
    } \
    explicit operator bool() const { return !!pt_; } \
  }
