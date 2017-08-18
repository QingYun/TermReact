#pragma once
#include <string>

namespace termreact {

class CanvasSlice;

class Canvas {
public:
  // get window dimensions
  virtual int getWidth() const = 0;
  virtual int getHeight() const = 0;

  // NOTE: default values will be overrided by concrete implementations
  virtual void clear(uint16_t foreground = 0, uint16_t background = 0) = 0;

  // set content and style for a single cell.
  // NOTE: default values will be overrided by concrete implementations
  virtual void setCell(int x, int y, uint32_t ch, uint16_t fg = 0, uint16_t bg = 0) = 0;

  // output a string starting from the position given. 
  // NOTE: default values will be overrided by concrete implementations
  virtual void writeString(int x, int y, std::string str, uint16_t fg = 0, uint16_t bg = 0) {
    for (std::size_t i = 0; i < str.size(); ++i ) {
      setCell(x + i, y, str[i], fg, bg);
    }
  }

  CanvasSlice slice(int x, int y, int w, int h);
  virtual void present() = 0;
  virtual ~Canvas() {}
};

class CanvasSlice : public Canvas {
private:
  Canvas *target_;
  int x_, y_;
  int width_, height_;
  
public:
  CanvasSlice() {}

  CanvasSlice(Canvas *target, int x, int y, int w, int h)
  : target_{target}, x_{x}, y_{y}, width_{w}, height_{h} {}
 
  // forward everything with only x and y modified
  int getWidth() const override { return width_; }
  int getHeight() const override { return height_; }
  void clear(uint16_t fg = 0, uint16_t bg = 0) override { target_->clear(fg, bg); }
  void setCell(int x, int y, uint32_t ch, uint16_t fg = 0, uint16_t bg = 0) override {
    target_->setCell(x + x_, y + y_, ch, fg, bg);
  }
  void present() override { target_->present(); }
};

CanvasSlice Canvas::slice(int x, int y, int w, int h) {
  return CanvasSlice{this, x, y, w, h};
}

}
