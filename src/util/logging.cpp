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


#include "util/logging.hpp"

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

// ===== log macros =====
namespace logging = boost::log;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;

namespace ndn {
namespace ntorrent {

log::severity_level LoggingUtil::severity_threshold = log::info;

void LoggingUtil::init(bool log_to_console)
{
  // set logging level
  logging::core::get()->set_filter
  (
      logging::trivial::severity >= severity_threshold
  );

  boost::shared_ptr< logging::core > core = logging::core::get();

  auto backend =
    boost::make_shared< sinks::text_file_backend >(
     keywords::file_name = "sample_%N.log",                                        // < file name pattern >
     keywords::rotation_size = 10 * 1024 * 1024,                                   // < rotate files every 10 MiB... >
     keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0) // < ...or at midnight >
  );
   // Enable auto-flushing after each log record written
  backend->auto_flush(true);

  // Wrap it into the frontend and register in the core.
  // The backend requires synchronization in the frontend.
  typedef sinks::synchronous_sink< sinks::text_file_backend > sink_t;
  boost::shared_ptr< sink_t > sink(new sink_t(backend));
  sink->set_formatter(
   expr::stream
             << expr::attr< unsigned int >("LineID")
             << ": [" << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S") << "]"
             << ": <" << logging::trivial::severity
             << "> " << expr::smessage
  );
  core->add_sink(sink);

  if (log_to_console) {
    logging::add_console_log(std::cerr,
                             keywords::format =
      (
        expr::stream
             << "[" << expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S") << "]"
             << ": <" << logging::trivial::severity
             << "> " << expr::smessage
      )
    );
  }
}

} // end ntorrent
} // end ndn


