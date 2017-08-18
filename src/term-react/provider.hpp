#pragma once
#include <memory>
#include <chrono>
#include <type_traits>
#include "./canvas.hpp"
#include "./component.hpp"

namespace termreact {
namespace details {

template <typename... Predicates>
class ShouldExit;

template <typename Predicate, typename... RestPredicates>
class ShouldExit<Predicate, RestPredicates...> : public ShouldExit<RestPredicates...> {
private:
  using Next = ShouldExit<RestPredicates...>;
  Predicate predicate_;

public:
  ShouldExit(Predicate&& predicate, RestPredicates&&... rest_predicates)
  : Next{std::forward<RestPredicates>(rest_predicates)...}, predicate_{std::forward<Predicate>(predicate)} {}

  template <typename StateT>
  bool shouldExit(const StateT& state) const {
    return predicate_(state) || Next::shouldExit(state);
  }
};

template <>
class ShouldExit<> {
public:
  template <typename StateT>
  bool shouldExit(const StateT&) const {
    return false;
  }
};

}

class Provider {
private:
  ComponentPointer root_elm_;
  virtual void render_(ComponentPointer root_elm) = 0;
  virtual void exit_() = 0;

protected:
  void setRootElm_(ComponentPointer root_elm) { root_elm_ = std::move(root_elm); }
  ComponentPointer& getRootElm_() { return root_elm_; }

public:
  virtual Canvas& getCanvas() = 0;
  virtual void runMainLoop(std::chrono::microseconds frame_duration = std::chrono::microseconds{16667}) = 0;

  template <template <typename> typename C, typename StoreT, typename... ExitPredicates>
  void render(StoreT& store, ExitPredicates&&... exit_predicates) {
    using CT = C<StoreT>;
    using State = typename StoreT::StateType;

    store.addListener(
      [this, should_exit{details::ShouldExit<ExitPredicates...>{std::forward<ExitPredicates>(exit_predicates)...}}]
      (const State&, const State& next_state) {
        if (should_exit.shouldExit(next_state)) {
          exit_();
        }
      }
    );

    store.addListener([this] (const State&, const State& next_state) {
      dynamic_cast<C<StoreT>*>(getRootElm_().get())->onStoreUpdate(static_cast<const void*>(&next_state));
    }, false);

    render_(details::createComponent<CT>(typename CT::Props{}, store));
  }
};


#define EXIT_COND [] (const auto& __state) -> bool
#define STORE_FIELD(F) __state.template get<std::decay_t<decltype(__state)>::Field::F>()

}
