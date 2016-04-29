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

#ifndef SEQUENTIAL_DATA_FETCHER_HPP
#define SEQUENTIAL_DATA_FETCHER_HPP

#include "fetching-strategy-manager.hpp"
#include "torrent-manager.hpp"

#include <ndn-cxx/name.hpp>

namespace ndn {
namespace ntorrent {

class SequentialDataFetcher : FetchingStrategyManager {
  public:
    class Error : public std::runtime_error
	  {
  	public:
  	  explicit
  	  Error(const std::string& what)
  	    : std::runtime_error(what)
  	  {
  	  }
  	};
    /**
     * @brief Create a new SequentialDataFetcher
     * @param torrentFileName The name of the torrent file
     * @param dataPath The path that the manager would look for already stored data packets and
     *                 will write new data packets
     */
    SequentialDataFetcher(const ndn::Name& torrentFileName,
                          const std::string& dataPath);

    ~SequentialDataFetcher();

    /**
     * @brief Start the sequential data fetcher
     */
    void
    start(const time::milliseconds& timeout = time::milliseconds::zero());

    /**
     * @brief Pause the sequential data fetcher
     */
    void
    pause();

    /**
     * @brief Resume the sequential data fetcher
     */
    void
    resume();

  protected:
    void
    downloadTorrentFile();

    void
    downloadManifestFiles(const std::vector<ndn::Name>& manifestsName);

    void
    downloadPackets(const std::vector<ndn::Name>& packetsName);

    void
    implementSequentialLogic();

    virtual void
    onDataPacketReceived(const ndn::Data& data);

    virtual void
    onDataRetrievalFailure(const ndn::Interest& interest, const std::string& errorCode);

    virtual void
    onManifestReceived(const std::vector<Name>& packetNames);

    virtual void
    onTorrentFileSegmentReceived(const std::vector<Name>& manifestNames);

  private:
    std::string m_dataPath;
    ndn::Name m_torrentFileName;
    shared_ptr<TorrentManager> m_manager;
};

} // namespace ntorrent
} // namespace ndn

#endif // SEQUENTIAL_DATA_FETCHER_HPP
