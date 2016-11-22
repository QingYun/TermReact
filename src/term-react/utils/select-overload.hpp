#pragma once

template<unsigned I>
struct choice : choice<I+1> {};
//                        ↓ modify this to allow more candidates
template<> struct choice<10> {};

struct otherwise{ otherwise(...){} };

struct select_overload_t : choice<0> {};
