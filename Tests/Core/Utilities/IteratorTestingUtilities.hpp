////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

#include "Iterator.hpp"

namespace sequoia::testing
{
  template<class Iterator, class DereferencePolicy>
  struct serializer<utilities::iterator<Iterator, DereferencePolicy>>
  {
    using iter_type = utilities::iterator<Iterator, DereferencePolicy>;
    
    [[nodiscard]]
    static std::string make(const iter_type iter)
    {        
      std::ostringstream os;
      os  << &*iter.base_iterator();
      return os.str();
    }
 };
}
