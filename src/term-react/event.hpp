#pragma once
#include <vector>
#include <functional>
#include <algorithm>
#include "./action.hpp"
#include "./reducer.hpp"
#include "./component.hpp"

namespace termreact {

struct Event {
  uint8_t mod; 
  uint16_t key;
  uint32_t ch;
};

using EventHandler = std::function<void(Event)>;

namespace details {

class Focusable {
public:
  virtual void onFocus() = 0;
  virtual void onLostFocus() = 0;
  virtual void onKeyPress(Event) = 0;
};

struct FocusableStore {
  bool rescanning;
  size_t build;
  Focusable *focus;
  std::vector<Focusable*> focusables;
};

INIT_REDUCER(focusableReducer, () { return FocusableStore{ false, 0, nullptr, {} }; });

// rescan is triggered by adding new focusables, in which case we need to know the right place for them
// the current focus should be reserved as it won't be deleted anyway
REDUCER(focusableReducer, (BuiltinAction::rescanFocusable), (FocusableStore prev) {
  return FocusableStore{ true, prev.build + 1, prev.focus, {} };
});

// the paired action of rescan, also triggered by adding focusables.
REDUCER(focusableReducer, (BuiltinAction::stopFocusableScanning), (FocusableStore prev) {
  return FocusableStore{ false, prev.build, prev.focus, std::move(prev.focusables) };
});

// select a new focus only when there's no previous one
REDUCER(focusableReducer, (BuiltinAction::selectFocus), (FocusableStore prev) {
  auto new_focus = prev.focus;
  if (new_focus == nullptr && prev.focusables.size() != 0) {
    new_focus = prev.focusables[0];
  }
  return FocusableStore{ prev.rescanning, prev.build, new_focus, std::move(prev.focusables) };
});

// focusables dispatch register action as the response to the scanning
REDUCER(focusableReducer, (BuiltinAction::registerFocusable), (FocusableStore prev, size_t build, Focusable *focusable) {
  if (build != prev.build) return prev;
  logger() << "register " << focusable << std::endl;
  auto focusables = std::move(prev.focusables);
  focusables.push_back(focusable);
  return FocusableStore{ prev.rescanning, prev.build, prev.focus, std::move(focusables) };
});

// triggered by removing a focusable. switch to next if the current focus is being removed.
REDUCER(focusableReducer, (BuiltinAction::unregisterFocusable), (FocusableStore prev, size_t build, Focusable *focusable) {
  if (build != prev.build) return prev;
  logger() << "unregister " << focusable << std::endl;
  auto focusables = std::move(prev.focusables);
  auto new_focus = prev.focus;
  if (focusable == prev.focus) {
    auto cur = std::find(focusables.begin(), focusables.end(), focusable);
    if ((cur + 1) == focusables.end()) {
      if (focusables.size() == 1) {
        new_focus = nullptr;
      } else {
        new_focus = focusables[0];
      }
    } else {
      new_focus = *(cur + 1);
    }
    focusables.erase(cur);
  } else {
    std::remove(focusables.begin(), focusables.end(), focusable);
  }
  return FocusableStore{ prev.rescanning, prev.build, new_focus, std::move(focusables) };
});

REDUCER(focusableReducer, (BuiltinAction::nextFocus), (FocusableStore prev) {
  auto focus = prev.focus;
  auto cur = std::find(prev.focusables.begin(), prev.focusables.end(), prev.focus);
  if ((cur + 1) == prev.focusables.end()) {
    focus = prev.focusables[0];
  } else {
    focus = *(cur + 1);
  }
  return FocusableStore{ prev.rescanning, prev.build, focus, std::move(prev.focusables) };
});

}
}