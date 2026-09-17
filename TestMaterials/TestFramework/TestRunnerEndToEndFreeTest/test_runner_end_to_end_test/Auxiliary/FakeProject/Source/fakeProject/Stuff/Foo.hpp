////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2020.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#include <type_traits>

namespace maths
{
  template<class T>
  concept floating_point = std::is_floating_point_v<T>;
}

namespace bar::baz
{
  template<maths::floating_point T>
  class foo
  {
  };
}