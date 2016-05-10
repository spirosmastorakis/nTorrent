/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
* Copyright (c) 2016 Regents of the University of California.
*
* This file is part of the nTorrent codebase.
*
* nTorrent is free software: you can redistribute it and/or modify it under the
* terms of the GNU Lesser General Public License as published by the Free Software
* Foundation, either version 3 of the License, or (at your option) any later version.
*
* nTorrent is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
* PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.
*
* You should have received copies of the GNU General Public License and GNU Lesser
* General Public License along with nTorrent, e.g., in COPYING.md file. If not, see
* <http://www.gnu.org/licenses/>.
*
* See AUTHORS for complete list of nTorrent authors and contributors.
*/
#ifndef UTIL_LOGGING_HPP
#define UTIL_LOGGING_HPP

#define BOOST_LOG_DYN_LINK 1

#include <iostream>

#include <boost/log/core.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/trivial.hpp>

// register a global logger
BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(logger, boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>)

// just a helper macro used by the macros below - don't use it in your code
#define LOG(severity) BOOST_LOG_SEV(logger::get(), boost::log::trivial::severity)

// ===== log macros =====
#define LOG_TRACE   std::cerr
#define LOG_DEBUG   std::cerr
#define LOG_INFO    std::cerr
#define LOG_WARNING std::cerr
#define LOG_ERROR   std::cerr
#define LOG_FATAL   std::cerr

namespace ndn {
namespace ntorrent {

namespace log = boost::log::trivial;

struct LoggingUtil {
  static log::severity_level severity_threshold;

  static void init(bool log_to_console = false);
  // Initialize the log for the application. THis method must be called in the main function in
  // the application before any logging may be performed.
};

} // end ntorrent
} // end ndn
#endif // UTIL_LOGGING_HPP
