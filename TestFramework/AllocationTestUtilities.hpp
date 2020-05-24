////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for allocation testing.
 */


namespace sequoia::unit_testing
{  
  /*! \brief Somewhat similar to std::allocator but logs (de)allocations via an counter
      which is shared upon copying.

      A fundamental ingredient of the allocation testing framework is the capactity to
      count the number of allocations which have occured before/after some operation and
      to compare the difference to a prediction. Experimentation has (tentatively)
      suggested that the most robust way to do this is for copies of an allocator to share
      a counter. The benefit of this is that the framework is sensitive to copies of the
      allocator being taken between the measurement points. For example, the framework can
      (and has!) detected a typo in an overload of operator== in which one of the arguments
      was accidentally taken by value, leading to unexpected allocations.
      
      There is also a more subtle difference to std:allocator<T>. Whereas the latter allows
      construction from std::allocator<U> this possibility is excluded to ensure that
      constructors of classes taking multiple allocators do not confuse them internally.

      In addition to taking the usual T as a template parameter, the class template accepts
      three bools which control whether or not the allocator is propapaged when the
      associated container is copied, mobed or swapped.
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

    // TO DO: Remove when libc++ is updated
    template< class U > struct rebind
    {
      using other = shared_counting_allocator<U, PropagateCopy, PropagateMove, PropagateSwap>;
    };    

    shared_counting_allocator()
      : m_pAllocs{std::make_shared<int>()}, m_pDeallocs{std::make_shared<int>()}
    {}
    
    shared_counting_allocator(const shared_counting_allocator&) = default;

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

    [[nodiscard]]
    int allocs() const noexcept { return *m_pAllocs; }

    [[nodiscard]]
    int deallocs() const noexcept { return *m_pDeallocs; }

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

  template<class T, class... U>
  struct type_demangler;

  template<class T, bool PropagateCopy, bool PropagateMove, bool PropagateSwap>
  struct type_demangler<shared_counting_allocator<T, PropagateCopy, PropagateMove, PropagateSwap>>
  {
    [[nodiscard]]
    static std::string make()
    {
      auto info{std::string{"[shared_counting_allocator<\n\t\t"}.append(demangle<T>()).append(",\n")};

      auto toString{[](bool b){ return b ? "true" : "false";}};

      info.append("\t\tPropagate on copy assignment = ").append(toString(PropagateCopy)).append(",\n");
      info.append("\t\tPropagate on move assignment = ").append(toString(PropagateMove)).append(",\n");
      info.append("\t\tPropagate on swap = ").append(toString(PropagateSwap)).append("\n\t>]");

      return info;
    }
  };
}
