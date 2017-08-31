#pragma once
#include <string>
#include "../end-component.hpp"

namespace termreact {

CREATE_END_COMPONENT_CLASS(CenteredText) {
  DECL_END_PROPS(
    (std::string, msg)
    (int, offset_x, 0)
    (int, offset_y, 0)
    (uint16_t, frontground, 0)
    (uint16_t, background, 0)
  );

  MAP_STATE_TO_END_PROPS();
 
public:
  END_COMPONENT_WILL_MOUNT(CenteredText) {}
  END_COMPONENT_WILL_UNMOUNT(CenteredText) {}

  END_COMPONENT_WILL_UPDATE(next_props) {}

  Canvas& present(Canvas& canvas) {
    std::string msg = PROPS(msg);
    canvas.writeString(
      (canvas.getWidth() - msg.size()) / 2 + PROPS(offset_x), 
      (canvas.getHeight() - 1) / 2 + PROPS(offset_y), 
      msg,
      PROPS(frontground),
      PROPS(background)
    );
    return canvas;
  }
};

}
