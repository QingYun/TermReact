#pragma once
#include <memory>
#include <chrono>
#include <unordered_map>
#include <functional>
#include "./provider.hpp"
#include "./termbox/termbox.h"
#include "./canvas.hpp"
#include "./component.hpp"
#include "./event.hpp"

namespace termreact {

class Termbox;
class TermboxCanvas : public Canvas {
private:
  // disable manual construction. Ensure that only Termbox can instantiate it, after initializing termbox.
  TermboxCanvas() {}

public:
  int getWidth() const override {
    return tb_width();
  }

  int getHeight() const override {
    return tb_height();
  }

  void clear(uint16_t fg = TB_DEFAULT, uint16_t bg = TB_DEFAULT) override {
    tb_set_clear_attributes(fg, bg);
    tb_clear();
  }

  void setCell(int x, int y, uint32_t ch, uint16_t fg = TB_DEFAULT, uint16_t bg = TB_DEFAULT) override {
    tb_change_cell(x, y, ch, fg, bg);
  }

  void present() override {
    tb_present();
  }

  friend class Termbox;
};

class Termbox : public Provider {
private:
  using EventHandler = void(Termbox::*)(const tb_event&);
  TermboxCanvas canvas_;
  bool should_exit_;
  std::function<void(int)> updateWindowWidth_, updateWindowHeight_;
  details::Focusable *focus_;
  std::function<void()> nextFocus_;
  std::unordered_map<int, EventHandler> event_handlers_;

  void render_(ComponentPointer root_elm) override {
    setRootElm_(std::move(root_elm));
    updateWindowWidth_(tb_width());
    updateWindowHeight_(tb_height());
  }

  void exit_() override {
    should_exit_ = true;
  }

  void updateFocus_(details::Focusable *focus) {
    if (focus == focus_) return;
    if (focus_) focus_->onLostFocus();
    focus_ = focus;
    focus_->onFocus();
  }

  void handleKey_(const tb_event& evt) {
    if (focus_ == nullptr) return;
    if (evt.key == TB_KEY_TAB) {
      return nextFocus_();     
    }
    focus_->onKeyPress(Event{ evt.mod, evt.key, evt.ch });
  }

  void handleMouse_(const tb_event&) {}

  void handleResize_(const tb_event& evt) {
    updateWindowWidth_(evt.w);
    updateWindowHeight_(evt.h);
  }

public:
  template <typename Store>
  Termbox(Store& store, int output_mode = TB_OUTPUT_256) : canvas_{}, should_exit_{false}, 
    updateWindowWidth_{[&store] (int width) { 
      store.template dispatch<ACTION(details::BuiltinAction::UpdateWindowWidth)>(width); 
    }},
    updateWindowHeight_{[&store] (int height) { 
      store.template dispatch<ACTION(details::BuiltinAction::UpdateWindowHeight)>(height); 
    }},
    focus_{nullptr},
    nextFocus_{[&store] () { 
      store.template dispatch<ACTION(details::BuiltinAction::nextFocus)>(); 
    }},
    event_handlers_{
      {TB_EVENT_KEY, &Termbox::handleKey_}, 
      {TB_EVENT_MOUSE, &Termbox::handleMouse_}, 
      {TB_EVENT_RESIZE, &Termbox::handleResize_}
    } {
    int ret = tb_init();
    if (ret) {
#ifdef NDEBUG
      // -2 in VS Code debugger
      throw std::runtime_error("tb_init() failed with error code " + std::to_string(ret));
#endif // NDEBUG
    }
    tb_clear();
    tb_select_output_mode(output_mode);

    using State = typename Store::StateType;
    store.addListener([this] (const State&, const State& next_state) {
      updateFocus_(STATE_FIELD(next_state, focusables).focus);
    });
  }

  ~Termbox() {
    tb_shutdown();
  }

  Canvas& getCanvas() override {
    return canvas_;
  }

  void runMainLoop(std::chrono::microseconds frame_duration = std::chrono::microseconds{16667 * 12}) override {
    using namespace std::chrono;
    using us = microseconds;
    using ms = milliseconds;
    tb_event ev;
    auto& canvas = getCanvas();
    while (!should_exit_) {
      auto start_time = high_resolution_clock::now();
      canvas.clear();
      getRootElm_()->present(canvas.slice(0, 0, canvas.getWidth(), canvas.getHeight()), true);
      canvas.present();
      do {
        us us_elapsed = duration_cast<us>(high_resolution_clock::now() - start_time);
        if (us_elapsed >= frame_duration) break;

        ms ms_to_wait = duration_cast<ms>(frame_duration - us_elapsed);
        int event_type = tb_peek_event(&ev, ms_to_wait.count());
        if (event_type > 0) {
          (this->*event_handlers_[event_type])(ev);
        } else if (event_type == -1) {
#ifdef NDEBUG 
          throw std::runtime_error("An error occured in tb_peek_event");
#endif // NDEBUG
        }
      } while (true);
    }
  }
};

}
