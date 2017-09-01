#pragma once
#include <functional>
#include "../end-component.hpp"

namespace termreact {

#define TERMREACT_NO_BORDER 0
#define TERMREACT_FULL_HEIGHT -1
#define TERMREACT_FULL_WIDTH -1

CREATE_END_COMPONENT_CLASS(Box) {
  DECL_END_PROPS(
    (int, left, 0)
    (int, top, 0)
    (int, height, TERMREACT_FULL_HEIGHT)
    (int, width, TERMREACT_FULL_WIDTH)
    (std::function<int(int, int)>, getLeft)
    (std::function<int(int, int)>, getTop)
    (std::function<int(int, int)>, getHeight)
    (std::function<int(int, int)>, getWidth)
    (uint32_t, border, TERMREACT_NO_BORDER)
    (uint32_t, border_top, TERMREACT_NO_BORDER)
    (uint32_t, border_bottom, TERMREACT_NO_BORDER)
    (uint32_t, border_left, TERMREACT_NO_BORDER)
    (uint32_t, border_right, TERMREACT_NO_BORDER)
    (std::string, text, "")
    (uint16_t, frontground, 0)
    (uint16_t, background, 0)
  );

  MAP_STATE_TO_END_PROPS();

public:
  END_COMPONENT_WILL_MOUNT(Box) {}
  END_COMPONENT_WILL_UNMOUNT(Box) {}
  END_COMPONENT_WILL_UPDATE(next_props) {}

  CanvasSlice present(CanvasSlice canvas) {
    auto width = canvas.getWidth();
    if (PROPS(width) != TERMREACT_FULL_WIDTH) width = PROPS(width);
    else if (PROPS(getWidth)) width = PROPS(getWidth)(canvas.getWidth(), canvas.getHeight());

    auto height = canvas.getHeight();
    if (PROPS(height) != TERMREACT_FULL_HEIGHT) height = PROPS(height);
    else if (PROPS(getHeight)) height = PROPS(getHeight)(canvas.getWidth(), canvas.getHeight());

    auto left = PROPS(left);
    if (PROPS(getLeft)) left = PROPS(getLeft)(canvas.getWidth(), canvas.getHeight());

    auto top = PROPS(top);
    if (PROPS(getTop)) top = PROPS(getTop)(canvas.getWidth(), canvas.getHeight());

    auto canvas_slice = canvas.slice(left, top, width, height);

    auto border_left = PROPS(border_left) == TERMREACT_NO_BORDER ? PROPS(border) : PROPS(border_left);
    if (border_left != TERMREACT_NO_BORDER) {
      for (int y = 0; y < height; y++) {
        canvas_slice.setCell(0, y, border_left);
      }
    }

    auto border_top = PROPS(border_top) == TERMREACT_NO_BORDER ? PROPS(border) : PROPS(border_top);
    if (border_top != TERMREACT_NO_BORDER) {
      for (int x = 0; x < width; x++) {
        canvas_slice.setCell(x, 0, border_top);
      }
    }

    auto border_right = PROPS(border_right) == TERMREACT_NO_BORDER ? PROPS(border) : PROPS(border_right);
    if (border_right != TERMREACT_NO_BORDER) {
      for (int y = 0; y < height; y++) {
        canvas_slice.setCell(width - 1, y, border_right);
      }
    }

    auto border_bottom = PROPS(border_bottom) == TERMREACT_NO_BORDER ? PROPS(border) : PROPS(border_bottom);
    if (border_bottom != TERMREACT_NO_BORDER) {
      for (int x = 0; x < width; x++) {
        canvas_slice.setCell(x, height - 1, border_bottom);
      }
    }

    size_t border_top_size = border_top == TERMREACT_NO_BORDER ? 0 : 1;
    size_t border_bottom_size = border_bottom == TERMREACT_NO_BORDER ? 0 : 1;
    size_t border_left_size = border_left == TERMREACT_NO_BORDER ? 0 : 1;
    size_t border_right_size = border_right == TERMREACT_NO_BORDER ? 0 : 1;

    size_t content_width = width - border_left_size - border_right_size;
    size_t content_height = height - border_top_size - border_bottom_size;

    auto &text = PROPS(text);
    if (text.size() != 0) {
      auto text_x = border_left_size + (content_width - text.size()) / 2;
      auto text_y = border_top_size + (content_height - 1) / 2;
      canvas_slice.writeString(text_x, text_y, text, PROPS(frontground), PROPS(background));
    }

    return canvas_slice.slice(border_left_size, border_top_size, content_width, content_height);
  }
};

}