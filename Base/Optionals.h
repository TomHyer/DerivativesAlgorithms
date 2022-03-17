
#pragma once

#include <optional>

// syntactic sugar for null default
template<class T_> const T_& operator+(const std::optional<T_>& opt) { return opt.get_value_or(0); }

// construct from a pointer
template<class T_> std::optional<T_> as_optional(const T_* p) { std::optional<T_> retval; if (p) retval = *p; return retval; }

