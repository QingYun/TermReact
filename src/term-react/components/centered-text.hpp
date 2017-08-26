#pragma once
#include <string>
#include "../end-component.hpp"

namespace termreact {

CREATE_END_COMPONENT_CLASS(CenteredText) {
  DECL_PROPS(
    (std::string, msg)
    (int, offset_x, 0)
    (int, offset_y, 0)
  );
 
public:
  END_COMPONENT_WILL_MOUNT(CenteredText) {}
  void componentWillUpdate(const Props&) {}

  Canvas& present(Canvas& canvas) {
    std::string msg = PROPS(msg);
    canvas.writeString(
      (canvas.getWidth() - msg.size()) / 2 + PROPS(offset_x), 
      (canvas.getHeight() - 1) / 2 + PROPS(offset_y), 
      msg
    );
    return canvas;
  }
};

}
