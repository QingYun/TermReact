#pragma once
#include "./component.hpp"
#include "./event.hpp"

namespace termreact {
namespace details {

// base class for all endpoint components
template <typename StoreT, typename T>
class EndComponent : public ComponentBase {
protected:
  using StoreType = StoreT;
  StoreType &store_;

  EndComponent(StoreType &store) : store_{store} {}

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

#define CREATE_END_COMPONENT_CLASS(cname) \
  template <typename StoreT> class cname : \
    public ::termreact::details::EndComponent<StoreT, cname<StoreT>>, \
    public ::termreact::details::Focusable

#define BASE_END_RENDERER ::termreact::details::EndComponent<std::decay_t<decltype(*this)>>::render_

#define END_COMPONENT_WILL_MOUNT(cname) \
  cname(Props props, StoreT& store): \
    ::termreact::details::EndComponent<StoreT, cname<StoreT>>{store}, props_{std::move(props)} { \
      if (PROPS(focusable)) { \
        DISPATCH(::termreact::details::BuiltinAction::rescanFocusable)(); \
        DISPATCH(::termreact::details::BuiltinAction::stopFocusableScanning)(); \
      } \
    } \
  void onFocus() override { \
    auto &onFocus = PROPS(onFocus); \
    if (onFocus) onFocus(); \
  } \
  void onLostFocus() override { \
    auto &onLostFocus = PROPS(onLostFocus); \
    if (onLostFocus) onLostFocus(); \
  } \
  void onKeyPress(Event evt) override { \
    auto &onKeyPress = PROPS(onKeyPress); \
    if (onKeyPress) onKeyPress(evt); \
  } \
  void componentWillMount()

#define END_COMPONENT_WILL_UNMOUNT(cname) \
  ~cname() { \
    if (PROPS(focusable)) { \
      DISPATCH(::termreact::details::BuiltinAction::unregisterFocusable)( \
        STATE_FIELD(this->store_, focusables).build, \
        static_cast<::termreact::details::Focusable*>(this) \
      ); \
    } \
    customComponentWillUnmount_(); \
  } \
  void customComponentWillUnmount_()

#define END_COMPONENT_WILL_UPDATE(next_props) \
  void componentWillUpdate(const Props &next_props) { \
    if ((PROPS(focusable) != PROPS_FIELD(next_props, focusable))) { \
      if (PROPS(focusable)) { \
        DISPATCH(::termreact::details::BuiltinAction::unregisterFocusable)( \
          STATE_FIELD(this->store_, focusables).build, \
          static_cast<::termreact::details::Focusable*>(this) \
        ); \
      } else if (PROPS_FIELD(next_props, focusable)) { \
        CHUNK_DISPATCH_BEGIN; \
        DISPATCH(::termreact::details::BuiltinAction::rescanFocusable)(); \
        DISPATCH(::termreact::details::BuiltinAction::stopFocusableScanning)(); \
        CHUNK_DISPATCH_END; \
        DISPATCH(::termreact::details::BuiltinAction::selectFocus)(); \
      } \
    } \
    customComponentWillUpdate_(next_props); \
  } \
  void customComponentWillUpdate_(const Props &next_props)

#define ADD_END_COMPONENT_BUILTIN_PROPS_FIELDS(fields) \
  fields \
  (bool, focusable) \
  (::std::function<void()>, onFocus) \
  (::std::function<void()>, onLostFocus) \
  (::termreact::EventHandler, onKeyPress)
  
#define DECL_END_PROPS(fields) \
  DECL_PROPS(ADD_END_COMPONENT_BUILTIN_PROPS_FIELDS(fields));

#define MAP_STATE_TO_END_PROPS(updaters) void onStoreUpdate_(const void *next_state_p) override { \
    auto& next_state = *static_cast<const typename StoreT::StateType*>(next_state_p); \
    Props next_props = getProps(); \
    BOOST_PP_SEQ_FOR_EACH(MAP_STATE_TO_PROPS_OP, _, BOOST_PP_VARIADIC_SEQ_TO_SEQ(updaters)) \
    /* response to focusable rescanning */ \
    if (STATE_FIELD(next_state, focusables).rescanning && PROPS_FIELD(next_props, focusable)) { \
      DISPATCH(::termreact::details::BuiltinAction::registerFocusable)( \
        STATE_FIELD(next_state, focusables).build, \
        static_cast<::termreact::details::Focusable*>(this) \
      ); \
    } \
    if (next_props != getProps()) { \
      ::termreact::details::renderComponent<std::decay_t<decltype(*this)>>(*this, std::move(next_props)); \
    } \
  }
