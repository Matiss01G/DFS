#line 1 "include/boost/atomic/detail/core_operations_emulated_fwd.hpp"
/*
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * Copyright (c) 2020 Andrey Semashev
 */
/*!
 * \file   atomic/detail/core_operations_emulated_fwd.hpp
 *
 * This header forward-declares lock pool-based implementation of the core atomic operations.
 */

#ifndef BOOST_ATOMIC_DETAIL_CORE_OPERATIONS_EMULATED_FWD_HPP_INCLUDED_
#define BOOST_ATOMIC_DETAIL_CORE_OPERATIONS_EMULATED_FWD_HPP_INCLUDED_

#include <cstddef>
#include <boost/atomic/detail/config.hpp>
#include <boost/atomic/detail/header.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {
namespace atomics {
namespace detail {

template< std::size_t Size, std::size_t Alignment, bool Signed, bool Interprocess >
struct core_operations_emulated;

} // namespace detail
} // namespace atomics
} // namespace boost

#include <boost/atomic/detail/footer.hpp>

#endif // BOOST_ATOMIC_DETAIL_CORE_OPERATIONS_EMULATED_FWD_HPP_INCLUDED_
