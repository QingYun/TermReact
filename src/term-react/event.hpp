#pragma once
#include <vector>
#include "./termbox/termbox.h"
#include "./action.hpp"
#include "./reducer.hpp"
#include "./component.hpp"

namespace termreact {

using Event = tb_event;

namespace details {

struct FocusableStore {
  bool rescanning;
  std::vector<ComponentBase*> focusables;
};

INIT_REDUCER(focusableReducer, FUNC(() { return FocusableStore{ false, {} }; }));
REDUCER(focusableReducer, (BuiltinAction::rescanFocusable), FUNC((FocusableStore prev) {
  return FocusableStore{ true, std::move(prev.focusables) };
}));

}
}