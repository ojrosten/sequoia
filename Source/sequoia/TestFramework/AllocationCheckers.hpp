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

#include "sequoia/TestFramework/FreeCheckers.hpp"
#include "sequoia/TestFramework/AllocationCheckersDetails.hpp"

#include "sequoia/Core/Utilities/ArrayUtilities.hpp"
#include "sequoia/Core/Meta/Utilities.hpp"

namespace sequoia::testing
{
  enum class individual_allocation_event {
    initialization,
    copy,
    move,
    mutation,
    para_copy,
    para_move
  };

  enum class assignment_allocation_event {
    assign_prop,
    assign,
    move_assign,
    copy_like_move_assign
  };

  template<auto To, auto From>
  [[nodiscard]]
  constexpr alloc_prediction<To> convert(alloc_prediction<From> p) noexcept
  {
    return {p.unshifted(), p.value() - p.unshifted()};
  }

  using initialization_prediction        = alloc_prediction<individual_allocation_event::initialization>;
  using copy_prediction                  = alloc_prediction<individual_allocation_event::copy>;
  using move_prediction                  = alloc_prediction<individual_allocation_event::move>;
  using mutation_prediction              = alloc_prediction<individual_allocation_event::mutation>;
  using para_copy_prediction             = alloc_prediction<individual_allocation_event::para_copy>;
  using para_move_prediction             = alloc_prediction<individual_allocation_event::para_move>;
  using assign_prop_prediction           = alloc_prediction<assignment_allocation_event::assign_prop>;
  using assign_prediction                = alloc_prediction<assignment_allocation_event::assign>;
  using move_assign_prediction           = alloc_prediction<assignment_allocation_event::move_assign>;
  using copy_like_move_assign_prediction = alloc_prediction<assignment_allocation_event::copy_like_move_assign>;

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
  initialization_prediction operator "" _init(unsigned long long int n) noexcept
  {
    return initialization_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
  copy_prediction operator "" _c(unsigned long long int n) noexcept
  {
    return copy_prediction{ static_cast<int>(n) };
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
  mutation_prediction operator "" _mu(unsigned long long int n) noexcept
  {
    return mutation_prediction{ static_cast<int>(n) };
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
  para_copy_prediction operator "" _pc(unsigned long long int n) noexcept
  {
    return para_copy_prediction{ static_cast<int>(n) };
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
  para_move_prediction operator "" _pm(unsigned long long int n) noexcept
  {
    return para_move_prediction{ static_cast<int>(n) };
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
  assign_prediction operator "" _anp(unsigned long long int n) noexcept
  {
    return assign_prediction{ static_cast<int>(n) };
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
  assign_prop_prediction operator "" _awp(unsigned long long int n) noexcept
  {
    return assign_prop_prediction{ static_cast<int>(n) };
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
    move_assign_prediction operator "" _ma(unsigned long long int n) noexcept
  {
    return move_assign_prediction{static_cast<int>(n)};
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
  copy_like_move_assign_prediction operator "" _clm(unsigned long long int n) noexcept
  {
    return copy_like_move_assign_prediction{ static_cast<int>(n) };
  }

  enum class scoped_prediction_corrections { num_containers, post_mutation };

  template<scoped_prediction_corrections CorrectionFlavour>
  class corrections
  {
  public:
    constexpr explicit corrections(std::size_t n) : m_Num{n} {}

    [[nodiscard]]
    constexpr int value() const noexcept
    {
      return static_cast<int>(m_Num);
    }

    [[nodiscard]]
    friend auto operator<=>(const corrections&, const corrections&) noexcept = default;

  private:
    std::size_t m_Num{};
  };

  using number_of_containers     = corrections<scoped_prediction_corrections::num_containers>;
  using post_mutation_correction = corrections<scoped_prediction_corrections::post_mutation>;

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
  number_of_containers operator "" _containers(unsigned long long int n) noexcept
  {
    return number_of_containers{ static_cast<std::size_t>(n) };
  }

  [[nodiscard]]
  SPECULATIVE_CONSTEVAL
  post_mutation_correction operator "" _postmutation(unsigned long long int n) noexcept
  {
    return post_mutation_correction{static_cast<std::size_t>(n)};
  }

  struct container_counts
  {
    constexpr container_counts(number_of_containers x, number_of_containers y, post_mutation_correction postMutation) noexcept
      : num_x{x}
      , num_y{y}
      , mutation_correction{postMutation}
    {}

    number_of_containers num_x, num_y;
    post_mutation_correction mutation_correction;
  };

  namespace allocation_equivalence_classes
  {
    template<class Allocator>
    struct container_of_values {};

    template<class Allocator>
    struct container_of_pointers {};
  }

  template<auto AllocEvent>
  [[nodiscard]]
  constexpr alloc_prediction<AllocEvent> increment_msvc_debug_count(alloc_prediction<AllocEvent> p, int val) noexcept
  {
    if constexpr (with_msvc_v && (iterator_debug_level() > 0))
    {
      const int unshifted{p.unshifted()};
      return alloc_prediction<AllocEvent>{unshifted, val};
    }
    else
    {
      return p;
    }
  }

  template<class T>
  struct allocation_count_shifter
  {
    template<auto NullAllocEvent>
    [[nodiscard]]
    constexpr static int shift(int count, const alloc_prediction<NullAllocEvent>&) noexcept
    {
      return count;
    }
  };

  enum class container_tag { x, y };
  enum class top_level { yes, no };
  enum class has_alloc_init_predictions { yes, no };

  /*! \brief class template for shifting allocation predictions, especially for MSVC debug builds.

      \anchor alloc_prediction_shifter_primary
   */

  template<class T>
  class alloc_prediction_shifter;

  template<class Allocator>
  class alloc_prediction_shifter<allocation_equivalence_classes::container_of_values<Allocator>>
  {
  public:
    alloc_prediction_shifter(const container_counts& data, top_level level)
      : m_Counts{data}
      , m_TopLevel{level}
    {}

    [[nodiscard]]
    constexpr copy_prediction shift(copy_prediction p, container_tag tag) const noexcept
    {
      const auto& c{tag == container_tag::x ? m_Counts.num_x : m_Counts.num_y};
      return increment_msvc_debug_count(p, c.value());
    }

    [[nodiscard]]
    constexpr move_prediction shift(move_prediction p) const noexcept
    {
      return increment_msvc_debug_count(p, is_top_level() ? 1 : 0);
    }

    [[nodiscard]]
    constexpr para_copy_prediction shift(para_copy_prediction p) const noexcept
    {
      return increment_msvc_debug_count(p, m_Counts.num_y.value());
    }

    [[nodiscard]]
    constexpr para_move_prediction shift(para_move_prediction p) const noexcept
    {
      return increment_msvc_debug_count(p, m_Counts.num_y.value());
    }

    [[nodiscard]]
    constexpr mutation_prediction shift(mutation_prediction p) const noexcept
    {
      return increment_msvc_debug_count(p, m_Counts.mutation_correction.value());
    }

    [[nodiscard]]
    constexpr assign_prop_prediction shift(assign_prop_prediction p) const noexcept
    {
      return increment_msvc_debug_count(p, m_Counts.num_y.value());
    }

    [[nodiscard]]
    constexpr move_assign_prediction shift(move_assign_prediction p) const noexcept
    {
      return increment_msvc_debug_count(p, is_top_level() ? 1 : 0);
    }

    [[nodiscard]]
    constexpr assign_prediction shift(assign_prediction p) const noexcept
    {
      return increment_msvc_debug_count(p, is_top_level() ? 0 : m_Counts.num_y.value());
    }

    [[nodiscard]]
    constexpr copy_like_move_assign_prediction shift(copy_like_move_assign_prediction p) const noexcept
    {
      const bool doIncrement{!is_top_level() && (m_Counts.num_y.value() > m_Counts.num_x.value())};
      return increment_msvc_debug_count(p, doIncrement ? m_Counts.num_y.value() : 0);
    }

    [[nodiscard]]
    constexpr const container_counts& counts() const noexcept
    {
      return m_Counts;
    }

    [[nodiscard]]
    constexpr bool is_top_level() const noexcept
    {
      return m_TopLevel == top_level::yes;
    }
  private:
    container_counts m_Counts;
    top_level m_TopLevel;
  };

  template<class Allocator>
  class alloc_prediction_shifter<allocation_equivalence_classes::container_of_pointers<Allocator>>
    : public alloc_prediction_shifter<allocation_equivalence_classes::container_of_values<Allocator>>
  {
  public:
    using alloc_prediction_shifter<allocation_equivalence_classes::container_of_values<Allocator>>::alloc_prediction_shifter;
    using alloc_prediction_shifter<allocation_equivalence_classes::container_of_values<Allocator>>::shift;

    [[nodiscard]]
    constexpr assign_prediction shift(assign_prediction p) const noexcept
    {
      return increment_msvc_debug_count(p, this->counts().num_y.value());
    }

    [[nodiscard]]
    constexpr assign_prop_prediction shift(assign_prop_prediction p) const noexcept
    {
      const auto val{
        []() {
          constexpr bool copyProp{std::allocator_traits<Allocator>::propagate_on_container_copy_assignment::value};
          constexpr bool moveProp{std::allocator_traits<Allocator>::propagate_on_container_move_assignment::value};
          constexpr bool swapProp{std::allocator_traits<Allocator>::propagate_on_container_swap::value};
          return !copyProp ? 1 :
                  moveProp ? 2 :
                  swapProp ? 1 : 3;
        }()
      };

      return increment_msvc_debug_count(p, this->counts().num_y.value()*val);
    }
  };

  template<movable_comparable T, alloc_getter<T> Getter>
  struct alloc_equivalence_class_generator
  {
    using allocator = std::remove_cvref_t<std::invoke_result_t<Getter, T>>;
    using type = allocation_equivalence_classes::container_of_values<allocator>;
  };

  template<movable_comparable T, alloc_getter<T> Getter>
    requires requires { Getter::alloc_equivalence_class; }
  struct alloc_equivalence_class_generator<T, Getter>
  {
    using type = typename Getter::alloc_equivalence_class;
  };

  template<movable_comparable T, alloc_getter<T> Getter>
  using alloc_equivalence_class_generator_t = typename alloc_equivalence_class_generator<T, Getter>::type;

  template<class T>
  struct type_to_allocation_predictions;

  template<class T>
  using type_to_allocation_predictions_t = typename type_to_allocation_predictions<T>::predictions_type;

  template<class T>
  using type_to_inner_allocation_predictions_t = typename type_to_allocation_predictions<T>::inner_predictions_type;

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

    constexpr explicit allocation_info_base(Getter allocGetter)
      : m_AllocatorGetter{std::move(allocGetter)}
    {}

    constexpr allocation_info_base(const allocation_info_base&) = default;

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

    constexpr allocation_info_base(allocation_info_base&&) noexcept = default;

    constexpr allocation_info_base& operator=(const allocation_info_base&)     = default;
    constexpr allocation_info_base& operator=(allocation_info_base&&) noexcept = default;

    [[nodiscard]]
    constexpr Getter make_getter() const
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
  template<movable_comparable T, alloc_getter<T> Getter>
  class allocation_info : public allocation_info_base<T, Getter>
  {
  private:
    using base_t = allocation_info_base<T, Getter>;
  public:
    using value_type             = T;
    using allocator_type         = typename base_t::allocator_type;
    using predictions_type       = type_to_allocation_predictions_t<T>;
    using inner_predictions_type = type_to_inner_allocation_predictions_t<T>;

    constexpr allocation_info(Getter allocGetter, const predictions_type& predictions)
      : base_t{std::move(allocGetter)}
      , m_Predictions{prediction_shifter{}(predictions)}
    {}

    constexpr allocation_info(Getter allocGetter, const inner_predictions_type& predictions)
      : base_t{std::move(allocGetter)}
      , m_Predictions{prediction_shifter{}(predictions)}
    {}

    [[nodiscard]]
    constexpr const predictions_type& get_predictions() const noexcept
    {
      return m_Predictions;
    }
  private:
    predictions_type m_Predictions;

    struct prediction_shifter
    {
      using allocClass = alloc_equivalence_class_generator_t<T, Getter>;

      constexpr predictions_type operator()(const predictions_type& predictions) const
      {
        return shift<allocClass>(predictions);
      }

      constexpr predictions_type operator()(const inner_predictions_type& predictions) const
      {
        return static_cast<predictions_type>(shift<allocClass>(predictions));
      }
    };
  };

  template
  <
    class Fn,
    class T = std::remove_cvref_t<typename function_signature<decltype(&std::remove_cvref_t<Fn>::operator())>::arg>
  >
  allocation_info(Fn, const type_to_allocation_predictions_t<T>&)
    -> allocation_info<T, Fn>;

  template<movable_comparable T, alloc_getter<T> Getter>
  struct outer_alloc_getter
  {
    using alloc_equivalence_class = alloc_equivalence_class_generator_t<T, Getter>;

    outer_alloc_getter(Getter g) : getter{std::move(g)} {}

    auto operator()(const T& c) const
    {
      return getter(c).outer_allocator();
    }

    Getter getter;
  };

  /*! \brief A specialization of allocation_info appropriate for std::scoped_allocator_adaptor

      The essential difference to the primary template is that multiple sets of predictions must
      be supplied, one for each level within the scoped_allocator_adaptor.
   */
  template<movable_comparable T, alloc_getter<T> Getter>
    requires scoped_alloc<std::invoke_result_t<Getter, T>>
  class allocation_info<T, Getter>
    : public allocation_info_base<T, Getter>
  {
  private:
    using base_t = allocation_info_base<T, Getter>;
  public:
    using value_type             = T;
    using allocator_type         = typename base_t::allocator_type;
    using predictions_type       = type_to_allocation_predictions_t<T>;
    using inner_predictions_type = type_to_inner_allocation_predictions_t<T>;

    constexpr static std::size_t size{alloc_count<allocator_type>::size};
    static_assert(size > 0);

    constexpr allocation_info(Getter allocGetter,
                              predictions_type predictions,
                              std::initializer_list<inner_predictions_type> innerPredictions)
      : base_t{std::move(allocGetter)}
      , m_Predictions{predictions}
      , m_InnerPredictions{utilities::to_array<inner_predictions_type, size-1>(innerPredictions)}
    {}

    /// unpacks the scoped_allocator_adaptor, returning allocation_info for the
    /// allocator at the ith level.
    template<std::size_t I>
    [[nodiscard]]
    auto unpack() const
    {
      if constexpr(I == 0)
      {
        using getter = outer_alloc_getter<T, Getter>;
        return allocation_info<T, getter>{getter{this->make_getter()}, m_Predictions};
      }
      else
      {
        auto scopedGetter{[getter{this->make_getter()}] (const T& c){
            return get<I>(getter(c));
          }
        };
        return allocation_info<T, decltype(scopedGetter)>{scopedGetter, m_InnerPredictions[I-1]};
      }
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

    predictions_type m_Predictions;
    std::array<inner_predictions_type, size-1> m_InnerPredictions;
  };

  template
  <
    class Fn,
    class T = std::remove_cvref_t<typename function_signature<decltype(&std::remove_cvref_t<Fn>::operator())>::arg>,
    class Predictions = type_to_allocation_predictions_t<T>,
    class InnerPredictions = type_to_inner_allocation_predictions_t<T>
  >
  allocation_info(Fn, Predictions, std::initializer_list<InnerPredictions>)
    -> allocation_info<T, Fn>;

  template<top_level TopLevel>
  class container_predictions_policy
  {
  public:
    [[nodiscard]]
    constexpr container_counts containers() const noexcept { return m_Containers; }
  protected:
    constexpr container_predictions_policy(container_counts counts)
      : m_Containers{std::move(counts)}
    {}

    constexpr container_predictions_policy(const container_predictions_policy&) = default;
    constexpr container_predictions_policy& operator=(const container_predictions_policy&) = default;

    ~container_predictions_policy() = default;
  private:
    container_counts m_Containers;
  };

  template<>
  class container_predictions_policy<top_level::yes>
  {
  protected:
    constexpr container_predictions_policy() = default;
    constexpr container_predictions_policy(const container_predictions_policy&) = default;
    constexpr container_predictions_policy& operator=(const container_predictions_policy&) = default;

    ~container_predictions_policy() = default;
  };

  template<has_alloc_init_predictions>
  class initialization_predictions_policy
  {
  public:
    [[nodiscard]]
    constexpr initialization_prediction x_init_alloc() const noexcept { return m_xInit; }

    [[nodiscard]]
    constexpr initialization_prediction y_init_alloc() const noexcept { return m_yInit; }
  protected:
    constexpr initialization_predictions_policy(initialization_prediction x, initialization_prediction y)
      : m_xInit{x},
        m_yInit{y}
    {}

    constexpr initialization_predictions_policy(const initialization_predictions_policy&) = default;
    constexpr initialization_predictions_policy& operator=(const initialization_predictions_policy& other) = default;

    ~initialization_predictions_policy() = default;
  private:
    initialization_prediction m_xInit, m_yInit;
  };

  template<>
  class initialization_predictions_policy<has_alloc_init_predictions::no>
  {
  protected:
    constexpr initialization_predictions_policy() = default;
    constexpr initialization_predictions_policy(const initialization_predictions_policy&) = default;
    constexpr initialization_predictions_policy& operator=(const initialization_predictions_policy&) = default;

    ~initialization_predictions_policy() = default;
  };
}
