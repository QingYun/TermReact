#include <string>
#include <iostream>
#include <type_traits>
#include "./util/logger.h"
#include "../src/term-react/term-react.hpp"
#include "../src/term-react/termbox/termbox.cpp"
#include "../src/term-react/termbox/utf8.cpp"

namespace tr = termreact;

enum class Action {
  increaseCounter
};

PASS_REDUCER(counterReducer);
INIT_REDUCER(counterReducer, () { return 0; });
REDUCER(counterReducer, (Action::increaseCounter), (int prev_counter) {
  return prev_counter + 1;
});

PASS_REDUCER(messageReducer);
INIT_REDUCER(messageReducer, () { return "Hello World"; });

DECL_STORE(Store,
  (int, counter, counterReducer)
  (std::string, message, messageReducer)
);

CREATE_COMPONENT_CLASS(App) {
  DECL_PROPS(
    (std::string, greeting)
    (int, times)
  );

  MAP_STATE_TO_PROPS(
    (greeting, STATE_FIELD(message))
    (times, STATE_FIELD(counter))
  )

  void render_() {
    RENDER_COMPONENT(tr::CenteredBox, ATTRIBUTES(
      (width, PROPS(greeting).size() + 10)
      (height, 5)
    )) {
      RENDER_COMPONENT(tr::CenteredText, "greetingID", ATTRIBUTES(
        (msg, PROPS(greeting) + " x" + std::to_string(PROPS(times)))
      )) { NO_CHILDREN };
    };
  }

  void onKeyPress_(const tr::Event&) {
    DISPATCH(Action::increaseCounter)();
  }

public:
  COMPONENT_WILL_MOUNT(App) {}
  void componentWillUpdate(const Props&) {}
};

int main() {
  Logger::init(Logger::createTerminal());
  Store store;
  tr::Termbox tb{store, TB_OUTPUT_NORMAL};

  tb.render<App>(store);
  tb.runMainLoop();
}
