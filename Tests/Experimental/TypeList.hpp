////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2023.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file */

namespace sequoia::testing
{
  /*! \brief Primary template for mapping a parameter pack to a type */
  template<class... Ts>
  struct type_list;

  template<class T, class... Ts>
  struct type_list<T, Ts...>
  {
    using head = T;
    using tail = type_list<Ts...>;
  };

  template<>
  struct type_list<>
  {
    using head = void;
    using tail = type_list<>;
  };

  /*! \brief Primary template for finding the head of either a variadic list of a type_list */
  template<class... Ts>
  struct head_of
  {
    using type = typename type_list<Ts...>::head;
  };

  template<class... Ts>
  struct head_of<type_list<Ts...>>
  {
    using type = typename type_list<Ts...>::head;
  };

  template<class... Ts>
  using head_of_t = typename head_of<Ts...>::type;

  /*! \brief Primary template for finding the tail of either a variadic list of a type_list */
  template<class... Ts>
  struct tail_of
  {
    using type = typename type_list<Ts...>::tail;
  };

  template<class... Ts>
  struct tail_of<type_list<Ts...>>
  {
    using type = typename type_list<Ts...>::tail;
  };

  template<class... Ts>
  using tail_of_t = typename tail_of<Ts...>::type;

  /*! @} */ // end of type_list Group

}