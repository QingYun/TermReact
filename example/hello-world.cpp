#include <string>
#include <iostream>
#include <type_traits>
#include "./util/logger.h"
#include "../src/term-react/term-react.hpp"
#include "../src/term-react/termbox/termbox.cpp"
#include "../src/term-react/termbox/utf8.cpp"
#include "../src/term-react/dangerous-pretty-macros.h"

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

InitReducer(counterReducer, () { return 0; });
Reducer(counterReducer, (GlobalState::Counter)(Action::Increase), (int prev_counter) {
  return prev_counter + 1;
});

InitReducer(messageReducer, () { return "Hello World"; });

enum class AppState {
  Box,
  Text1,
  Text2
};

InitReducer(boxReducer, () { return 1; });
Reducer(boxReducer, (AppState::Box)(Action::Enlarge), (int prev_factor) {
  return prev_factor + 1;
});
Reducer(boxReducer, (AppState::Box)(Action::Shrink), (int prev_factor) {
  return prev_factor - 1;
});

InitReducer(text1Reducer, () { return false; });
Reducer(text1Reducer, (AppState::Text1)(Action::Highlight), (bool) {
  return true;
});
Reducer(text1Reducer, (AppState::Text1)(Action::ResetHighlight), (bool) {
  return false;
});
InitReducer(text2Reducer, () { return false; });
Reducer(text2Reducer, (AppState::Text2)(Action::Highlight), (bool) {
  return true;
});
Reducer(text2Reducer, (AppState::Text2)(Action::ResetHighlight), (bool) {
  return false;
});

DeclareStore(Store,
  (int, counter, counterReducer)
  (std::string, message, messageReducer)
  (int, boxSizeFactor, boxReducer)
  (bool, text1Highlight, text1Reducer)
  (bool, text2Highlight, text2Reducer)
);

ComponentClass(App) {
  DeclareProps(
    (std::string, greeting)
    (int, times)
    (int, boxFactor)
    (bool, text1Highlight)
    (bool, text2Highlight)
  );

  MapStoreStateToProps(
    (greeting, State(message))
    (times, State(counter))
    (boxFactor, State(boxSizeFactor))
    (text1Highlight, State(text1Highlight))
    (text2Highlight, State(text2Highlight))
  )

  Render() {
    Component(tr::CenteredBox, Attr(
      (width, (Props(greeting).size() + 10) * Props(boxFactor))
      (height, 3 * Props(boxFactor))
      (focusable, Props(times) < 5)
      (onKeyPress, [this] (tr::Event evt) { this->onKeyPress(evt); })
      (onFocus, [this] () { Dispatch(AppState::Box, Action::Enlarge)(); })
      (onLostFocus, [this] () { Dispatch(AppState::Box, Action::Shrink)(); })
    )) {
      Component(tr::CenteredText, "greetingID", Attr(
        (msg, Props(greeting) + " x" + std::to_string(Props(times)))
        (focusable, true)
        (frontground, Props(text1Highlight) ? TB_DEFAULT | TB_BOLD | TB_UNDERLINE : TB_DEFAULT)
        (onKeyPress, [this] (tr::Event evt) { this->onKeyPress(evt); })
        (onFocus, [this] () { Dispatch(AppState::Text1, Action::Highlight)(); })
        (onLostFocus, [this] () { Dispatch(AppState::Text1, Action::ResetHighlight)(); })
      )) { NO_CHILDREN };
      Component(tr::CenteredText, "greetingID2", Attr(
        (offset_y, 1)
        (msg, "greeting *" + std::to_string(Props(times)))
        (focusable, true)
        (frontground, Props(text2Highlight) ? TB_BLACK : TB_DEFAULT)
        (background, Props(text2Highlight) ? TB_WHITE : TB_DEFAULT)
        (onKeyPress, [this] (tr::Event evt) { this->onKeyPress(evt); })
        (onFocus, [this] () { Dispatch(AppState::Text2, Action::Highlight)(); })
        (onLostFocus, [this] () { Dispatch(AppState::Text2, Action::ResetHighlight)(); })
      )) { NO_CHILDREN };
      if (Props(times) % 2) {
        Component(tr::CenteredText, "greetingID3", Attr(
          (offset_y, 2)
          (msg, "switch *" + std::to_string(Props(times)))
          (focusable, true)
        )) { NO_CHILDREN };
      }
    };
  }

  void onKeyPress(const tr::Event&) {
    Dispatch(GlobalState::Counter, Action::Increase)();
  }

public:
  ComponentWillMount(App) {}
  ComponentWillUnmount(App) {}
  ComponentWillUpdate(next_props) {
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
