#include <string>
#include <iostream>
#include <type_traits>
#include "./util/logger.h"
#include "../src/term-react/term-react.hpp"
#include "../src/term-react/termbox/termbox.cpp"
#include "../src/term-react/termbox/utf8.cpp"

namespace tr = termreact;

enum class Action {
  Increase,
  Enlarge,
  Shrink,
  Highlight,
  ResetHighlight
};

enum class GlobalState {
  Counter
};

INIT_REDUCER(counterReducer, () { return 0; });
REDUCER(counterReducer, (GlobalState::Counter)(Action::Increase), (int prev_counter) {
  return prev_counter + 1;
});

INIT_REDUCER(messageReducer, () { return "Hello World"; });

enum class AppState {
  Box,
  Text1,
  Text2
};

INIT_REDUCER(boxReducer, () { return 1; });
REDUCER(boxReducer, (AppState::Box)(Action::Enlarge), (int prev_factor) {
  return prev_factor + 1;
});
REDUCER(boxReducer, (AppState::Box)(Action::Shrink), (int prev_factor) {
  return prev_factor - 1;
});

INIT_REDUCER(text1Reducer, () { return false; });
REDUCER(text1Reducer, (AppState::Text1)(Action::Highlight), (bool) {
  return true;
});
REDUCER(text1Reducer, (AppState::Text1)(Action::ResetHighlight), (bool) {
  return false;
});
INIT_REDUCER(text2Reducer, () { return false; });
REDUCER(text2Reducer, (AppState::Text2)(Action::Highlight), (bool) {
  return true;
});
REDUCER(text2Reducer, (AppState::Text2)(Action::ResetHighlight), (bool) {
  return false;
});

DECL_STORE(Store,
  (int, counter, counterReducer)
  (std::string, message, messageReducer)
  (int, boxSizeFactor, boxReducer)
  (bool, text1Highlight, text1Reducer)
  (bool, text2Highlight, text2Reducer)
);

CREATE_COMPONENT_CLASS(App) {
  DECL_PROPS(
    (std::string, greeting)
    (int, times)
    (int, boxFactor)
    (bool, text1Highlight)
    (bool, text2Highlight)
  );

  MAP_STATE_TO_PROPS(
    (greeting, STATE_FIELD(message))
    (times, STATE_FIELD(counter))
    (boxFactor, STATE_FIELD(boxSizeFactor))
    (text1Highlight, STATE_FIELD(text1Highlight))
    (text2Highlight, STATE_FIELD(text2Highlight))
  )

  void render_() {
    RENDER_COMPONENT(tr::CenteredBox, ATTRIBUTES(
      (width, (PROPS(greeting).size() + 10) * PROPS(boxFactor))
      (height, 3 * PROPS(boxFactor))
      (focusable, PROPS(times) < 5)
      (onKeyPress, [this] (tr::Event evt) { this->onKeyPress_(evt); })
      (onFocus, [this] () { DISPATCH(AppState::Box, Action::Enlarge)(); })
      (onLostFocus, [this] () { DISPATCH(AppState::Box, Action::Shrink)(); })
    )) {
      RENDER_COMPONENT(tr::CenteredText, "greetingID", ATTRIBUTES(
        (msg, PROPS(greeting) + " x" + std::to_string(PROPS(times)))
        (focusable, true)
        (frontground, PROPS(text1Highlight) ? TB_DEFAULT | TB_BOLD | TB_UNDERLINE : TB_DEFAULT)
        (onKeyPress, [this] (tr::Event evt) { this->onKeyPress_(evt); })
        (onFocus, [this] () { DISPATCH(AppState::Text1, Action::Highlight)(); })
        (onLostFocus, [this] () { DISPATCH(AppState::Text1, Action::ResetHighlight)(); })
      )) { NO_CHILDREN };
      RENDER_COMPONENT(tr::CenteredText, "greetingID2", ATTRIBUTES(
        (offset_y, 1)
        (msg, "greeting *" + std::to_string(PROPS(times)))
        (focusable, true)
        (frontground, PROPS(text2Highlight) ? TB_BLACK : TB_DEFAULT)
        (background, PROPS(text2Highlight) ? TB_WHITE : TB_DEFAULT)
        (onKeyPress, [this] (tr::Event evt) { this->onKeyPress_(evt); })
        (onFocus, [this] () { DISPATCH(AppState::Text2, Action::Highlight)(); })
        (onLostFocus, [this] () { DISPATCH(AppState::Text2, Action::ResetHighlight)(); })
      )) { NO_CHILDREN };
      if (PROPS(times) % 2) {
        RENDER_COMPONENT(tr::CenteredText, "greetingID3", ATTRIBUTES(
          (offset_y, 2)
          (msg, "switch *" + std::to_string(PROPS(times)))
          (focusable, true)
        )) { NO_CHILDREN };
      }
    };
  }

  void onKeyPress_(const tr::Event&) {
    DISPATCH(GlobalState::Counter, Action::Increase)();
  }

public:
  COMPONENT_WILL_MOUNT(App) {}
  void componentWillUpdate(const Props&) {
    logger() << typeid(std::decay_t<decltype(*this)>).hash_code();
  }
};

int main() {
  Logger::init(Logger::createTerminal());
  Store store;
  tr::Termbox tb{store, TB_OUTPUT_NORMAL};

  tb.render<App>(store, [] (const Store::StateType &state) {
    if (state.template get<std::decay_t<decltype(state)>::Field::counter>() > 10)
      return true;
    return false;
  });
  tb.runMainLoop();
}
