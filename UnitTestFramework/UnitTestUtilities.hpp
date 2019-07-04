////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2018.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file UnitTestUtilities.hpp
    \brief Utilties for use in unit tests.
*/

#include <cstddef>

namespace sequoia::unit_testing
{
  class no_default_constructor
  {
  public:
    constexpr explicit no_default_constructor(int i) : m_i{i} {}

    constexpr int get() const noexcept { return m_i; }

    constexpr friend bool operator==(const no_default_constructor& lhs, const no_default_constructor& rhs)
    {
      return lhs.get() == rhs.get();
    }

    template<class Stream> friend Stream& operator<<(Stream& stream, const no_default_constructor& ndc)
    {
      stream << ndc.get();
      return stream;
    }
  private:
    int m_i{};
  };

  /*! \class test_allocator
      \brief Similar to std::allocator but without certain copy-like constructors.

      Whereas std::allocator<T> allows construction from std::allocator<U> this 
      possibility is excluded to ensure that constructors taking multiple allocators
      do not confuse them internally.
   */

  template<class T> class custom_allocator
  {
  public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    constexpr custom_allocator() noexcept = default;
    constexpr custom_allocator(const custom_allocator&) = default;

    [[nodiscard]] T* allocate(std::size_t n)
    {
      return static_cast<T*>(::operator new(n));
    }

    void deallocate(T* p, std::size_t n)
    {
      ::operator delete(p, n);
    }

    [[nodiscard]]
    friend bool operator==(const custom_allocator& lhs, const custom_allocator& rhs)
    {
      return true;
    }

    [[nodiscard]]
    friend bool operator!=(const custom_allocator& lhs, const custom_allocator& rhs)
    {
      return !(lhs == rhs);
    }
  private:
  };  
}
