#line 1 "include/boost/log/keywords/file_name.hpp"
/*
 *          Copyright Andrey Semashev 2007 - 2015.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   keywords/file_name.hpp
 * \author Andrey Semashev
 * \date   14.03.2009
 *
 * The header contains the \c file_name keyword declaration.
 */

#ifndef BOOST_LOG_KEYWORDS_FILE_NAME_HPP_INCLUDED_
#define BOOST_LOG_KEYWORDS_FILE_NAME_HPP_INCLUDED_

#include <boost/parameter/keyword.hpp>
#include <boost/log/detail/config.hpp>

#ifdef BOOST_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace keywords {

//! The keyword allows to pass log file name the rotating file stream methods
BOOST_PARAMETER_KEYWORD(tag, file_name)

} // namespace keywords

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#endif // BOOST_LOG_KEYWORDS_FILE_NAME_HPP_INCLUDED_
