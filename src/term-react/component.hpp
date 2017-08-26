#pragma once
#include <type_traits>
#include <string>
#include <typeinfo>
#include <deque>
#include <vector>
#include <functional>
#include <algorithm>
#include <boost/preprocessor/seq.hpp>
#include <boost/preprocessor/tuple.hpp>
#include <boost/preprocessor/variadic.hpp>
#include "./utils/immutable-struct.hpp"
#include "./canvas.hpp"
#include "./action.hpp"

namespace termreact {
namespace details {

class ComponentBase {
protected:
  // need to redraw? set by render() and clear by present()
  bool updated_;

  virtual void render_() = 0;
  virtual void present_(Canvas&, bool) = 0;

public:
  virtual ~ComponentBase() {}

  // generate / update child components to reflect the current props & state
  void render() {
    render_();
    updated_ = true;
  }

  // draw on canvas. Decide whether to actually draw something based on this->updated_ and parent_updated
  void present(Canvas& canvas, bool parent_updated) {
    present_(canvas, parent_updated);
    updated_ = false;
  }

  // pass the pointer around and concrete component can convert it to the actual state type
  virtual void onStoreUpdate(const void* next_state) = 0;
};

template <typename T> class ComponentAccessor;

// base class for all non-endpoint components
template <typename StoreT>
class Component : public ComponentBase {
private:
  std::size_t node_type_;
  std::unique_ptr<ComponentBase> node_;

protected:
  using StoreType = StoreT;
  StoreType& store_;

  // does nothing by default, custom component may override it to update its props
  virtual void onStoreUpdate_(const void*) {}

  void present_(Canvas& canvas, bool parent_updated) {
    // only components handle present, so just forward the call
    node_->present(canvas, parent_updated || updated_);
  }
public:
  Component(StoreT& store) : store_(store) {}
  
  void onStoreUpdate(const void *next_state) override {
    onStoreUpdate_(next_state);
    node_->onStoreUpdate(next_state);
  }
  
  friend class details::ComponentAccessor<StoreType>;
};

class ComponentHolder;
using Child = std::shared_ptr<ComponentHolder>;
using Children = std::deque<Child>;
using ChildrenCreator = std::function<void(bool*, Children*)>;

template <typename T>
void renderComponent(ComponentBase& component, typename T::Props next_props) {
  T* target = dynamic_cast<T*>(&component);
  target->componentWillUpdate(next_props);
  target->setProps(std::move(next_props));
  target->render();
}

template <typename ChildT, typename StoreT>
std::unique_ptr<ComponentBase> createComponent(typename ChildT::Props next_props, StoreT& store) {
  auto target = std::make_unique<ChildT>(std::move(next_props), store);
  target->componentWillMount();
  target->render();
  return std::unique_ptr<ComponentBase>(target.release());
}

// container for child component
// child components are only rendered when we really need it
class ComponentHolder : public ComponentBase {
public:
  // try to update component's props.
  // if there is any change made, store the new props in ComponentHolderT::next_props and return true
  // if no changes, return false
  using Updater = std::function<bool(ComponentBase*, ComponentHolder*)>;

private:
  // needs to be unique within, at least, same-level children
  const std::string id_;
  // comes from typeid(ComponentType).hash_code()
  std::size_t component_type_hash_;
  std::vector<Updater> updaters_;

protected:
  std::unique_ptr<ComponentBase> component_;

public:
  ComponentHolder(std::string id, std::size_t type_hash, Updater updater)
    : id_{id}, component_type_hash_{type_hash}, updaters_{std::move(updater)} {}

  bool calculateNextProps(ComponentBase *component) {
    return std::accumulate(std::begin(updaters_), std::end(updaters_), false, 
      [this, component] (bool updated, Updater updater) {
        return updater(component, this) || updated;
      }
    );
  }

  void addPropsUpdater(Updater updater) {
    updaters_.emplace_back(std::move(updater));
  }

  static Children mergeChildren(Children next_children, const Children& prev_children) {
    for (auto& pc : next_children) {
      auto prev_ppc = std::find_if(std::begin(prev_children), std::end(prev_children), [&pc] (const Child& prev_pc) {
        return pc->component_type_hash_ == prev_pc->component_type_hash_ && pc->id_ == prev_pc->id_;
      });

      if (prev_ppc == std::end(prev_children)) {
        // always assume an unrendered component is different from the new one
        // defer props calculation to the final render call as it's possible for a parent component 
        // to choose to not render some of its children
        continue;
      }

      // Note: to update, leave pc untouched; to keep, set pc = prev_pc
      const Child& prev_pc = *prev_ppc;
      if (pc == prev_pc) {
        // parent chooses to not update
        // skip children-forwarding in the middle of a component tree;
        continue;
      }

      if (!prev_pc->component_) {
        // update
        // no previous component, which means:
        // 1) never rendered or
        // 2) parent found it needs to be updated and has already moved it into pc
        //    Note: all children refers to same holder objects after rendering,
        //          i.e. sub-components do not create their own component holder,
        //          they just keep a reference to holders passed from parents.
        continue;
      }

      if (pc->calculateNextProps(prev_pc->component_.get())) {
        // update, property changed
        // Note: the next_props calculated in pc->calculateNextProps() will be memorized,
        //       so there will be no recalcuating when rendering
        pc->component_ = std::move(prev_pc->component_);
      } else {
        // do not update, no property changes
        // the next_props moved to the component during the last render
        // so no render will happen to this component again
        pc = prev_pc;
      }
    }
    return next_children;
  }
};

template <typename ChildT, typename StoreT>
class ComponentHolderT : public ComponentHolder {
private:
  typename ChildT::Props next_props_;
  StoreT& store_;

protected:
  void render_() override {
    // actually create the component in the first rendering
    if (!component_) {
      calculateNextProps(nullptr);
      component_ = createComponent<ChildT>(std::move(next_props_), store_);
      return;
    }
    if (next_props_) {
      renderComponent<ChildT>(*component_.get(), std::move(next_props_));
      return;
    }
  }

  void present_(Canvas& canvas, bool parent_updated) override {
    if (component_) component_->present(canvas, parent_updated);
  }

public:
  ComponentHolderT(std::string id, StoreT& store, Updater updater)
  : ComponentHolder{id, typeid(ChildT).hash_code(), std::move(updater)}, next_props_{TrivalConstruction_t{}}, store_{store} {}

  void setNextProps(typename ChildT::Props next_props) {
    next_props_ = std::move(next_props);
  }
  
  void onStoreUpdate(const void *next_state) override {
    component_->onStoreUpdate(next_state);
  }
};

// provide access to private members of a component
template <typename T>
class ComponentAccessor {
private:
  Component<T> *pc_;
public:
  ComponentAccessor(Component<T> *pc) : pc_{pc} {}

  std::unique_ptr<ComponentBase>& getNode() const { return pc_->node_; }
  void setNode(std::unique_ptr<ComponentBase> node) { pc_->node_ = std::move(node); }

  std::size_t getType() const { return pc_->node_type_; }
  void setType(std::size_t node_type) { pc_->node_type_ = node_type; }

  T& getStore() const { return pc_->store_; }
};

// handler of component property update
template <typename T>
class ComponentRendererHelper {
public:
  using Props = typename T::Props;

private:
  const ChildrenCreator& children_creator_;
  std::function<Props(Props)> props_updater_;

public:
  ComponentRendererHelper(const ChildrenCreator& children_creator, std::function<Props(Props)> props_updater)
  : children_creator_{children_creator}, props_updater_{props_updater} {}

  Props updateProps(Props prev_props) const {
    auto next_props = props_updater_(std::move(prev_props));

    bool trivial_creator = true;
    children_creator_(&trivial_creator, nullptr);
    if (!trivial_creator) {
      Children next_children;
      children_creator_(&trivial_creator, &next_children);
      next_props.template update<Props::Field::children>(
        ComponentHolder::mergeChildren(std::move(next_children), next_props.template get<Props::Field::children>())
      );
    }

    return next_props;
  }
};

// set or update a component's render result
// should be only used in component.render()
template <typename ChildT, typename StoreT>
class ComponentRenderer {
private:
  using Props = typename ComponentRendererHelper<ChildT>::Props;

  ComponentRendererHelper<ChildT> helper_;
  ComponentAccessor<StoreT> parent_;

public:
  ComponentRenderer(Component<StoreT>* parent, ChildrenCreator& children_creator, std::function<Props(Props)> props_updater)
  : helper_{children_creator, props_updater}, parent_{parent} {}

  ~ComponentRenderer() {
    if (typeid(ChildT).hash_code() != parent_.getType() || !parent_.getNode()) {
      // a different type component needed or no component existing at all
      parent_.setType(typeid(ChildT).hash_code());
      parent_.setNode(createComponent<ChildT>(helper_.updateProps(Props{}), parent_.getStore()));
      return;
    }

    ChildT* target = dynamic_cast<ChildT*>(parent_.getNode().release());
    auto next_props = helper_.updateProps(target->getProps());
    if (next_props != target->getProps()) {
      renderComponent<ChildT>(*target, std::move(next_props));
    }
    parent_.setNode(std::unique_ptr<ComponentBase>{target});
  }
};

// add child holders to a component rendered by ComponentRenderer
template <typename ChildT, typename StoreT>
class ChildRenderer {
private:
  using Props = typename ComponentRendererHelper<ChildT>::Props;

  ChildrenCreator& children_creator_;
  std::function<Props(Props)> props_updater_;
  std::string id_;
  Children& next_children_;
  StoreT& store_;

public:
  ChildRenderer(std::string id, Children& next_children, ChildrenCreator& children_creator,
                std::function<Props(Props)> props_updater, StoreT& store)
  : children_creator_{children_creator}, props_updater_{props_updater},
    id_{id}, next_children_{next_children}, store_{store} {}

  ~ChildRenderer() {
    next_children_.emplace_front(new ComponentHolderT<ChildT, StoreT>{id_, store_,
      [c{std::move(children_creator_)}, u{std::move(props_updater_)}]
      (ComponentBase *pc, ComponentHolder *ph) {
        ComponentRendererHelper<ChildT> helper{c, u};
        auto component = dynamic_cast<ChildT*>(pc);
        auto holder = dynamic_cast<ComponentHolderT<ChildT, StoreT>*>(ph);

        auto next_props = helper.updateProps(component ? component->getProps() : Props{});
        if (!component || next_props != component->getProps()) {
          holder->setNextProps(std::move(next_props));
          return true;
        }
        return false;
      }});
  }
};

// similar to ChildRenderer, but only for forwarding existing
// child holders, which is in most cases props.children
template <typename VecT>
class ArrayRenderer {
private:
  Children& next_children_;
  VecT components_;

public:
  ArrayRenderer(Children& next_children, VecT components)
  : next_children_{next_children}, components_{std::move(components)} {}

  ~ArrayRenderer() {
    for (auto it = std::rbegin(components_); it != std::rend(components_); ++it) {
      next_children_.emplace_front(std::move(*it));
    }
  }
};

template <typename VecT>
ArrayRenderer<VecT> createArrayRender(Children& next_children, VecT&& components) {
  VecT copy_components{std::forward<VecT>(components)};
  return ArrayRenderer<VecT>{next_children, std::move(copy_components)};
}

}

using ComponentPointer = std::unique_ptr<details::ComponentBase>;

#define DECL_PROPS(fields) \
  public: \
    IMMUTABLE_STRUCT(Props, BOOST_PP_SEQ_PUSH_BACK(BOOST_PP_VARIADIC_SEQ_TO_SEQ(fields), \
                                                  (::termreact::details::Children, children))); \
    const Props& getProps() const { return props_; } \
    void setProps(Props next_props) { props_ = ::std::move(next_props); } \
  private: \
    Props props_

#define __DETAILS_CHILDREN_CREATOR_NAME(C) BOOST_PP_CAT(__, BOOST_PP_CAT(__LINE__, CHILDREN_CREATOR__))
#define __DETAILS_COMPONENT_RENDERER_NAME(C) BOOST_PP_CAT(__, BOOST_PP_CAT(__LINE__, COMPONENT_RENDERER__))
#define __DETAILS_ARRAY_RENDERER_NAME BOOST_PP_CAT(__, BOOST_PP_CAT(__LINE__, ARRAY_RENDERER))

#define __DETAILS_PROPS_UPDATER_OP(s, prefix, tuple) \
  props.template update<prefix::BOOST_PP_TUPLE_ELEM(0, tuple)>(BOOST_PP_TUPLE_ELEM(1, tuple));
#define __DETAILS_PROPS_UPDATER(attr) [this] (auto props) { \
    BOOST_PP_SEQ_FOR_EACH(__DETAILS_PROPS_UPDATER_OP, decltype(props)::Field, attr) \
    return props; \
  }
#define __DETAILS_EMPTY_PROPS_UPDATER(attr) [] (auto props) { return props; }

#define ATTRIBUTES(attr) \
  BOOST_PP_IF(BOOST_PP_SEQ_SIZE(BOOST_PP_VARIADIC_SEQ_TO_SEQ(attr)), \
    __DETAILS_PROPS_UPDATER, \
    __DETAILS_EMPTY_PROPS_UPDATER)(BOOST_PP_VARIADIC_SEQ_TO_SEQ(attr)) \

#define RENDER_MAIN_COMPONENT(C, attr) \
  ::termreact::details::ChildrenCreator __DETAILS_CHILDREN_CREATOR_NAME(C); \
  ::termreact::details::ComponentRenderer<C<StoreT>, StoreT> __DETAILS_COMPONENT_RENDERER_NAME(C) { \
    this, __DETAILS_CHILDREN_CREATOR_NAME(C), attr \
  }; \
  __DETAILS_CHILDREN_CREATOR_NAME(C) = [this] (bool *trivial_creator, ::termreact::details::Children *next_children)

#define RENDER_CHILD_COMPONENT(C, id, attr) \
  if (*trivial_creator) { *trivial_creator = false; return; } \
  ::termreact::details::ChildrenCreator __DETAILS_CHILDREN_CREATOR_NAME(C); \
  ::termreact::details::ChildRenderer<C<StoreT>, StoreT> __DETAILS_COMPONENT_RENDERER_NAME(C) { \
    id, *next_children, __DETAILS_CHILDREN_CREATOR_NAME(C), attr, \
    ::termreact::details::ComponentAccessor<StoreT>{this}.getStore() \
  }; \
  __DETAILS_CHILDREN_CREATOR_NAME(C) = [this] (bool *trivial_creator, ::termreact::details::Children *next_children)

#define RENDER_COMPONENT_ARRAY(arr) \
  if (*trivial_creator) { *trivial_creator = false; return; } \
  auto __DETAILS_ARRAY_RENDERER_NAME = ::termreact::details::createArrayRender(*next_children, arr)

#define RENDER_COMPONENT(...) \
  BOOST_PP_IF(BOOST_PP_EQUAL(1, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)), RENDER_COMPONENT_ARRAY, \
  BOOST_PP_IF(BOOST_PP_EQUAL(2, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)), RENDER_MAIN_COMPONENT, \
  BOOST_PP_IF(BOOST_PP_EQUAL(3, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)), RENDER_CHILD_COMPONENT, \
  BOOST_PP_EMPTY)))(__VA_ARGS__)

#define NO_CHILDREN do { (void)trivial_creator; (void)next_children; } while(0);

#define CREATE_COMPONENT_CLASS(cname) \
  template <typename StoreT> class cname : public ::termreact::details::Component<StoreT>

#define CREATE_END_COMPONENT_CLASS(cname) \
  template <typename StoreT> class cname : public ::termreact::details::EndComponent<cname<StoreT>>

#define BASE_END_RENDERER ::termreact::details::EndComponent<std::decay_t<decltype(*this)>>::render_

#define COMPONENT_WILL_MOUNT(cname) \
  cname(Props props, StoreT& store) : ::termreact::details::Component<StoreT>{store}, props_{std::move(props)} {} \
  void componentWillMount()

#define END_COMPONENT_WILL_MOUNT(cname) \
  cname(Props props, StoreT&) : props_{std::move(props)} {} \
  void componentWillMount()
  
#define STATE_FIELD_IMPL(state, F) state.template get<std::decay_t<decltype(state)>::Field::F>()
#define STATE_FIELD(...) \
  STATE_FIELD_IMPL( \
    BOOST_PP_IF(BOOST_PP_EQUAL(1, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)), next_state, BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__)), \
    BOOST_PP_IF(BOOST_PP_EQUAL(1, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__)), \
                BOOST_PP_VARIADIC_ELEM(0, __VA_ARGS__), BOOST_PP_VARIADIC_ELEM(1, __VA_ARGS__)) \
  )

#define MAP_STATE_TO_PROPS_OP(s, d, tuple) \
  next_props.template update<Props::Field::BOOST_PP_TUPLE_ELEM(0, tuple)>(BOOST_PP_TUPLE_ELEM(1, tuple));
// updaters: (props_field, expr)(...)...
#define MAP_STATE_TO_PROPS(updaters) void onStoreUpdate_(const void *next_state_p) override { \
    auto& next_state = *static_cast<const typename StoreT::StateType*>(next_state_p); \
    Props next_props = getProps(); \
    BOOST_PP_SEQ_FOR_EACH(MAP_STATE_TO_PROPS_OP, _, BOOST_PP_VARIADIC_SEQ_TO_SEQ(updaters)) \
    if (next_props != getProps()) { \
      ::termreact::details::renderComponent<std::decay_t<decltype(*this)>>(*this, std::move(next_props)); \
    } \
  }

#define PROPS(field) this->getProps().template get<Props::Field::field>()
#define PROPS_FIELD(p, field) p.template get<std::decay_t<decltype(p)>::Field::field>()
#define DISPATCH(...) this->store_.template dispatch<ACTION(__VA_ARGS__)>
#define CHUNK_DISPATCH_BEGIN this->store_.startChunkDispatch();
#define CHUNK_DISPATCH_END this->store_.endChunkDispatch();

}
