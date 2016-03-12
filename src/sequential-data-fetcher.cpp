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

#include "sequential-data-fetcher.hpp"
#include "util/io-util.hpp"

namespace ndn {
namespace ntorrent {

SequentialDataFetcher::SequentialDataFetcher(const ndn::Name& torrentFileName,
                                             const std::string& dataPath)
  : m_dataPath(dataPath)
  , m_torrentFileName(torrentFileName)
{
  m_manager = make_shared<TorrentManager>(m_torrentFileName, m_dataPath);
}

SequentialDataFetcher::~SequentialDataFetcher()
{
}

void
SequentialDataFetcher::start()
{
  m_manager->Initialize();
  // downloading logic
  this->implementSequentialLogic();
}

void
SequentialDataFetcher::pause()
{
  // TODO(Spyros): Implement asyncrhonous pause of the torrent downloading
  // For now, do nothing...
  throw(Error("Not implemented yet"));
}

void
SequentialDataFetcher::resume()
{
  // TODO(Spyros): Implement asyncrhonous re-establishment of the torrent downloading
  // For now, do nothing...
  throw(Error("Not implemented yet"));
}

std::vector<ndn::Name>
SequentialDataFetcher::downloadTorrentFile()
{
  std::vector<ndn::Name> returnedNames;
  returnedNames = m_manager->downloadTorrentFile(".appdata/torrent_files/");
  std::cout << "Torrent File Received: "
            << m_torrentFileName.getSubName(0, m_torrentFileName.size() - 1) << std::endl;
  return returnedNames;
}

void
SequentialDataFetcher::downloadManifestFiles(const std::vector<ndn::Name>& manifestsName)
{
  std::vector<ndn::Name> packetsName;
  for (auto i = manifestsName.begin(); i != manifestsName.end(); i++) {
    m_manager->download_file_manifest(*i,
                              ".appdata/manifests/",
                              bind(&SequentialDataFetcher::onManifestReceived, this, _1),
                              bind(&SequentialDataFetcher::onDataRetrievalFailure, this, _1, _2));
  }
}

void
SequentialDataFetcher::downloadPackets(const std::vector<ndn::Name>& packetsName)
{
  for (auto i = packetsName.begin(); i != packetsName.end(); i++) {
    m_manager->download_data_packet(*i,
                              bind(&SequentialDataFetcher::onDataPacketReceived, this, _1),
                              bind(&SequentialDataFetcher::onDataRetrievalFailure, this, _1, _2));
  }
}

void
SequentialDataFetcher::implementSequentialLogic() {
  std::vector<ndn::Name> returnedNames;
  returnedNames = this->downloadTorrentFile();
  if (returnedNames.empty()) {
    // we have downloaded the entire torrent (including manifests, data packets, etc..)
    return;
  }
  // check the first returned name whether it is the name of a file manifest or a data packet
  const Name& nameToCheck = returnedNames[0];
  if (IoUtil::findType(nameToCheck) == IoUtil::DATA_PACKET) {
    // In this case, the returned names correspond to data packets
    this->downloadPackets(returnedNames);
  }
  else {
    // In this case, the returned names correspond to file manifests
    this->downloadManifestFiles(returnedNames);
  }
}

void
SequentialDataFetcher::onDataPacketReceived(const ndn::Data& data)
{
  // Data Packet Received
  std::cout << "Data Packet Received: " << data.getName();
}

void
SequentialDataFetcher::onManifestReceived(const std::vector<Name>& packetNames)
{
  std::cout << "Manifest File Received: "
            << packetNames[0].getSubName(0, packetNames[0].size()- 3) << std::endl;
  this->downloadPackets(packetNames);
}

void
SequentialDataFetcher::onDataRetrievalFailure(const ndn::Interest& interest,
                                              const std::string& errorCode)
{
  std::cerr << "Data Retrieval Failed: " << interest.getName() << std::endl;
  
  // Data retrieval failure
  uint32_t nameType = IoUtil::findType(interest.getName());
  if (nameType == IoUtil::TORRENT_FILE) {
    // this should never happen
    std::cerr << "Torrent File Segment Downloading Failed: " << interest.getName();
    this->downloadTorrentFile();
  }
  else if (nameType == IoUtil::FILE_MANIFEST) {
    std::cerr << "Manifest File Segment Downloading Failed: " << interest.getName();
    this->downloadManifestFiles({ interest.getName() });
  }
  else if (nameType == IoUtil::DATA_PACKET) {
    std::cerr << "Data Packet Downloading Failed: " << interest.getName();
    this->downloadPackets({ interest.getName() });
  }
  else {
    // This should never happen
    std::cerr << "Unknown Packet Type Downloading Failed: " << interest.getName();
  }
}

} // namespace ntorrent
} // namespace ndn
