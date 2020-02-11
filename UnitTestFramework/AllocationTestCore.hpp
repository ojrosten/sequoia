////////////////////////////////////////////////////////////////////
//                 Copyright Oliver Rosten 2020.                  //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file AllocationTestCore.hpp
    \brief Extension of unit testing framework for allocator testing
*/

#include "UnitTestCore.hpp"

namespace sequoia::unit_testing
{
  enum class allocation_test_flavour { standard, move_only};

  template<class Logger>
  class allocation_extender
  {
  public:
    allocation_extender(Logger& logger) : m_Logger{logger} {}

    allocation_extender(const allocation_extender&)            = delete;
    allocation_extender& operator=(const allocation_extender&) = delete;

    template<class T, class... Allocators/*, std::enable_if_t<!std::is_copy_constructible_v<T>, int> = 0*/>
    void check_regular_semantics(std::string_view description, T&& x, T&& y, const T& xClone, const T& yClone, move_only_allocation_info<T, Allocators>... info)
    {
      unit_testing::check_regular_semantics(combine_messages("Regular Semantics", description), m_Logger, std::move(x), std::move(y), xClone, yClone, info...);
    }

    template<class T, class Mutator, class... Allocators/*, std::enable_if_t<std::is_copy_constructible_v<T>, int> = 0*/>
    void check_regular_semantics(std::string_view description, const T& x, const T& y, Mutator m, allocation_info<T, Allocators>... info)
    {
      unit_testing::check_regular_semantics(combine_messages("Regular Semantics", description), m_Logger, x, y, m, info...);
    }
  protected:
    allocation_extender(allocation_extender&&) noexcept = default;
    ~allocation_extender() = default;

    allocation_extender& operator=(allocation_extender&&) noexcept = default;
  private:
    Logger& m_Logger;
  };
  
  template<class Logger, allocation_test_flavour Flavour, class Test>
  class basic_allocation_test : public basic_test<Logger, checker<Logger, allocation_extender<Logger>>>
  {
  public:
    basic_allocation_test(std::string_view name, Test& test)
      : basic_test<Logger, checker<Logger, allocation_extender<Logger>>>{name}
      , m_Test{test}
    {}

  protected:
    void run_tests() final
    {
      do_allocation_tests();
    }

    void do_allocation_tests()
    {
      if constexpr(Flavour == allocation_test_flavour::standard)
      {
        m_Test.template test_allocation<false, false, false>();
        m_Test.template test_allocation<false, false, true>();
        m_Test.template test_allocation<false, true, false>();
        m_Test.template test_allocation<false, true, true>();
        m_Test.template test_allocation<true, false, false>();
        m_Test.template test_allocation<true, false, true>();
        m_Test.template test_allocation<true, true, false>();
        m_Test.template test_allocation<true, true, true>();
      }
      else
      {
        m_Test.template test_allocation<false, false>();
        m_Test.template test_allocation<false, true>();
        m_Test.template test_allocation<true, false>();
        m_Test.template test_allocation<true, true>();
      }
    }

  private:
    Test& m_Test;
  };
   
  template<test_mode Mode, class Test>
  using regular_allocation_test = basic_allocation_test<unit_test_logger<Mode>, allocation_test_flavour::standard, Test>;

  template<class Test> using allocation_test                = regular_allocation_test<test_mode::standard, Test>;
  template<class Test> using allocation_false_negative_test = regular_allocation_test<test_mode::false_negative, Test>;
  template<class Test> using allocation_false_positive_test = regular_allocation_test<test_mode::false_positive, Test>;

  template<test_mode Mode, class Test>
  using regular_move_only_allocation_test = basic_allocation_test<unit_test_logger<Mode>, allocation_test_flavour::move_only, Test>;

  template<class Test> using move_only_allocation_test                = regular_move_only_allocation_test<test_mode::standard, Test>;
  template<class Test> using move_only_allocation_false_negative_test = regular_move_only_allocation_test<test_mode::false_negative, Test>;
  template<class Test> using move_only_allocation_false_positive_test = regular_move_only_allocation_test<test_mode::false_positive, Test>;

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
