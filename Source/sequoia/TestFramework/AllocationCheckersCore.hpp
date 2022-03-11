////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2022.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \brief Core components for the Allocation Testing framework

    The allocation testing framework is built upon the idea of supplying and testing
    predictions for particular allocation events, in particular:

    - copying
    - mutating
    - para copy (use of the copy-like constructor taking an allocator as an extra argument)
    - para move (use of the move-like constructor taking an allocator as an extra argument)
    - assignment with/without propagation of the allocator
    - swap with/without propagation of the allocator

    To help with readability of actual tests, various user defined literals are introduced
    so that, for example, 1_c may be used to mean a prediction of 1 allocation for a copy
    event. This is more expressive than just '1' and less verbose than copy_prediction{1},
    particularly bearing in mind that often several predictions are supplied together.

    In addition to predictions, clients must also supply a function object, per allocator,
    which consumes a container and returns a copy of the allocator. With these ingredients,
    together with a container which uses the shared_counting_allocator, the following
    scenario may be realized:

    A extract a copy of the allocator
    B acquire the number of allocations
    C perform an operation with an expected number of allocations
    D acquire the number of allocations
    E compare the prediction to the difference between D and B.

    Note that the framework is designed to work smoothly with std::scoped_allocator_adaptor
    and/or with containers containing multiple allocators, scoped or otherwise.

    Finally, the framework provides a mechanism for systematically shifting predictions. This
    is very useful for e.g. the MSVC debug build which systematically performs extra allocations.
    Thus, rather than clients having to specify different predictions on different platforms - or
    even on a single platform with different build settings - the 'natural' prediction is 
    supplied and the framework shifts this appropriately. The implementation is sufficiently
    for flexible clients to define their own allocation shifts, should the need arise.
 */

#include "sequoia/TestFramework/AllocationCheckersTraits.hpp"

#include <string>

namespace sequoia::testing
{
  enum class container_tag { x, y };

  template<container_tag tag>
  struct container_tag_constant : std::integral_constant<container_tag, tag>
  {};

  [[nodiscard]]
  std::string to_string(container_tag tag);

  template<movable_comparable T, alloc_getter<T> Getter>
  class allocation_info;

  enum class null_allocation_event { comparison, spectator, serialization, swap };

  /*! Type-safe wrapper for allocation predictions, to avoid mixing different allocation events */
  template<auto Event>
  class alloc_prediction
  {
  public:
    constexpr alloc_prediction() = default;

    constexpr alloc_prediction(int unshifted, int delta = {}) noexcept
      : m_Unshifted{unshifted}
      , m_Prediction{m_Unshifted + delta}
    {}

    [[nodiscard]]
    constexpr int value() const noexcept
    {
      return m_Prediction;
    }

    [[nodiscard]]
    constexpr int unshifted() const noexcept
    {
      return m_Unshifted;
    }
  private:
    int m_Unshifted{}, m_Prediction{};
  };

  template<class T>
  struct allocation_count_shifter;
}
