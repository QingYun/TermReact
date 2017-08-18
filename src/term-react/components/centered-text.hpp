#pragma once
#include <string>
#include "../component.hpp"

namespace termreact {

CREATE_END_COMPONENT_CLASS(CenteredText) {
  DECL_PROPS(
    (std::string, msg)
  );
 
public:
  END_COMPONENT_WILL_MOUNT(CenteredText) {}
  void componentWillUpdate(const Props&) {}

  Canvas& present(Canvas& canvas) {
    std::string msg = PROPS(msg);
    canvas.writeString((canvas.getWidth() - msg.size()) / 2, (canvas.getHeight() - 1) / 2, msg);
    return canvas;
  }
};

}
