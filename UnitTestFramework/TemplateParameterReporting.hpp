////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2019.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file TemplateParameterReporting.hpp
    \brief Unit tests may be templated. In this case, reporting the
           line of a failure is not necessarily sufficient to
           disambiguate the failing test. This header declares a
           struct for converting types to strings and provides some
           common specializations.
*/

#include <string>

namespace sequoia::unit_testing
{
  template<class T> struct type_to_string
  {
    static std::string str();
  };

  template<> struct type_to_string<char>
  {
    static std::string str() noexcept { return "CHAR"; }
  };
    
  template<> struct type_to_string<int>
  {
    static std::string str() noexcept { return "INT"; }
  };

  template<> struct type_to_string<std::size_t>
  {
    static std::string str() noexcept { return "STD::SIZE_T"; }
  };

  template<> struct type_to_string<float>
  {
    static std::string str() noexcept { return "FLOAT"; }
  };

  template<> struct type_to_string<double>
  {
    static std::string str() noexcept { return "DOUBLE"; }
  };
    
  template<class T, class U> struct type_to_string<std::pair<T,U>>
  {
    static std::string str() noexcept { return "PAIR<" + type_to_string<T>::str() + "," + type_to_string<U>::str() + ">"; }
  };

  template<bool> struct bool_to_string
  {
    static std::string str() noexcept { return "TRUE"; }
  };

  template<> struct bool_to_string<false>
  {
    static std::string str() noexcept { return "FALSE"; }
  };

  template<template <class...> class T> struct template_class_to_string
  {
    static std::string str();
  };
}
