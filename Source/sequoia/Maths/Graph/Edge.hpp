////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Various edge types for use by graphs.

    The fundamental building block of the edge types is the edge_base. This
    can be thought of as a directed edge with no additional information; as
    such is 'points', via an index, to a target node.

    All concrete edges inherit from this class but may have additional adornments.
    Depending on the context, edges may have one or more of the following:

    1. A non-trivial weight;

    2. An index recording the location on the target node into which the edge is
    embedded.
*/

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/Core/Meta/TypeTraits.hpp"
#include "sequoia/Core/Object/HandlerTraits.hpp"
#include "sequoia/Maths/Graph/EdgesAndNodesUtilities.hpp"

#include <stdexcept>

namespace sequoia
{
  namespace maths
  {
    /*! \class weighting
        \brief A class to store non-trivial edge weights.

        This class is designed to wrap an edge but in a manner which is flexible
        enough to support the sharing of weights between edges. The latter is used
        for undirected graphs where two partial edges representing the pair of
        links between two vertices may wish to share the common weight, particularly
        if it is expensive to store twice. The sharing of weights between distinct
        edges is done at the level of the graph.
    */

    template<class WeightHandler, std::integral IndexType>
      requires object::handler<WeightHandler>
    class weighting
    {
    public:
      using weight_type = typename WeightHandler::value_type;

      template<class... Args>
        requires (sizeof...(Args) > 0)
      constexpr void weight(Args&&... args)
      {
        WeightHandler::get(m_Weight) = weight_type{std::forward<Args>(args)...};
      }

      [[nodiscard]]
      constexpr const weight_type& weight() const noexcept { return WeightHandler::get(m_Weight); }

      template<std::invocable<weight_type&> Fn>
      constexpr std::invoke_result_t<Fn, weight_type&> mutate_weight(Fn fn)
      {
        return fn(WeightHandler::get(m_Weight));
      }

      [[nodiscard]]
      friend constexpr bool operator==(const weighting& lhs, const weighting& rhs) noexcept
        requires is_deep_equality_comparable_v<weight_type>
      {
        return lhs.weight() == rhs.weight();
      }
    protected:
      ~weighting() = default;

      template<class... Args>
        requires (!resolve_to_copy_v<weighting, Args...>)
      constexpr explicit(sizeof...(Args) == 1) weighting(Args&&... args)
        : m_Weight{WeightHandler::producer_type::make(std::forward<Args>(args)...)}
      {}

      constexpr weighting(const weighting& other)
        : m_Weight{WeightHandler::producer_type::make(WeightHandler::get(other.m_Weight))}
      {}

      template<class Other>
        requires std::is_base_of_v<weighting, std::remove_cvref_t<Other>>
      constexpr weighting(Other&& other) : m_Weight{other.m_Weight}
      {}

      constexpr weighting(weighting&&) noexcept = default;

      constexpr weighting& operator=(const weighting& other)
      {
        if(&other != this) m_Weight = WeightHandler::producer_type::make(WeightHandler::get(other.m_Weight));
        return *this;
      }

      constexpr weighting& operator=(weighting&&) noexcept = default;
    private:
      typename WeightHandler::product_type m_Weight;
    };

    /*! \class weighting<WeightHandler, IndexType, true>
        \brief An empty (base) class for edges without a weight.

     */

    template<class WeightHandler, std::integral IndexType>
      requires (object::handler<WeightHandler> && std::is_empty_v<typename WeightHandler::value_type>)
    class weighting<WeightHandler, IndexType>
    {
    public:
      using weight_type = typename WeightHandler::value_type;

      [[nodiscard]]
      friend constexpr bool operator==(const weighting&, const weighting&) noexcept = default;
    protected:
      constexpr weighting() noexcept = default;
      ~weighting() = default;

      constexpr weighting(const weighting&)                  noexcept = default;
      constexpr weighting(weighting&&)                       noexcept = default;
      constexpr weighting& operator=(const weighting&)       noexcept = default;
      constexpr weighting& operator=(weighting&&)            noexcept = default;
      constexpr weighting(const weighting&, const IndexType) noexcept {}
    };

    //===================================Partial Edge Base===================================//

    /*! \class partial_edge_base
        \brief Combines the edge_base and weighting class

     */

    template<class WeightHandler, std::integral IndexType>
      requires object::handler<WeightHandler>
    class partial_edge_base : public weighting<WeightHandler, IndexType>
    {
    public:
      using index_type  = IndexType;
      using weight_type = typename weighting<WeightHandler, IndexType>::weight_type;

      template<class... Args>
        requires (!resolve_to_copy_v<partial_edge_base, Args...>)
      constexpr explicit(sizeof...(Args) == 0) partial_edge_base(const index_type target, Args&&... args)
        : weighting<WeightHandler, IndexType>{std::forward<Args>(args)...}
        , m_Target{target}
      {}

      constexpr partial_edge_base(const index_type target, const partial_edge_base& other)
        : weighting<WeightHandler, IndexType>{other}
        , m_Target{target}
      {}

      constexpr partial_edge_base(const partial_edge_base&)     = default;
      constexpr partial_edge_base(partial_edge_base&&) noexcept = default;

      [[nodiscard]]
      constexpr IndexType target_node() const noexcept { return m_Target; }

      constexpr void target_node(const index_type target) noexcept { m_Target = target; }

      [[nodiscard]]
      friend constexpr bool operator==(const partial_edge_base&, const partial_edge_base&) noexcept = default;
    protected:
      ~partial_edge_base() = default;

      constexpr partial_edge_base& operator=(const partial_edge_base&)     = default;
      constexpr partial_edge_base& operator=(partial_edge_base&&) noexcept = default;
    private:
      IndexType m_Target;
    };

    //===================================Decorated Partial Edge Base===================================//

    template<class WeightHandler, class MetaData, std::integral IndexType>
      requires object::handler<WeightHandler>
    class decorated_partial_edge_base : public partial_edge_base<WeightHandler, IndexType>
    {
    public:
      using partial_edge_base<WeightHandler, IndexType>::partial_edge_base;

      constexpr decorated_partial_edge_base(const decorated_partial_edge_base&)     = default;
      constexpr decorated_partial_edge_base(decorated_partial_edge_base&&) noexcept = default;
    protected:
      ~decorated_partial_edge_base() = default;

      constexpr decorated_partial_edge_base& operator=(const decorated_partial_edge_base&)     = default;
      constexpr decorated_partial_edge_base& operator=(decorated_partial_edge_base&&) noexcept = default;
    };

    template<class WeightHandler, class MetaData, std::integral IndexType>
      requires object::handler<WeightHandler> && (!std::is_empty_v<MetaData>)
    class decorated_partial_edge_base<WeightHandler, MetaData, IndexType> : public partial_edge_base<WeightHandler, IndexType>
    {
    public:
      using index_type     = IndexType;
      using meta_data_type = MetaData;
      using weight_type    = typename partial_edge_base<WeightHandler, IndexType>::weight_type;

      constexpr decorated_partial_edge_base(const index_type target, weight_type w, meta_data_type m)
        requires (!std::is_empty_v<weight_type>)
        : partial_edge_base<WeightHandler, IndexType>{target, std::move(w)}
        , m_MetaData{std::move(m)}
      {}

      constexpr decorated_partial_edge_base(const index_type target, meta_data_type m)
        requires std::is_empty_v<weight_type>
        : partial_edge_base<WeightHandler, IndexType>{target}
        , m_MetaData{std::move(m)}
      {}

      constexpr decorated_partial_edge_base(const index_type target, const decorated_partial_edge_base& other)
        : partial_edge_base<WeightHandler, IndexType>{other}
        , m_MetaData{other.m_MetaData}
      {}

      constexpr decorated_partial_edge_base(const decorated_partial_edge_base&)     = default;
      constexpr decorated_partial_edge_base(decorated_partial_edge_base&&) noexcept = default;

      [[nodiscard]]
      constexpr const MetaData& meta_data() const noexcept { return m_MetaData; }

      constexpr void meta_data(MetaData m) { m_MetaData = m; }
    protected:
      ~decorated_partial_edge_base() = default;

      constexpr decorated_partial_edge_base& operator=(const decorated_partial_edge_base&)     = default;
      constexpr decorated_partial_edge_base& operator=(decorated_partial_edge_base&&) noexcept = default;
    private:
      MetaData m_MetaData;
    };

    struct null_meta_data {};

    //===================================Helper Enum===================================//

    enum class edge_flavour : bool { partial, partial_embedded };

    //===================================Partial Edge===================================//

    /*! \class partial_edge
        \brief A concrete edge containing a target index and, optionally, a weight

     */

    template<class WeightHandler, class MetaData, std::integral IndexType=std::size_t>
      requires object::handler<WeightHandler>
    class partial_edge : public decorated_partial_edge_base<WeightHandler, MetaData, IndexType>
    {
    public:
      constexpr static edge_flavour flavour{edge_flavour::partial};

      using weight_type    = typename decorated_partial_edge_base<WeightHandler, MetaData, IndexType>::weight_type;
      using meta_data_type = MetaData;
      using index_type     = IndexType;

      using decorated_partial_edge_base<WeightHandler, MetaData, IndexType>::decorated_partial_edge_base;
    };

    //===================================Embedded Partial Edge===================================//

    /*! \class embedded_partial_edge
        \brief Decoration of a partial_edge to record the location on the target node into
               which the edge is embedded.

     */

    template<class WeightHandler, class MetaData, std::integral IndexType=std::size_t>
      requires object::handler<WeightHandler>
    class embedded_partial_edge : public decorated_partial_edge_base<WeightHandler, MetaData, IndexType>
    {
    public:
      constexpr static edge_flavour flavour{edge_flavour::partial_embedded};

      using weight_type    = typename decorated_partial_edge_base<WeightHandler, MetaData, IndexType>::weight_type;
      using meta_data_type = MetaData;
      using index_type     = IndexType;

      template<class... Args>
        requires (!resolve_to_copy_v<embedded_partial_edge, Args...>)
      constexpr embedded_partial_edge(const index_type target, const index_type complimentaryIndex, Args&&... args)
        : decorated_partial_edge_base<WeightHandler, MetaData, IndexType>{target, std::forward<Args>(args)...}
        , m_ComplementaryIndex{complimentaryIndex}
      {}

      constexpr embedded_partial_edge(const index_type target, const index_type complimentaryIndex, const embedded_partial_edge& other)
        : decorated_partial_edge_base<WeightHandler, MetaData, IndexType>{target, other}
        , m_ComplementaryIndex{complimentaryIndex}
      {}

      [[nodiscard]]
      constexpr index_type complementary_index() const noexcept { return m_ComplementaryIndex; }
      constexpr void complementary_index(const index_type index) noexcept { m_ComplementaryIndex = index; }
    private:
      IndexType m_ComplementaryIndex;
    };
  }
}
