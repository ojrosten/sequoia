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

    All concrete edges
    inherit from this class but may have additional adornments. Depending on
    the context, edges may have one or more of the following:

    1. A non-trivial weight;

    2. A record of the 'source' node, from which the edge points to its target;

    3. An index recording the location on the target node into which the edge is
    embedded.
*/

#include "sequoia/Core/Meta/Concepts.hpp"
#include "sequoia/Core/Meta/TypeTraits.hpp"
#include "sequoia/Core/ObjectHandling/HandlerTraits.hpp"
#include "sequoia/Maths/Graph/EdgesAndNodesUtilities.hpp"

#include <stdexcept>

namespace sequoia
{
  namespace object
  {
    template<class> struct independent;
  }

  namespace maths
  {
    /*! \class edge_base
        \brief The base class of all edges.

        This class is designed for inheritance and has protected destructor
        and assignment operators.

        The private data comprises a single member of IndexType which is the
        'target node' to which the edge points.
    */

    template<std::integral IndexType>
    class edge_base
    {
    public:
      using index_type = IndexType;

      explicit constexpr edge_base(const index_type target) noexcept : m_Target{target} {}
      constexpr edge_base(const edge_base&) noexcept = default;
      constexpr edge_base(edge_base&&)      noexcept = default;

      [[nodiscard]]
      constexpr IndexType target_node() const noexcept { return m_Target; }

      constexpr void target_node(const index_type target) noexcept { m_Target = target; }

      [[nodiscard]]
      friend constexpr bool operator==(const edge_base& lhs, const edge_base& rhs) noexcept
      {
        return (lhs.target_node() == rhs.target_node());
      }

      [[nodiscard]]
      friend constexpr bool operator!=(const edge_base& lhs, const edge_base& rhs) noexcept
      {
        return !(lhs == rhs);
      }
    protected:
      ~edge_base() = default;

      constexpr edge_base& operator=(const edge_base&) noexcept = default;
      constexpr edge_base& operator=(edge_base&&)      noexcept = default;
    private:
      IndexType m_Target;
    };

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
      using weight_proxy_type = typename WeightHandler::elementary_type;
      using weight_type       = typename weight_proxy_type::value_type;

      template<class Arg, class... Args>
      constexpr void weight(Arg&& arg, Args&&... args)
      {
        WeightHandler::get(m_Weight).set(std::forward<Arg>(arg), std::forward<Args>(args)...);
      }

      [[nodiscard]]
      constexpr const weight_type& weight() const noexcept { return WeightHandler::get(m_Weight).get(); }

      [[nodiscard]]
      constexpr const weight_proxy_type& weight_proxy() const noexcept { return WeightHandler::get(m_Weight); }

      template<class Fn>
      constexpr void mutate_weight(Fn fn)
      {
        WeightHandler::get(m_Weight).mutate(fn);
      }

      [[nodiscard]]
      friend constexpr bool operator==(const weighting& lhs, const weighting& rhs) noexcept
      {
        return lhs.weight() == rhs.weight();
      }

      [[nodiscard]]
      friend constexpr bool operator!=(const weighting& lhs, const weighting& rhs) noexcept
      {
        return !(lhs == rhs);
      }
    protected:
      ~weighting() = default;

      template<class... Args>
        requires (!resolve_to_copy_v<weighting, Args...> && !is_base_of_head_v<weighting, Args...>)
      constexpr explicit weighting(Args&&... args) : m_Weight{WeightHandler::make(std::forward<Args>(args)...)}
      {}

      constexpr weighting(const weighting& in) : m_Weight{WeightHandler::make(WeightHandler::get(in.m_Weight))}
      {
      }

      constexpr weighting(const weighting& in, const IndexType) : m_Weight{in.m_Weight}
      {
      }

      constexpr weighting(weighting&&) noexcept = default;

      constexpr weighting& operator=(const weighting& in)
      {
        if(&in != this) m_Weight = WeightHandler::make(WeightHandler::get(in.m_Weight));
        return *this;
      }

      constexpr weighting& operator=(weighting&&) noexcept = default;
    private:
      typename WeightHandler::handle_type m_Weight;
    };

    /*! \class weighting<WeightHandler, IndexType, true>
        \brief An empty (base) class for edges without a weight.

     */

    template<class WeightHandler, std::integral IndexType>
      requires (object::handler<WeightHandler> && std::is_empty_v<typename WeightHandler::elementary_type::value_type>)
    class weighting<WeightHandler, IndexType>
    {
    public:
      using weight_proxy_type = typename WeightHandler::elementary_type;
      using weight_type       = typename weight_proxy_type::value_type;

      [[nodiscard]]
      friend constexpr bool operator==(const weighting&, const weighting&) noexcept = default;

      [[nodiscard]]
      friend constexpr bool operator!=(const weighting&, const weighting&) noexcept = default;
    protected:
      constexpr weighting() = default;
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
    class partial_edge_base : public edge_base<IndexType>, public weighting<WeightHandler, IndexType>
    {
    public:
      using index_type = IndexType;

      template<class... Args>
        requires (!resolve_to_copy_v<partial_edge_base, Args...>)
      constexpr explicit partial_edge_base(const index_type target, Args&&... args)
        : edge_base<IndexType>{target}
        , weighting<WeightHandler, IndexType>{std::forward<Args>(args)...}
      {}

      constexpr partial_edge_base(const index_type target, const partial_edge_base& in)
        : edge_base<IndexType>{target}
        , weighting<WeightHandler, IndexType>{in, target}
      {
      }

      constexpr partial_edge_base(const partial_edge_base& in)     = default;
      constexpr partial_edge_base(partial_edge_base&& in) noexcept = default;

      [[nodiscard]]
      friend constexpr bool operator==(const partial_edge_base& lhs, const partial_edge_base& rhs) noexcept = default;

      friend constexpr bool operator!=(const partial_edge_base& lhs, const partial_edge_base& rhs) noexcept = default;
    protected:
      ~partial_edge_base() = default;

      constexpr partial_edge_base& operator=(const partial_edge_base& in)  = default;
      constexpr partial_edge_base& operator=(partial_edge_base&&) noexcept = default;
    };

    //===================================Helper Enum===================================//

    enum class edge_flavour{ partial, partial_embedded, full, full_embedded};

    //===================================Partial Edge===================================//

    /*! \class partial_edge
        \brief A concrete edge containing a target index and, optionally, a weight

     */

    template<class WeightHandler, std::integral IndexType=std::size_t>
      requires object::handler<WeightHandler>
    class partial_edge : public partial_edge_base<WeightHandler, IndexType>
    {
    public:
      constexpr static edge_flavour flavour{edge_flavour::partial};
      using partial_edge_base<WeightHandler, IndexType>::partial_edge_base;
    };

    //===================================Decorated Partial Edge Base===================================//

    /*! \class decorated_edge_base
        \brief A new base class which aggregates a partial_edge_base and an auxiliary index.

     */

    template<class WeightHandler, std::integral IndexType=std::size_t>
      requires object::handler<WeightHandler>
    class decorated_edge_base : public partial_edge_base<WeightHandler, IndexType>
    {
    public:
      using weight_type = typename partial_edge_base<WeightHandler, IndexType>::weight_type;
      using index_type  = typename partial_edge_base<WeightHandler, IndexType>::index_type;

      template<class... Args>
        requires (!resolve_to_copy_v<decorated_edge_base, Args...>)
      constexpr decorated_edge_base(const index_type target, const index_type auxIndex, Args&&... args)
        : partial_edge_base<WeightHandler, IndexType>{target, std::forward<Args>(args)...}
        , m_AuxiliaryIndex{auxIndex}
      {}

      constexpr decorated_edge_base(const index_type target, const index_type auxIndex, const decorated_edge_base& in)
        : partial_edge_base<WeightHandler, IndexType>{target, in}
        , m_AuxiliaryIndex{auxIndex}
      {}

      constexpr decorated_edge_base(const decorated_edge_base& in)             = default;
      constexpr decorated_edge_base(decorated_edge_base&& in)                  = default;
      constexpr decorated_edge_base& operator=(const decorated_edge_base& in)  = default;
      constexpr decorated_edge_base& operator=(decorated_edge_base&&)          = default;

      [[nodiscard]]
      friend constexpr bool operator==(const decorated_edge_base& lhs, const decorated_edge_base& rhs) noexcept = default;

      [[nodiscard]]
      friend constexpr bool operator!=(const decorated_edge_base& lhs, const decorated_edge_base& rhs) noexcept = default;
    protected:
      ~decorated_edge_base() = default;

      [[nodiscard]]
      constexpr index_type auxiliary_index() const noexcept { return m_AuxiliaryIndex; }

      constexpr void auxiliary_index(const index_type auxIndex) noexcept { m_AuxiliaryIndex = auxIndex; }
    private:
      IndexType m_AuxiliaryIndex;
    };

    //===================================Embedded Partial Edge===================================//

    /*! \class embedded_partial_edge
        \brief Decoration of a partial_edge to record the location on the target node into
               which the edge is embedded.

     */

    template<class WeightHandler, std::integral IndexType=std::size_t>
      requires object::handler<WeightHandler>
    class embedded_partial_edge : public decorated_edge_base<WeightHandler, IndexType>
    {
    public:
      constexpr static edge_flavour flavour{edge_flavour::partial_embedded};
      using weight_type = typename decorated_edge_base<WeightHandler, IndexType>::weight_type;
      using index_type  = typename decorated_edge_base<WeightHandler, IndexType>::index_type;

      using decorated_edge_base<WeightHandler, IndexType>::decorated_edge_base;

      [[nodiscard]]
      constexpr index_type complementary_index() const noexcept { return this->auxiliary_index(); }
      constexpr void complementary_index(const index_type index) noexcept { return this->auxiliary_index(index); }
    };

    //===================================Full Edge===================================//

    template<bool Inverted> struct inversion_constant : public std::bool_constant<Inverted> {};

    template<bool Inverted> constexpr bool inversion_v{inversion_constant<Inverted>::value};

    /*! \class edge
        \brief Stores the source node as well as the target node and an optional weight.

     */

    template<class WeightHandler, std::integral IndexType=std::size_t>
      requires object::handler<WeightHandler>
    class edge : public decorated_edge_base<WeightHandler, IndexType>
    {
    public:
      constexpr static edge_flavour flavour{edge_flavour::full};
      using weight_type = typename decorated_edge_base<WeightHandler, IndexType>::weight_type;
      using index_type  = typename decorated_edge_base<WeightHandler, IndexType>::index_type;

      template<class... Args>
        requires (!resolve_to_copy_v<edge, Args...>)
      constexpr edge(const index_type source, const index_type target, Args&&... args)
        : decorated_edge_base<WeightHandler, IndexType>{target, source, std::forward<Args>(args)...}
      {
        if(source == npos) throw std::runtime_error("Cannot initialize full edge using max value of index");
      }

      template
      <
        bool Inverted,
        class... Args
      >
      constexpr edge(const index_type node, const inversion_constant<Inverted> inverted, Args&&... args)
        : decorated_edge_base<WeightHandler, IndexType>{
            node, inverted.value ? npos : node, std::forward<Args>(args)...
          }
      {
        if(node == npos) throw std::runtime_error("Cannot initialize full edge using max value of index");
      }

      constexpr edge(const edge& in)             = default;
      constexpr edge(edge&& in)                  = default;
      constexpr edge& operator=(const edge& in)  = default;
      constexpr edge& operator=(edge&&)          = default;

      [[nodiscard]]
      constexpr index_type source_node() const noexcept
      {
        return this->auxiliary_index() < npos ? this->auxiliary_index() : this->target_node();
      }

      constexpr void source_node(const index_type source) noexcept
      {
        if(!inverted()) this->auxiliary_index(source);
        else            this->target_node(source);
      }

      [[nodiscard]]
      constexpr bool inverted() const noexcept { return this->auxiliary_index() == npos; }
    private:
      constexpr static auto npos = std::numeric_limits<IndexType>::max();
    };

    //===================================Full Embedded Edge===================================//

    /*! \class embedded_edge
        \brief Decoration of edge to record the location on the target node into
               which the edge is embedded.

     */

    template<class WeightHandler, std::integral IndexType=std::size_t>
      requires object::handler<WeightHandler>
    class embedded_edge : public decorated_edge_base<WeightHandler, IndexType>
    {
    public:
      constexpr static edge_flavour flavour{edge_flavour::full_embedded};
      using weight_type = typename decorated_edge_base<WeightHandler, IndexType>::weight_type;
      using index_type  = typename decorated_edge_base<WeightHandler, IndexType>::index_type;

      template<class... Args>
        requires (!resolve_to_copy_v<embedded_edge, Args...>)
      constexpr embedded_edge(const index_type source, const index_type target, const index_type auxIndex, Args&&... args)
        : decorated_edge_base<WeightHandler, IndexType>{target, auxIndex, std::forward<Args>(args)...}
        , m_HostIndex{source}
      {
        if(source == npos) throw std::runtime_error("Cannot initialize full edge using max value of index");
      }

      template<bool Inverted, class... Args>
      constexpr embedded_edge(const index_type node, const inversion_constant<Inverted> inverted, const index_type auxIndex, Args&&... args)
        : decorated_edge_base<WeightHandler, IndexType>{node, auxIndex, std::forward<Args>(args)...}
        , m_HostIndex{inverted.value ? npos : node}
      {
        if(node == npos) throw std::runtime_error("Cannot initialize full edge using max value of index");
      }

      constexpr embedded_edge(const index_type auxIndex, const embedded_edge& in)
        : decorated_edge_base<WeightHandler, IndexType>{in.target_node(), auxIndex, in}
        , m_HostIndex{in.m_HostIndex}
      {
      }

      constexpr embedded_edge(const embedded_edge& in)             = default;
      constexpr embedded_edge(embedded_edge&& in)                  = default;
      constexpr embedded_edge& operator=(const embedded_edge& in)  = default;
      constexpr embedded_edge& operator=(embedded_edge&&)          = default;

      [[nodiscard]]
      constexpr index_type complementary_index() const noexcept { return this->auxiliary_index(); }
      constexpr void complementary_index(const index_type auxIndex) noexcept { return this->auxiliary_index(auxIndex); }

      [[nodiscard]]
      constexpr index_type source_node() const noexcept { return !inverted() ? m_HostIndex : this->target_node(); }

      constexpr void source_node(const index_type source) noexcept
      {
        if(!inverted()) m_HostIndex = source;
        else            this->target_node(source);
      }

      [[nodiscard]]
      constexpr bool inverted() const noexcept { return m_HostIndex == npos; }

      [[nodiscard]]
      friend constexpr bool operator==(const embedded_edge& lhs, const embedded_edge& rhs)
      {
        return (lhs.source_node() == rhs.source_node())
          && (lhs.inverted() == rhs.inverted())
          && (static_cast<const decorated_edge_base<WeightHandler, IndexType>&>(lhs) ==
              static_cast<const decorated_edge_base<WeightHandler, IndexType>&>(rhs));
      }

      [[nodiscard]]
      friend constexpr bool operator!=(const embedded_edge& lhs, const embedded_edge& rhs)
      {
        return !(lhs == rhs);
      }
    private:
      constexpr static auto npos = std::numeric_limits<IndexType>::max();

      IndexType m_HostIndex;
    };
  }
}
