#pragma once
#include "../end-component.hpp"

namespace termreact {
namespace {
  
const uint32_t v_wall = 0x2588; // █
const uint32_t h_wall = 0x2588;

}

CREATE_END_COMPONENT_CLASS(CenteredBox) {
  DECL_PROPS(
    (int, x)
    (int, y)
    (int, width)
    (int, height)
  );
  
  MAP_STATE_TO_PROPS(
    (x, (STATE_FIELD(window_width) - PROPS(width)) / 2)
    (y, (STATE_FIELD(window_height) - PROPS(height)) / 2)
  )
  
public:
  END_COMPONENT_WILL_MOUNT(CenteredBox) {}

  Canvas& present(Canvas& canvas) {
    auto canvas_slice = canvas.slice(PROPS(x), PROPS(y), PROPS(width), PROPS(height));
    int w = PROPS(width);
    int h = PROPS(height);
    for (int i = -1; i <= w; ++i) {
      canvas_slice.setCell(i, -1, h_wall);
      canvas_slice.setCell(i, h, h_wall);
    }
    
    for (int i = -1; i <= h; ++i) {
      canvas_slice.setCell(-1, i, v_wall);
      canvas_slice.setCell(w, i, v_wall);
    }
    
    return canvas;
  }

  void componentWillUpdate(const Props&) {}
};

}
