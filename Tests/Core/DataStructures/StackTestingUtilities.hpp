////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2026.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/** \file */

#include "ContainerAdaptorTestingUtilities.hpp"

#include "sequoia/Core/DataStructures/Stack.hpp"

namespace sequoia::testing
{
  template<class T>
  struct value_tester<data_structures::stack<T>> : container_adaptor_value_tester<data_structures::stack<T>>
  {};
}
