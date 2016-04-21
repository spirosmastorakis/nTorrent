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

#ifndef FETCHING_STRATEGY_MANAGER_HPP
#define FETCHING_STRATEGY_MANAGER_HPP

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/util/time.hpp>

namespace ndn {
namespace ntorrent {

class FetchingStrategyManager {
  public:
    /**
     * @brief Default Constructor
     */
    FetchingStrategyManager() = default;

    /**
     * @brief Class Destructor
     */
    virtual
    ~FetchingStrategyManager();

    /**
     * @brief Method called to start the torrent downloading
     */
    virtual void
    start() = 0;

    /**
     * @brief Method called to pause the torrent downloading
     */
    virtual void
    pause() = 0;

    /**
     * @brief Method called to continue the torrent downloading
     */
    virtual void
    resume() = 0;

    /**
     * @brief Struct representing the status of the downloading
     *  process that will be passed to the application layer
     */
    struct status {
      double downloadedPercent;
    };
    /**
     * @brief Seed downloaded data for the specified timeout.
     * By default this will go into an infinite loop.
     */
    virtual void
    seed(const time::milliseconds& timeout = time::milliseconds::zero()) const = 0;

  private:
    /**
     * @brief Callback to be called when data is received
     */
    virtual void
    onDataPacketReceived(const ndn::Data& data) = 0;

    /**
     * @brief Callback to be called when data retrieval failed
     */
    virtual void
    onDataRetrievalFailure(const ndn::Interest& interest, const std::string& errorCode) = 0;

    /**
     * @brief Callback to be called when a file manifest is received
     */
    virtual void
    onManifestReceived(const std::vector<Name>& packetNames) = 0;
};

inline
FetchingStrategyManager::~FetchingStrategyManager()
{
}

} // namespace ntorrent
} // namespace ndn

#endif // FETCHING_STRATEGY_MANAGER_HPP
