////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2019.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Utilities for performing allocation checks.

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

    In addition to predictions, clients must also supply a function object, per allocator
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
*/

#include "FreeCheckers.hpp"
#include "AllocationCheckersDetails.hpp"

#include "ArrayUtilities.hpp"
#include "Utilities.hpp"

namespace sequoia::testing
{  
  enum class allocation_event { copy,
                                move,
                                mutation,
                                para_copy,
                                para_move,
                                assign_prop,
                                assign,
                                move_assign,
                                copy_like_move_assign};

  /*! Type-safe wrapper for allocation predictions, to avoid mixing different allocation events */
  template<allocation_event Event>
  class alloc_prediction
  {
  public:
    constexpr alloc_prediction() = default;
    
    constexpr explicit alloc_prediction(int n) noexcept : m_Num{n} {}

    [[nodiscard]]
    constexpr operator int() const noexcept
    {
      return m_Num;
    }
  private:
    int m_Num{};
  };

  using copy_prediction                  = alloc_prediction<allocation_event::copy>;
  using move_prediction                  = alloc_prediction<allocation_event::move>;
  using mutation_prediction              = alloc_prediction<allocation_event::mutation>;
  using para_copy_prediction             = alloc_prediction<allocation_event::para_copy>;
  using para_move_prediction             = alloc_prediction<allocation_event::para_move>;
  using assign_prop_prediction           = alloc_prediction<allocation_event::assign_prop>;
  using assign_prediction                = alloc_prediction<allocation_event::assign>;  
  using move_assign_prediction           = alloc_prediction<allocation_event::move_assign>;
  using copy_like_move_assign_prediction = alloc_prediction<allocation_event::copy_like_move_assign>;

  template<allocation_event AllocEvent>
  constexpr static alloc_prediction<AllocEvent> increment_msvc_debug_count(alloc_prediction<AllocEvent> p, int i=1)
  {
    if constexpr(has_msvc_v && (iterator_debug_level() > 0))
    {
      return alloc_prediction<AllocEvent>{static_cast<int>(p) + i};
    }
    else
    {
      return p;
    }
  }

  /*! \brief class template for shifting allocation predictions, especially for MSVC debug builds.

      \anchor alloc_prediction_shifter_primary
   */

  template<class T>
  struct alloc_prediction_shifter
  {
    template<allocation_event AllocEvent>
    constexpr static alloc_prediction<AllocEvent> shift(alloc_prediction<AllocEvent> p) noexcept
    {
      return p;
    }
  };

  template<class T, class Allocator>
  struct alloc_prediction_shifter<std::vector<T, Allocator>>
  {
    template<allocation_event AllocEvent>
    constexpr static alloc_prediction<AllocEvent> shift(alloc_prediction<AllocEvent> p) noexcept
    {
      return p;
    }
    
    constexpr static copy_prediction shift(copy_prediction p)
    {
      return increment_msvc_debug_count(p);
    }

    constexpr static move_prediction shift(move_prediction p)
    {
      return increment_msvc_debug_count(p);
    }

    constexpr static assign_prop_prediction shift(assign_prop_prediction p)
    {
      return increment_msvc_debug_count(p);
    }

    constexpr static move_assign_prediction shift(move_assign_prediction p)
    {
      return increment_msvc_debug_count(p);
    }

    constexpr static para_copy_prediction shift(para_copy_prediction p)
    {
      return increment_msvc_debug_count(p);
    }

    constexpr static para_move_prediction shift(para_move_prediction p)
    {
      return increment_msvc_debug_count(p);
    }
  };

  template<class T, class Allocator>
    requires (std::is_pointer_v<T> || sequoia::is_const_pointer_v<T>)
  struct alloc_prediction_shifter<std::vector<T, Allocator>>
    : alloc_prediction_shifter<std::vector<std::decay_t<T>, Allocator>>
  {
    using alloc_prediction_shifter<std::vector<std::decay_t<T>, Allocator>>::shift;

    constexpr static assign_prediction shift(assign_prediction p)
    {
      return increment_msvc_debug_count(p);
    }

    constexpr static assign_prop_prediction shift(assign_prop_prediction p)
    {
      const auto num{
        []() {
          if constexpr (std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value)
          {
            constexpr bool moveProp{ std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value };
            constexpr bool swapProp{ std::allocator_traits<Allocator>::propagate_on_container_swap::value };
            return moveProp ? 2 :
                   swapProp ? 1 : 3;
          }
          else
          {
            return 1;
          }
        }()
      };

      return increment_msvc_debug_count(p, num);
    }
  };

  template<class T>
  struct type_to_alloc_shifter
  {
    using shifter_type = alloc_prediction_shifter<std::vector<int, std::allocator<int>>>;
  };

  template<class T>
  using type_to_alloc_shifter_t = typename type_to_alloc_shifter<T>::shifter_type;  
  
  [[nodiscard]]
  SPECULATIVE_CONSTEVAL copy_prediction
  operator "" _c(unsigned long long int n) noexcept
  {
    return copy_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL mutation_prediction
  operator "" _mu(unsigned long long int n) noexcept
  {
    return mutation_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL para_copy_prediction
  operator "" _pc(unsigned long long int n) noexcept
  {
    return para_copy_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL para_move_prediction
  operator "" _pm(unsigned long long int n) noexcept
  {
    return para_move_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL assign_prediction
  operator "" _anp(unsigned long long int n) noexcept
  {
    return assign_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL assign_prop_prediction
  operator "" _awp(unsigned long long int n) noexcept
  {
    return assign_prop_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL copy_like_move_assign_prediction
  operator "" _clm(unsigned long long int n) noexcept
  {
    return copy_like_move_assign_prediction{static_cast<int>(n)};
  }

  /*! \brief Base class for use with both plain (shared counting) allocators and std::scoped_allocator_adaptor

      This class wraps one of the essential ingredients for allocation testing: a function
      object which can acquire a copy of a container's allocator. Note that a container with
      multiple independent allocators will generally be associated with multiple instantiations
      of this class. (Each of these independent allocators may be scoped.)

      The class is designed for inheritance but not for the purpose of type erasure and so
      has a protected destructor etc.
   */
  template<movable_comparable T, alloc_getter<T> Getter>
  class allocation_info_base
  {
  public:
    using allocator_type = std::invoke_result_t<Getter, T>;
    
    explicit allocation_info_base(Getter allocGetter)
      : m_AllocatorGetter{std::move(allocGetter)}
    {}

    allocation_info_base(const allocation_info_base&) = default;

    [[nodiscard]]
    int count(const T& c) const noexcept
    {        
      return m_AllocatorGetter(c).allocs();
    }

    template<class... Args>
    [[nodiscard]]
    allocator_type make_allocator(Args&&... args) const
    {
      return {std::forward<Args>(args)...};
    }

    [[nodiscard]]
    allocator_type allocator(const T& c) const
    {
      return m_AllocatorGetter(c);
    }
  protected:
    ~allocation_info_base() = default;

    allocation_info_base(allocation_info_base&&) noexcept = default;

    allocation_info_base& operator=(const allocation_info_base&)     = default;
    allocation_info_base& operator=(allocation_info_base&&) noexcept = default;

    [[nodiscard]]
    Getter make_getter() const
    {
      return m_AllocatorGetter;
    }
  private:

    Getter m_AllocatorGetter;
  };

  /*! \brief Class for use with a container possessing a (shared counting) allocator

      By inheriting from allocation_info_base, this class is able to acquire a copy
      of an allocator from a container. On top of this, the class holds predictions
      for the various allocation events.
   */
  template<movable_comparable T, alloc_getter<T> Getter, class Predictions>
  class basic_allocation_info : public allocation_info_base<T, Getter>
  {
  private:
    using base_t = allocation_info_base<T, Getter>;
  public:
    using value_type       = T;
    using allocator_type   = typename base_t::allocator_type;
    using predictions_type = Predictions;

    struct vector_like_shifter
    {
      constexpr Predictions operator()(const Predictions& predictions) const
      {
        using ShiftType = type_to_alloc_shifter_t<T>;
        return shift<ShiftType>(predictions);
      }
    };

    template<class Shifter=vector_like_shifter>
    basic_allocation_info(Getter allocGetter, const Predictions& predictions, Shifter shifter=Shifter{})
      : base_t{std::move(allocGetter)}
      , m_Predictions{shifter(predictions)}
    {}

    [[nodiscard]]
    const Predictions& get_predictions() const noexcept
    {
      return m_Predictions;
    }
  private:
    Predictions m_Predictions;
  };

  namespace impl
  {
    template<class T, std::size_t I>
    struct shift_type_generator
    {
      using type = type_to_alloc_shifter_t<T>;
    };

    template<class T, std::size_t I>
    requires (is_tuple<typename type_to_alloc_shifter<T>::shifter_type>::value)
      struct shift_type_generator<T, I>
      {
        using shifter_type = typename type_to_alloc_shifter<T>::shifter_type;
        using type = std::remove_cvref_t<decltype(std::get<I>(std::declval<shifter_type>()))>;
      };

    template<class T, std::size_t I>
    using shift_type_generator_t = typename shift_type_generator<T, I>::type;
  }

  /*! \brief A specialization of basic_allocation_info appropriate for std::scoped_allocator_adaptor

      The essential difference to the primary template is that multiple sets of predictions must
      be supplied, one for each level within the scoped_allocator_adaptor.
   */
  template<movable_comparable T, alloc_getter<T> Getter, class Predictions>
    requires scoped_alloc<std::invoke_result_t<Getter, T>>
  class basic_allocation_info<T, Getter, Predictions>
    : public allocation_info_base<T, Getter>
  {
  private:
    using base_t = allocation_info_base<T, Getter>;
  public:
    using value_type       = T;
    using allocator_type   = typename base_t::allocator_type;
    using predictions_type = Predictions;

    constexpr static std::size_t size{alloc_count<allocator_type>::size};
    
    basic_allocation_info(Getter allocGetter, std::initializer_list<Predictions> predictions)
      : base_t{std::move(allocGetter)}
      , m_Predictions{utilities::to_array<Predictions, size>(predictions)}
    {}

    /// unpacks the scoped_allocator_adaptor, returning basic_allocation_info for the
    /// allocator at the ith level.
    template<std::size_t I>
    [[nodiscard]]
    decltype(auto) unpack() const
    {
      auto scopedGetter{[getter{this->make_getter()}](const T& c){
          return get<I>(getter(c));
        }
      };

      auto shifter{
        [](const Predictions& predictions) {          
          using shift_type = impl::shift_type_generator_t<T, I>;
          return shift<shift_type>(predictions);
        }
      };

      return basic_allocation_info<T, decltype(scopedGetter), Predictions>{scopedGetter, m_Predictions[I], shifter};
    }

  private:
    template<std::size_t I, counting_alloc... As>
    [[nodiscard]]
    static auto get(const std::scoped_allocator_adaptor<As...>& a)
    {
      if constexpr(I==0)
      {
        return a.outer_allocator();
      }
      else
      {
        return get<I-1>(a.inner_allocator());
      }
    }
      
    std::array<Predictions, size> m_Predictions;
  };
}
