////////////////////////////////////////////////////////////////////
//                Copyright Oliver J. Rosten 2018.                //
// Distributed under the GNU GENERAL PUBLIC LICENSE, Version 3.0. //
//    (See accompanying file LICENSE.md or copy at                //
//          https://www.gnu.org/licenses/gpl-3.0.en.html)         //
////////////////////////////////////////////////////////////////////

#pragma once

/*! \file
    \brief Structs to enable homogenous treatment of data which is/is not
    handled via shared pointers.

 */

#include "sequoia/Core/Object/HandlerTraits.hpp"
#include "sequoia/Core/Meta/Concepts.hpp"

#include <memory>

namespace sequoia::object
{
  template<class T, class... Args>
  [[nodiscard]]
  std::shared_ptr<T> make_shared_braced(Args&&... args)
  {
    return std::shared_ptr<T>(new T{std::forward<Args>(args)...});
  }

  template<class T>
  struct shared
  {
  public:
    using product_type = std::shared_ptr<T>;
    using value_type   = T;

    template<class... Args>
    [[nodiscard]]
    static product_type make(Args&&... args)
    {
      return make_shared_braced<T>(std::forward<Args>(args)...);
    }

    [[nodiscard]]
    static T& get(product_type& ptr)
    {
      return *ptr;
    }

    [[nodiscard]]
    static const T& get(const product_type& ptr)
    {
      return *ptr;
    }

    [[nodiscard]]
    static T* get_ptr(product_type& ptr)
    {
      return &*ptr;
    }

    [[nodiscard]]
    static const T* get_ptr(const product_type& ptr)
    {
      return &*ptr;
    }

  protected:
    shared() noexcept = default;
    shared(const shared&) noexcept = default;
    shared(shared&&)      noexcept = default;

    ~shared() noexcept = default;

    shared& operator=(const shared&) noexcept = default;
    shared& operator=(shared&&)      noexcept = default;
  };

  template<class T>
  struct independent
  {
  public:
    using product_type = T;
    using value_type   = T;

    template<class... Args>
    [[nodiscard]]
    constexpr static T make(Args&&... args)
    {
      return T{std::forward<Args>(args)...};
    }

    [[nodiscard]]
    constexpr static T& get(T& in) noexcept { return in; }

    [[nodiscard]]
    constexpr static const T& get(const T& in) noexcept { return in; }

    [[nodiscard]]
    constexpr static T* get_ptr(T& in) noexcept { return &in; }

    [[nodiscard]]
    constexpr static const T* get_ptr(const T& in) noexcept { return &in; }

  protected:
    independent() noexcept = default;
    independent(const independent&) noexcept = default;
    independent(independent&&)      noexcept = default;

    ~independent() noexcept = default;

    independent& operator=(const independent&) noexcept = default;
    independent& operator=(independent&&)      noexcept = default;
  };
}
