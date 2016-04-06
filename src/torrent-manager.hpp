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
#ifndef INCLUDED_TORRENT_FILE_MANAGER_H
#define INCLUDED_TORRENT_FILE_MANAGER_H

#include "torrent-file.hpp"
#include "file-manifest.hpp"

#include <ndn-cxx/name.hpp>
#include <ndn-cxx/data.hpp>

#include <boost/filesystem/fstream.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = boost::filesystem;

namespace ndn {
namespace ntorrent {

class TorrentManager : noncopyable {
  /**
   * \class TorrentManager
   *
   * \brief A class to manage the interaction with the system in seeding/leaching a torrent
   */
 public:
  typedef std::function<void(const ndn::Name&)>                     DataReceivedCallback;
  typedef std::function<void(const std::vector<ndn::Name>&)>        ManifestReceivedCallback;
  typedef std::function<void(const ndn::Name&, const std::string&)> FailedCallback;

  TorrentManager(const ndn::Name&   torrentFileName,
                 const std::string& dataPath);
  /*
   * \brief Create a new Torrent manager with the specified parameters.
   * @param torrentFileName The full name of the initial segment of the torrent file
   * @param filePath The path to the location on disk to use for the torrent data
   *
   * The behavior is undefined unless Initialize() is called before calling any other method on a
   * TorrentManger object.
   */

  void Initialize();
  /*
   * \brief Initialize the state of this object.
   * Read and validate from disk all torrent file segments, file manifests, and data packets for
   * the torrent file managed by this object initializing all state in this manager respectively.
   * Also seeds all validated data.
   */

  std::vector<Name> downloadTorrentFile(const std::string& path = "");
  // Request from the network the all segments of the 'torrentFileName' and write onto local disk at
  // the specified 'path'.

  void download_file_manifest(const Name&              manifestName,
                              const std::string&       path,
                              ManifestReceivedCallback onSuccess,
                              FailedCallback           onFailed) const;
  // Request from the network the all segments of the 'manifestName' and write onto local disk at
  // the specified 'path'.

  void download_data_packet(const Name&          packetName,
                            DataReceivedCallback onSuccess,
                            FailedCallback       onFailed) const;
  // Request from the network the Data with the specified 'packetName' and write onto local disk at
  // 'm_dataPath'.

  void seed(const Data& data) const;
  // Seed the specified 'data' to the network.

 protected:
  bool writeData(const Data& packet);
  /*
   * \brief Write @p packet composed of torrent date to disk.
   * @param packet The data packet to be written to the disk
   * Write the Data packet to disk, return 'true' if data successfully written to disk 'false'
   * otherwise. Behavior is undefined unless the corresponding file manifest has already been
   * downloaded.
  */

  bool writeTorrentSegment(const TorrentFile& segment, const std::string& path);
  /*
   * \brief Write the @p segment torrent segment to disk at the specified path.
   * @param segment The torrent file segment to be written to disk
   * @param path The path at which to write the torrent file segment
   * Write the segment to disk, return 'true' if data successfully written to disk 'false'
   * otherwise. Behavior is undefined unless @segment is a correct segment for the torrent file of
   * this manager and @p path is the directory used for all segments of this torrent file.
   */

  bool writeFileManifest(const FileManifest& manifest, const std::string& path);
  /*
   * \brief Write the @p manifest file manifest to disk at the specified @p path.
   * @param manifest The file manifest  to be written to disk
   * @param path The path at which to write the file manifest
   * Write the file manifest to disk, return 'true' if data successfully written to disk 'false'
   * otherwise. Behavior is undefined unless @manifest is a correct file manifest for a file in the
   * torrent file of this manager and @p path is the directory used for all file manifests of this
   * torrent file.
   */

  void onDataReceived(const Data& data);

  void onInterestReceived(const Name& name);

  // A map from each fileManifest to corresponding file stream on disk and a bitmap of which Data
  // packets this manager currently has
  mutable std::unordered_map<Name,
                             std::pair<std::shared_ptr<fs::fstream>,
                                       std::vector<bool>>>            m_fileStates;
  // The segments of the TorrentFile this manager has
  std::vector<TorrentFile>                                            m_torrentSegments;
  // The FileManifests this manager has
  std::vector<FileManifest>                                           m_fileManifests;
  // The name of the initial segment of the torrent file for this manager
  Name                                                                m_torrentFileName;
  // The path to the location on disk of the Data packet for this manager
  std::string                                                         m_dataPath;
};

inline
TorrentManager::TorrentManager(const ndn::Name&   torrentFileName,
                               const std::string& dataPath)
: m_fileStates()
, m_torrentSegments()
, m_fileManifests()
, m_torrentFileName(torrentFileName)
, m_dataPath(dataPath)
{
}

}  // end ntorrent
}  // end ndn

#endif // INCLUDED_TORRENT_FILE_MANAGER_H