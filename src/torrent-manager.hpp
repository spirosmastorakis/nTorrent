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

#include "file-manifest.hpp"
#include "stats-table.hpp"
#include "torrent-file.hpp"

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/link.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/security/key-chain.hpp>

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
   typedef std::function<void(const std::vector<ndn::Name>&)>        TorrentFileReceivedCallback;
   typedef std::function<void(const ndn::Name&, const std::string&)> FailedCallback;

   /*
    * \brief Create a new Torrent manager with the specified parameters.
    * @param torrentFileName The full name of the initial segment of the torrent file
    * @param dataPath The path to the location on disk to use for the torrent data
    * @param face Optional face object to be used for data retrieval
    *
    * The behavior is undefined unless Initialize() is called before calling any other method on a
    * TorrentManger object.
    */
   TorrentManager(const ndn::Name&   torrentFileName,
                  const std::string& dataPath,
                  ndn::Face& face);

   /*
    * \brief Create a new Torrent manager with the specified parameters.
    * @param torrentFileName The full name of the initial segment of the torrent file
    * @param dataPath The path to the location on disk to use for the torrent data
    *
    * The behavior is undefined unless Initialize() is called before calling any other method on a
    * TorrentManger object.
    */
   TorrentManager(const ndn::Name&   torrentFileName,
                  const std::string& dataPath);

  /*
   * @brief Initialize the state of this object.
   *
   * Read and validate from disk all torrent file segments, file manifests, and data packets for
   * the torrent file managed by this object initializing all state in this manager respectively.
   * Also seeds all validated data.
  */
  void
  Initialize();

  /*
   * @brief Download the torrent file
   * @param path The path to write the downloaded segments
   * @return A vector of the file manifest names contained in the torrent file
   *
   */
  std::vector<Name>
  downloadTorrentFile(const std::string& path);

  /*
   * @brief Download the torrent file
   * @param path The path to write the downloaded segments
   * @param onSuccess Callback to be called if we successfully download all the
   *                  segments of the torrent file. It passes the name of the file manifest
   *                  to be downloaded to the callback
   * @param onFailed Callaback to be called if we fail to download a segment of
   *                 the torrent file. It passes the name of the torrent file segment that
   *                 failed to download and a failure reason to the callback
   *
   * This method provides non-blocking downloading of all the torrent file segments
   *
   */
  void
  downloadTorrentFile(const std::string& path,
                      TorrentFileReceivedCallback onSuccess,
                      FailedCallback onFailed);

  /*
   * @brief Download a file manifest
   * @param manifestName The name of the manifest file to be downloaded
   * @param path The path to write the downloaded segments
   * @param onSuccess Callback to be called if we successfully download all the
   *                  segments of the file manifest. It passes the names of the data packets
   *                  to be downloaded to the callback
   * @param onFailed Callaback to be called if we fail to download a segment of
   *                 the file manifest. It passes the name of the data packet that failed
   *                 to download and a failure reason
   *
   * This method provides non-blocking downloading of all the file manifest segments
   *
   */
  void
  download_file_manifest(const Name&              manifestName,
                         const std::string&       path,
                         ManifestReceivedCallback onSuccess,
                         FailedCallback           onFailed);

  /*
   * @brief Download a data packet
   * @param packetName The name of the data packet to be downloaded
   * @param onSuccess Callback to be called if we successfully download the data packet.
   *                  It passes the name of the data packet to the callback
   * @param onFailed Callaback to be called if we fail to download the requested data packet
   *                 It passes the name of the data packet to the callback and a failure reason
   *
   * This method writes the downloaded data packet to m_dataPath on disk
   *
   */
  void
  download_data_packet(const Name&          packetName,
                       DataReceivedCallback onSuccess,
                       FailedCallback       onFailed);

  // Seed the specified 'data' to the network.
  void
  seed(const Data& data) const;

 protected:
  /*
   * \brief Write @p packet composed of torrent date to disk.
   * @param packet The data packet to be written to the disk
   * Write the Data packet to disk, return 'true' if data successfully written to disk 'false'
   * otherwise. Behavior is undefined unless the corresponding file manifest has already been
   * downloaded.
   */
  bool
  writeData(const Data& packet);

  /*
   * \brief Write the @p segment torrent segment to disk at the specified path.
   * @param segment The torrent file segment to be written to disk
   * @param path The path at which to write the torrent file segment
   * Write the segment to disk, return 'true' if data successfully written to disk 'false'
   * otherwise. Behavior is undefined unless @segment is a correct segment for the torrent file of
   * this manager and @p path is the directory used for all segments of this torrent file.
   */
  bool
  writeTorrentSegment(const TorrentFile& segment, const std::string& path);

  /*
   * \brief Write the @p manifest file manifest to disk at the specified @p path.
   * @param manifest The file manifest  to be written to disk
   * @param path The path at which to write the file manifest
   * Write the file manifest to disk, return 'true' if data successfully written to disk 'false'
   * otherwise. Behavior is undefined unless @manifest is a correct file manifest for a file in the
   * torrent file of this manager and @p path is the directory used for all file manifests of this
   * torrent file.
   */
  bool
  writeFileManifest(const FileManifest& manifest, const std::string& path);

  /*
   * \brief Download the segments of the torrent file
   * @param name The name of the torrent file to be downloaded
   * @param path The path to write the torrent file on disk
   * @param manifestNames A vector containing the name of the manifests in the torrent file.
   *                      This parameter will be updated every time we receive a torrent
   *                      file segment
   * @param async Blocking (sync) or non-blocking (async) operation
   * @param onSuccess Optional callback to be called when all the segments of the torrent file
   *                  have been downloaded. The default value is an empty callack. A callback
   *                  should be specified when async is false
   * @param onFailed Optional callback to be called when we fail to download a segment of the
   *                 torrent file. The default value is an empty callack. A callback should be
   *                 specified when async is false
   *
   */
  void
  downloadTorrentFileSegment(const ndn::Name& name,
                             const std::string& path,
                             std::shared_ptr<std::vector<Name>> manifestNames,
                             bool async,
                             TorrentFileReceivedCallback onSuccess,
                             FailedCallback onFailed);

  /*
   * \brief Download the segments of a file manifest
   * @param manifestName The name of the file manifest to be downloaded
   * @param path The path to write the file manifest on disk
   * @param packetNames A vector containing the name of the data packets in the file manifest
   * @param onSuccess Callback to be called when all the segments of the file manifest have been
   *                  downloaded
   * @param onFailed Callback to be called when we fail to download a file manifest segment
   *
   */
  void
  downloadFileManifestSegment(const Name& manifestName,
                              const std::string& path,
                              std::shared_ptr<std::vector<Name>> packetNames,
                              ManifestReceivedCallback onSuccess,
                              FailedCallback onFailed);

  enum {
    // Number of times to retry if a routable prefix fails to retrieve data
    MAX_NUM_OF_RETRIES = 5,
    // Number of Interests to be sent before sorting the stats table
    SORTING_INTERVAL = 100
  };

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

protected:
  /*
   * \brief Find the torrent file segment that we should download
   *        (either we have nothing or we have them all)
   * @return A shared_ptr to the name of the segment to download or
   *         nullptr if we have all the segments
   *
   */
  shared_ptr<Name>
  findTorrentFileSegmentToDownload();

  /*
   * \brief Given a file manifest segment name, find the next file manifest segment
   *        that we should download
   * @param manifestName The name of the file manifest segment that we want to download
   * @return A shared_ptr to the name of the segment to download or
   *         nullptr if we have all the segments
   *
   */
  shared_ptr<Name>
  findManifestSegmentToDownload(const Name& manifestName);

  /*
   * \brief Given a data packet name, find whether we have already downloaded this packet
   * @param dataName The name of the data packet to download
   * @return True if we have already downloaded this packet, false otherwise
   *
   */
  bool
  dataAlreadyDownloaded(const Name& dataName);

  /*
   * \brief Find the segments of all the file manifests that we are missing
   * @param manifestNames The name of the manifest file segments to download (currently missing)
   *                      This parameter is used as an output vector of names
   */
  void
  findFileManifestsToDownload(std::vector<Name>& manifestNames);

  /*
   * \brief Find the names of the data packets of a file manifest that we are currently missing
   * @param manifestName The name of the manifest
   * @param packetNames The name of the data packets to be downloaded
   *                    (used as an output vector of names)
   *
   * No matter what segment of a manifest file the manifestName parameter might refer to, the
   * missing data packets starting from the first segment of this manifest file would be returned
   *
   */
  void
  findDataPacketsToDownload(const Name& manifestName, std::vector<Name>& packetNames);

  /*
   * \brief Find all the data packets that we are currently missing
   * @param packetNames The name of the data packets to be downloaded
   *                    (used as an output vector of names)
   *
   */
  void
  findAllMissingDataPackets(std::vector<Name>& packetNames);

private:
  shared_ptr<Interest>
  createInterest(Name name);

  // Stats table where routable prefixes are stored
  StatsTable                                                          m_statsTable;
  // Face used for network communication
  Face&                                                               m_face;
  // Iterator to the routable prefix that we currently use
  StatsTable::iterator                                                m_stats_table_iter;
  // Number of retries per routable prefix
  uint64_t                                                            m_retries;
  // Number of Interests sent since last sorting
  uint64_t                                                            m_sortingCounter;
  // Keychain instance
  unique_ptr<KeyChain>                                                m_keyChain;
};

inline
TorrentManager::TorrentManager(const ndn::Name&      torrentFileName,
                               const std::string&    dataPath,
                               ndn::Face& face)
: m_fileStates()
, m_torrentSegments()
, m_fileManifests()
, m_torrentFileName(torrentFileName)
, m_dataPath(dataPath)
, m_face(face)
, m_retries(0)
, m_sortingCounter(0)
, m_keyChain(new KeyChain())
{
  // Hardcoded prefixes for now
  // TODO(Spyros): Think of something more clever to bootstrap...
  m_statsTable.insert("/ucla");
  m_statsTable.insert("/arizona");
  m_stats_table_iter = m_statsTable.begin();
}

inline
TorrentManager::TorrentManager(const ndn::Name&      torrentFileName,
                               const std::string&    dataPath)
: m_fileStates()
, m_torrentSegments()
, m_fileManifests()
, m_torrentFileName(torrentFileName)
, m_dataPath(dataPath)
, m_face(*(new ndn::Face()))
, m_retries(0)
, m_sortingCounter(0)
, m_keyChain(new KeyChain())
{
  // Hardcoded prefixes for now
  // TODO(Spyros): Think of something more clever to bootstrap...
  m_statsTable.insert("/ucla");
  m_statsTable.insert("/arizona");
  m_stats_table_iter = m_statsTable.begin();
}

}  // end ntorrent
}  // end ndn

#endif // INCLUDED_TORRENT_FILE_MANAGER_H
