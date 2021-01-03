////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once


#include "RegularTestCore.hpp"
#include "CoreDiagnosticsUtilities.hpp"

#include <vector>
#include <string>

namespace sequoia::testing
{
  template<template<class...> class OuterType, class InnerType>
  struct scoped_beast_builder
  {
    using inner_type = InnerType;

    using inner_allocator_type = typename InnerType::allocator_type;

    using outer_allocator_type = typename inner_allocator_type::template rebind<inner_type>::other;

    using allocator_type = std::scoped_allocator_adaptor<scoped_beast_builder::outer_allocator_type, inner_allocator_type>;

    using beast = OuterType<inner_type, allocator_type>;
  };

  template<class InnerAllocator>
  using perfectly_scoped_beast
    = typename scoped_beast_builder<perfectly_normal_beast, std::basic_string<char, std::char_traits<char>, InnerAllocator>>::beast;

  template<class InnerAllocator>
  using perfectly_mixed_beast
    = typename scoped_beast_builder<perfectly_normal_beast, perfectly_sharing_beast<int, std::shared_ptr<int>, InnerAllocator>>::beast;

  template<class InnerAllocator>
  using perfectly_inverted_beast
    = typename scoped_beast_builder<perfectly_sharing_beast, perfectly_normal_beast<int, InnerAllocator>>::beast;
}
