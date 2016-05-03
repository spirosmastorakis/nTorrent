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
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with nTorrent, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of nTorrent authors and contributors.
 */
#include "sequential-data-fetcher.hpp"
#include "torrent-file.hpp"
#include "util/io-util.hpp"
#include "util/logging.hpp"

#include <iostream>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>

#include <boost/program_options.hpp>
#include <boost/program_options/errors.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

namespace logging = boost::log;
namespace po = boost::program_options;

using namespace ndn;
using namespace ndn::ntorrent;

namespace ndn {

class Error : public std::runtime_error
{
public:
  explicit
  Error(const std::string& what)
    : runtime_error(what)
  {
  }
};

namespace ntorrent {

const char * SharedConstants::commonPrefix = "/ndn";

} // end ntorrent
} // end ndn

// TODO(msweatt) Add options verification
int main(int argc, char *argv[])
{
  try {
    po::options_description desc("Allowed options");
    desc.add_options()
    // TODO(msweatt) Consider  adding  flagged args for other parameters
      ("help,h", "produce help message")
      ("generate,g" , "-g <data directory> <output-path>? <names-per-segment>? <names-per-manifest-segment>? <data-packet-size>?")
      ("seed,s", "After download completes, continue to seed")
      ("dump,d", "-d <file> Dump the contents of the Data stored at the <file>.")
      ("log-level", po::value<std::string>(), "trace | debug | info | warming | error | fatal")
      ("args", po::value<std::vector<std::string> >(), "For arguments you want to specify without flags")
    ;
    po::positional_options_description p;
    p.add("args", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).allow_unregistered().positional(p).run(), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
    if (vm.count("log-level")) {
    std::unordered_map<std::string, log::severity_level> supported_levels = {
        {"trace"   , log::severity_level::trace},
        { "debug"  , log::severity_level::debug},
        { "info"   , log::severity_level::info},
        { "warning", log::severity_level::warning},
        { "error"  , log::severity_level::error},
        { "fatal"  , log::severity_level::fatal}
      };
      auto log_str = vm["log-level"].as<std::string>();
      if (supported_levels.count(log_str)) {
        LoggingUtil::severity_threshold  = supported_levels[log_str];
      }
      else {
        throw ndn::Error("Unsupported log level: " + log_str);
      }
    }
    // setup log
    LoggingUtil::init();
    logging::add_common_attributes();

    if (vm.count("args")) {
      auto args = vm["args"].as<std::vector<std::string>>();
      // if generate mode
      if (vm.count("generate")) {
        if (args.size() < 1 || args.size() > 5) {
          throw ndn::Error("wrong number of arguments for generate");
        }
        auto dataPath         = args[0];
        auto outputPath       = args.size() >= 2 ? args[1] : ".appdata/";
        auto namesPerSegment  = args.size() >= 3 ? boost::lexical_cast<size_t>(args[2]) : 1024;
        auto namesPerManifest = args.size() >= 4 ? boost::lexical_cast<size_t>(args[3]) : 1024;
        auto dataPacketSize   = args.size() == 5 ? boost::lexical_cast<size_t>(args[4]) : 1024;

        const auto& content = TorrentFile::generate(dataPath,
                                                    namesPerSegment,
                                                    namesPerManifest,
                                                    dataPacketSize);
        const auto& torrentSegments = content.first;
        std::vector<FileManifest> manifests;
        for (const auto& ms : content.second) {
          manifests.insert(manifests.end(), ms.first.begin(), ms.first.end());
        }
        auto torrentPrefix = fs::canonical(dataPath).filename().string();
        outputPath += ("/" + torrentPrefix);
        auto torrentPath =  outputPath + "/torrent_files/";
        // write all the torrent segments
        for (const TorrentFile& t : torrentSegments) {
          if (!IoUtil::writeTorrentSegment(t, torrentPath)) {
            LOG_ERROR << "Write failed: " << t.getName() << std::endl;
            return -1;
          }
        }
        auto manifestPath = outputPath + "/manifests/";
        for (const FileManifest& m : manifests) {
          if (!IoUtil::writeFileManifest(m, manifestPath)) {
            LOG_ERROR << "Write failed: " << m.getName() << std::endl;
            return -1;
          }
        }
      }
      // if dump mode
      else if(vm.count("dump")) {
        if (args.size() != 1) {
          throw ndn::Error("wrong number of arguments for dump");
        }
        auto filePath = args[0];
        auto data = io::load<Data>(filePath);
        if (nullptr != data) {
          std::cout << data->getFullName() << std::endl;
        }
        else {
          throw ndn::Error("Invalid data.");
        }
      }
      // standard torrent mode
      else {
        // <torrent-file-name> <data-path>
        if (args.size() != 2) {
          throw ndn::Error("wrong number of arguments for torrent");
        }
        auto torrentName = args[0];
        auto dataPath    = args[1];
        auto seedFlag    = (vm.count("seed") != 0);
        SequentialDataFetcher fetcher(torrentName, dataPath, seedFlag);
        fetcher.start();
      }
    }
    else {
      std::cout << desc << std::endl;
      return 1;
    }
  }
  catch(std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
  catch(...) {
    std::cerr << "Exception of unknown type!\n";
  }
  return 0;
}
