////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file AllocationTestUtilities.hpp
    \brief Utilities for allocator testing
*/


namespace sequoia::unit_testing
{
    /*! \class shared_counting_allocator
      \brief Somewhat similar to std::allocator but logs (de)allocations and is without certain copy-like constructors.

      Whereas std::allocator<T> allows construction from std::allocator<U> this 
      possibility is excluded to ensure that constructors of classes taking multiple
      allocators do not confuse them internally.
   */

  template<class T, bool PropagateCopy=true, bool PropagateMove=true, bool PropagateSwap=false>
  class shared_counting_allocator
  {
  public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;    
    using propagate_on_container_copy_assignment = std::bool_constant<PropagateCopy>;
    using propagate_on_container_move_assignment = std::bool_constant<PropagateMove>;
    using propagate_on_container_swap            = std::bool_constant<PropagateSwap>;
    using is_always_equal = std::false_type;

    // Remove when libc++ is updated
    template< class U > struct rebind { using other = shared_counting_allocator<U, PropagateCopy, PropagateMove, PropagateSwap>; };    

    shared_counting_allocator()
      : m_pAllocs{std::make_shared<int>()}, m_pDeallocs{std::make_shared<int>()}
    {}
    
    constexpr shared_counting_allocator(const shared_counting_allocator&) = default;

    [[nodiscard]] T* allocate(std::size_t n)
    {
      const auto ptr{static_cast<T*>(::operator new(n * sizeof(T)))};

      if(n) ++(*m_pAllocs);

      return ptr;
    }

    void deallocate(T* p, std::size_t n)
    {
      ::operator delete(p);
      if(n) ++(*m_pDeallocs);
    }

    int allocs() const noexcept { return *m_pAllocs; }

    int deallocs() const noexcept { return *m_pDeallocs; }

    std::shared_ptr<int> preserve_dealloc_count() const noexcept { return m_pDeallocs; }

    [[nodiscard]]
    friend bool operator==(const shared_counting_allocator& lhs, const shared_counting_allocator& rhs) noexcept
    {
      return (lhs.m_pAllocs == rhs.m_pAllocs) && (lhs.m_pDeallocs == rhs.m_pDeallocs);
    }

    [[nodiscard]]
    friend bool operator!=(const shared_counting_allocator& lhs, const shared_counting_allocator& rhs) noexcept
    {
      return !(lhs == rhs);
    }
  private:
    std::shared_ptr<int> m_pAllocs{}, m_pDeallocs{};
  };
}
