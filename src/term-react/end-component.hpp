#pragma once

#include "./component.hpp"

namespace termreact {
namespace details {

// base class for all endpoint components
template <typename T>
class EndComponent : public ComponentBase {
protected:
  void render_() override {
    // endpoint components do not render new components,
    // so just render children passed to it
    T* self = dynamic_cast<T*>(this);
    for (auto& pc : self->getProps().template get<T::Props::Field::children>()) {
      pc->render();
    }
  }

  void present_(Canvas& canvas, bool parent_updated) override {
    T* self = dynamic_cast<T*>(this);
    bool should_redraw = parent_updated || updated_;

    // we are pure !
    if (should_redraw) {
      // parent may choose to return a wrapped canvas to alter children's behavior
      Canvas& new_canvas = self->present(canvas);
      for (auto& pc : self->getProps().template get<T::Props::Field::children>()) {
        pc->present(new_canvas, should_redraw);
      }
    } else {
      for (auto& pc : self->getProps().template get<T::Props::Field::children>()) {
        pc->present(canvas, should_redraw);
      }
    }
  }
  
  // does nothing by default, custom component may override it to update its props
  virtual void onStoreUpdate_(const void*) {}

public:
  void onStoreUpdate(const void *next_state) override {
    T* self = dynamic_cast<T*>(this);
    onStoreUpdate_(next_state);
    for (auto& pc : self->getProps().template get<T::Props::Field::children>()) {
      pc->onStoreUpdate(next_state);
    }
  }
};

}
}
  