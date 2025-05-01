////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2024.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file 
    \brief type_list and various associated operations 
 */

#include <tuple>
#include <variant>

namespace sequoia
{
    //================================================ type_list ================================================//

    /*! \class */

    template<class... Ts>
    struct type_list {};

    /*! @defgroup is_type_list The is_type_list Group
        This group provides a mechanism for determining if a type is a type_list
        @{
     */

    template<class T>
    struct is_type_list : std::false_type {};

    template<class... Ts>
    struct is_type_list<type_list<Ts...>> : std::true_type {};

    template<class T>
    using is_type_list_t = typename is_type_list<T>::type;

    template<class T>
    inline constexpr bool is_type_list_v = is_type_list<T>::value;

    /*! @} */ // end of is_type_list group

     /*! @defgroup faithful_type_list The faithful_type_list Group
        This group provides a mechanism for constructing faithful type lists: each type occurs exactly once
        @{
     */

    template<class...>
    struct build_faithful_type_list;

    template<class... Ts>
    using faithful_type_list = typename build_faithful_type_list<Ts...>::type;

    template<>
    struct build_faithful_type_list<>
    {
        using type = type_list<>;
    };

    template<class T>
    struct build_faithful_type_list<T>
    {
        using type = type_list<T>;
    };

    template<class... Ts, class T>
        requires (std::is_same_v<Ts, T> || ...)
    struct build_faithful_type_list<type_list<Ts...>, T>
    {
        using type = type_list<Ts...>;
    };

    template<class... Ts, class T>
    struct build_faithful_type_list<type_list<Ts...>, T>
    {
        using type = type_list<Ts..., T>;
    };

    template<class... Ts, class T, class... Us>
    struct build_faithful_type_list<type_list<Ts...>, T, Us...>
    {
        using type = faithful_type_list<faithful_type_list<type_list<Ts...>, T>, Us...>;
    };

    template<class T, class... Ts>
    struct build_faithful_type_list<T, Ts...>
    {
        using type = faithful_type_list<type_list<T>, Ts...>;
    };

    /*! @} */ // end of faithful_type_list group

    /*! @defgroup is_type_list The type_list_union Group
        This group provides a mechanism for constructing unions of `type_list`s
        @{
     */

    template<class...>
    struct type_list_union;

    template<class... Ts>
    using type_list_union_t = typename type_list_union<Ts...>::type;

    template<class... Ts>
    struct type_list_union<type_list<Ts...>>
    {
        using type = type_list<Ts...>;
    };

    template<class... Ts, class... Us>
    struct type_list_union<type_list<Ts...>, type_list<Us...>>
    {
        using type = faithful_type_list<Ts..., Us...>;
    };

    template<class... Ts, class... Us, class... Vs>
        requires (is_type_list_v<Vs> && ...)
    struct type_list_union<type_list<Ts...>, type_list<Us...>, Vs...>
    {
        using type = type_list_union_t<type_list_union_t<type_list<Ts...>, type_list<Us...>>, Vs...>;
    };

    /*! @} */ // end of type_list_union group

    /*! @defgroup type_list_conversions The type_list_conversions Group
       This group provides mechanisms for converting type_lists to other types
       @{
    */

    template<class...>
    struct to_variant;

    template<class... Ts>
    using to_variant_t = typename to_variant<Ts...>::type;

    template<class... Ts>
    struct to_variant<type_list<Ts...>>
    {
        using type = std::variant<Ts...>;
    };

    template<class...>
    struct to_tuple;

    template<class... Ts>
    using to_tuple_t = typename to_tuple<Ts...>::type;

    template<class... Ts>
    struct to_tuple<type_list<Ts...>>
    {
        using type = std::tuple<Ts...>;
    };

    /*! @} */ // end of type_list_conversions group
}