#pragma once
#include <vector>
#include <queue>
#include <functional>
#include <type_traits>
#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/tuple.hpp>
#include <boost/preprocessor/variadic.hpp>
#include "./utils/immutable-struct.hpp"
#include "./utils/apply-tuple.hpp"
#include "./action.hpp"
#include "./reducer.hpp"
#include "./event.hpp"

namespace termreact {
namespace details {

using InitStore = EnumValue<BuiltinAction, BuiltinAction::InitStore>;

template <typename Reducer, typename StateType, typename StateType::Field Field>
struct UpdateStoreState {

  template <typename...>
  static void run(...) {}

  template <typename Tuple, typename = std::enable_if_t<Reducer::valid>>
  static void run(const StateType &state, StateType &next_state, Tuple t) {
    next_state.template update<Field>(ApplyTuple<std::decay_t<decltype(state.template get<Field>())>>::apply(
      Reducer::reduce,
      t,
      state.template get<Field>()
    ));
  }

};

}

#define DECL_STATE_OP(s, d, tuple) BOOST_PP_TUPLE_POP_BACK(tuple)
#define DECL_STATE(fields) \
  IMMUTABLE_STRUCT(StateType, \
    BOOST_PP_SEQ_TRANSFORM(DECL_STATE_OP, _, BOOST_PP_VARIADIC_SEQ_TO_SEQ(fields)))

#define STATE_INIT_OP(s, d, tuple) \
  BOOST_PP_TUPLE_ELEM(2, tuple)<::termreact::details::InitStore>::reduce()
#define STATE_INIT(fields) \
  BOOST_PP_SEQ_ENUM( \
    BOOST_PP_SEQ_TRANSFORM(STATE_INIT_OP, _, BOOST_PP_VARIADIC_SEQ_TO_SEQ(fields)))

#define STATE_REDUCERS_OP(s, _, field) \
  ::termreact::details::UpdateStoreState< \
    BOOST_PP_TUPLE_ELEM(2, field)<Vs...>,  \
    StateType, \
    StateType::Field::BOOST_PP_TUPLE_ELEM(1, field) \
  >::run(state, next_state, t);

// generate state updating statements
#define STATE_REDUCERS(fields) \
  BOOST_PP_SEQ_FOR_EACH(STATE_REDUCERS_OP, _, BOOST_PP_VARIADIC_SEQ_TO_SEQ(fields))

#define STORE_ADD_BUILTIN_FIELDS(fields) \
  fields \
  (int, window_height, ::termreact::details::windowHeightReducer) \
  (int, window_width, ::termreact::details::windowWidthReducer) \
  (::termreact::details::FocusableStore, focusables, ::termreact::details::focusableReducer)

// fields = ((field_type, field_name, field_reducer)(...)...)
#define DECL_STORE(classname, fields) \
  class classname { \
  public: \
    DECL_STATE(STORE_ADD_BUILTIN_FIELDS(fields)); \
    using Listener = std::function<void(const StateType&, const StateType&)>; \
  private: \
    StateType state_; \
    bool in_processing_dispatches_; \
    std::vector<Listener> listeners_; \
    std::queue<std::function<void(const StateType&, StateType&)>> pending_dispatches_; \
    void doDispatch_() { \
      in_processing_dispatches_ = true; \
      StateType next_state = state_; \
      while (pending_dispatches_.size() != 0) { \
        pending_dispatches_.front()(state_, next_state); \
        if (next_state != state_) { \
          for (auto& listener : listeners_) { \
            listener(state_, next_state); \
          } \
        } \
        pending_dispatches_.pop(); \
      } \
      state_ = std::move(next_state); \
      in_processing_dispatches_ = false; \
    } \
  public: \
    classname() : state_{STATE_INIT(STORE_ADD_BUILTIN_FIELDS(fields))}, in_processing_dispatches_{false} {} \
    template <StateType::Field F> \
    auto get() const { return state_.get<F>(); } \
    /* We have to avoid moving payloads around since they will be passed through all the reducers */ \
    template <typename... Vs, typename... Ts> \
    void dispatch(const Ts&... payload) { \
      pending_dispatches_.emplace( \
        [t = std::make_tuple(payload...)] (const StateType &state, StateType &next_state) { \
          STATE_REDUCERS(STORE_ADD_BUILTIN_FIELDS(fields)) \
        }); \
      if (!in_processing_dispatches_) \
        doDispatch_(); \
    } \
    /* to commit continuous dispatches (prevent any nested dispatches from */ \
    /*   being inserted into the queue in between them), put them in a chunk dispatch block*/ \
    void startChunkDispatch() { in_processing_dispatches_ = true; } \
    void endChunkDispatch() { doDispatch_(); } \
    void addListener(Listener listener, bool immediate_call = true) { \
      if (immediate_call) listener(state_, state_); \
      listeners_.push_back(std::move(listener)); \
    } \
  } 

}