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

#ifndef TORRENT_FILE_HPP
#define TORRENT_FILE_HPP

#include "file-manifest.hpp"

#include <ndn-cxx/name.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/data.hpp>

#include <memory>

namespace ndn {

namespace ntorrent {

class TorrentFile : public Data {
public:
  class Error : public Data::Error
  {
  public:
    explicit
    Error(const std::string& what)
      : Data::Error(what)
    {
    }
  };
  static ndn::Name
  torrentFileName(const Name& name);

  /**
   * @brief Create a new empty TorrentFile.
   */
  TorrentFile() = default;

  /**
   * @brief Create a new TorrentFile.
   * @param torrentFileName The name of the torrent-file
   * @param torrentFilePtr A pointer (name) to the next segment of the torrent-file
   * @param commonPrefix The common name prefix of the manifest file names included in the catalog
   * @param catalog The catalog containing the name of each file manifest
   */
  TorrentFile(const Name& torrentFileName,
              const Name& torrentFilePtr,
              const Name& commonPrefix,
              const std::vector<ndn::Name>& catalog);

  /**
   * @brief Create a new TorrentFile.
   * @param torrentFileName The name of the torrent-file
   * @param commonPrefix The common name prefix of the manifest file names included in the catalog
   * @param catalog The catalog containing the name of each file manifest
   */
  TorrentFile(const Name& torrentFileName,
              const Name& commonPrefix,
              const std::vector<ndn::Name>& catalog);

  /**
   * @brief Create a new TorrentFile
   * @param block The block format of the torrent-file
   */
  explicit
  TorrentFile(const Block& block);

  /**
   * @brief Get the name of the TorrentFile
   */
  const Name&
  getName() const;

  /**
   * @brief Get the common prefix of the file manifest names of this torrent-file
   */
  const Name&
  getCommonPrefix() const;

  /**
   * @brief Get a shared pointer to the name of the next segment of the torrent-file.
   *
   * If there is no next segment, it returns a nullptr
   */
  shared_ptr<Name>
  getTorrentFilePtr() const;

  /**
   * @brief Get the catalog of names of the file manifests
   */
  const std::vector<Name>&
  getCatalog() const;

  /**
   * @brief Get the segment number for this torrent file
   */
  size_t
  getSegmentNumber() const;

  /**
   * @brief Get the directory name for this torrent file
   */
  std::string
  getTorrentFilePath() const;

  /**
   * @brief Decode from wire format
   */
  void
  wireDecode(const Block& wire);

  /**
   * @brief Finalize torrent-file before signing the data packet
   *
   * This method has to be called (every time) right before signing or encoding
   * the torrent-file
   */
  void
  finalize();

  /**
   * @brief Insert a name to the catalog of file manifest names
   */
  void
  insert(const Name& name);

  /**
   * @brief Erase a name from the catalog of file manifest names
   */
  bool
  erase(const Name& name);

  /**
   * @brief Get the size of the catalog of file manifest names
   */
  size_t
  catalogSize() const;

  /**
   * @brief Given a directory path for the torrent file, it generates the torrent file
   *
   * @param directoryPath The path to the directory for which we are to create a torrent-file
   * @param torrentFilePrefix The prefix to be used for the name of this torrent-file
   * @param namesPerSegment The number of manifest names to be included in each segment of the
   *        torrent-file
   * @param returnData Determines whether the data would be returned in memory or it will be
   *        stored on disk without being returned
   *
   * Generates the torrent-file for the directory at the specified 'directoryPath',
   * splitting the torrent-file into multiple segments, each one of which contains
   * at most 'namesPerSegment' number of manifest names
   *
   **/
  static std::pair<std::vector<TorrentFile>,
            std::vector<std::pair<std::vector<FileManifest>,
                                  std::vector<Data>>>>
  generate(const std::string& directoryPath,
           size_t namesPerSegment,
           size_t subManifestSize,
           size_t dataPacketSize,
           bool returnData = false);

protected:
  /**
   * @brief prepend torrent file as a Content block to the encoder
   */
  template<encoding::Tag TAG>
  size_t
  encodeContent(EncodingImpl<TAG>& encoder) const;

  void
  encodeContent();

  void
  decodeContent();

private:
  /**
   * @brief Check whether the torrent-file has a pointer to the next segment
   */
  bool
  hasTorrentFilePtr() const;

  /**
   * @brief Create a catalog of suffixes for the names of the file manifests
   *
   * To optimize encoding and decoding, we encode the name of the file manifests
   * as suffixes along with their common prefix.
   *
   */
  void
  createSuffixCatalog();

  /**
   * @brief Construct the catalog of long names from a catalog of suffixes for the file
   *        manifests' name
   *
   * After decoding a torrent-file from its wire format, we construct the catalog of
   * long names from the decoded common prefix and suffixes
   *
   */
  void
  constructLongNames();

  /**
   * @brief Insert a suffix to the suffix catalog
   */
  void
  insertToSuffixCatalog(const PartialName& suffix);

  /**
   * @brief Set the pointer of the current torrent-file segment to the next segment
   */
  void
  setTorrentFilePtr(const Name& ptrName);

private:
  Name m_commonPrefix;
  Name m_torrentFilePtr;
  std::vector<ndn::Name> m_suffixCatalog;
  std::vector<ndn::Name> m_catalog;
};

inline
ndn::Name
TorrentFile::torrentFileName(const Name& name) {
  return (name.get(name.size() - 2).isSequenceNumber())
    ? name.getSubName(0, name.size() - 2)
    : name.getSubName(0, name.size() - 1);
}

inline bool
TorrentFile::hasTorrentFilePtr() const
{
  return !m_torrentFilePtr.empty();
}

inline const std::vector<Name>&
TorrentFile::getCatalog() const
{
  return m_catalog;
}

inline size_t
TorrentFile::catalogSize() const
{
  return m_catalog.size();
}

inline void
TorrentFile::insert(const Name& name)
{
  m_catalog.push_back(name);
}

inline void
TorrentFile::insertToSuffixCatalog(const PartialName& suffix)
{
  m_suffixCatalog.push_back(suffix);
}

inline void
TorrentFile::setTorrentFilePtr(const Name& ptrName)
{
  m_torrentFilePtr = ptrName;
}

inline const Name&
TorrentFile::getName() const
{
  return Data::getName();
}

inline const Name&
TorrentFile::getCommonPrefix() const
{
  return m_commonPrefix;
}

inline std::string
TorrentFile::getTorrentFilePath() const
{
  Name commonPrefix(SharedConstants::commonPrefix);
  return torrentFileName(getFullName()).get(commonPrefix.size() + 1).toUri();
}

inline size_t
TorrentFile::getSegmentNumber() const
{
  const auto& lastComponent = getName().get(getName().size() - 1);
  return lastComponent.isSequenceNumber() ? lastComponent.toSequenceNumber() : 0;
}


} // namespace ntorrent

} // namespace ndn

#endif // TORRENT_FILE_HPP
