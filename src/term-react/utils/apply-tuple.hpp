#pragma once
#include <tuple>

namespace termreact {
namespace details {

template<int...> struct index_tuple{}; 

template<int I, typename IndexTuple, typename... Types> 
struct make_indexes_impl; 

template<int I, int... Indexes, typename T, typename ... Types> 
struct make_indexes_impl<I, index_tuple<Indexes...>, T, Types...> 
{ 
    typedef typename make_indexes_impl<I + 1, index_tuple<Indexes..., I>, Types...>::type type; 
}; 

template<int I, int... Indexes> 
struct make_indexes_impl<I, index_tuple<Indexes...> > 
{ 
    typedef index_tuple<Indexes...> type; 
}; 

template<typename ... Types> 
struct make_indexes : make_indexes_impl<0, index_tuple<>, Types...> 
{}; 

template <class ... Args>
struct ApplyTuple {

  template<class Ret, class... TArgs, class... RArgs, int... Indexes > 
  static Ret apply_helper_with( Ret (*pf)(Args..., TArgs...), index_tuple< Indexes... >, std::tuple<TArgs...>&& tup, RArgs&&... args) 
  { 
      return pf( std::forward<RArgs...>(args...), std::forward<TArgs>( std::get<Indexes>(tup))... ); 
  } 

  template<class Ret, class... TArgs, class ... RArgs> 
  static Ret apply(Ret (*pf)(Args..., TArgs...), const std::tuple<TArgs...>& tup, RArgs&&... args)
  {
      return apply_helper_with(pf, typename make_indexes<TArgs...>::type(), 
        std::tuple<TArgs...>(tup), std::forward<RArgs...>(args...));
  }

  template<class Ret, class... TArgs, class ... RArgs> 
  static Ret apply(Ret (*pf)(Args..., TArgs...), std::tuple<TArgs...>&& tup, RArgs&&... args)
  {
      return apply_helper_with(pf, typename make_indexes<TArgs...>::type(), 
        std::forward<std::tuple<TArgs...>>(tup), std::forward<RArgs...>(args...));
  }

};

template <>
struct ApplyTuple<> {
  template<class Ret, class... TArgs, int... Indexes > 
  static Ret apply_helper( Ret (*pf)(TArgs...), index_tuple< Indexes... >, std::tuple<TArgs...>&& tup) 
  { 
      return pf( std::forward<TArgs>( std::get<Indexes>(tup))... ); 
  } 

  template<class Ret, class ... TArgs> 
  static Ret apply(Ret (*pf)(TArgs...), std::tuple<TArgs...>&& tup)
  {
      return apply_helper(pf, typename make_indexes<TArgs...>::type(), std::forward<std::tuple<TArgs...>>(tup));
  }

  template<class Ret, class ... TArgs> 
  static Ret apply(Ret (*pf)(TArgs...), const std::tuple<TArgs...>& tup)
  {
      return apply_helper(pf, typename make_indexes<TArgs...>::type(), std::tuple<TArgs...>(tup));
  }
};

}
}