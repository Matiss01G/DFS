#line 1 "include/boost/log/keywords/format.hpp"
/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   keywords/format.hpp
 * \author Andrey Semashev
 * \date   14.03.2009
 *
 * The header contains the \c format keyword declaration.
 */

#ifndef BOOST_LOG_KEYWORDS_FORMAT_HPP_INCLUDED_
#define BOOST_LOG_KEYWORDS_FORMAT_HPP_INCLUDED_

#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace keywords {

//! The keyword for passing format specifiers to functions
BOOST_PARAMETER_KEYWORD(tag, format)

} // namespace keywords

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_KEYWORDS_FORMAT_HPP_INCLUDED_
