////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once


#include "sequoia/TestFramework/RegularTestCore.hpp"
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

    using allocator_type = std::scoped_allocator_adaptor<outer_allocator_type, inner_allocator_type>;

    using beast = OuterType<inner_type, allocator_type>;
  };
}
