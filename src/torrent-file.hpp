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

  /**
   * @brief Create a new empty .TorrentFile.
   */
  TorrentFile() = default;

  /**
   * @brief Create a new .TorrentFile.
   * @param torrentFileName The name of the .torrent-file
   * @param torrentFilePtr A pointer (name) to the next segment of the .torrent-file
   * @param commonPrefix The common name prefix of the manifest file names included in the catalog
   * @param catalog The catalog containing the name of each file manifest
   */
  TorrentFile(const Name& torrentFileName,
              const Name& torrentFilePtr,
              const Name& commonPrefix,
              const std::vector<ndn::Name>& catalog);

  /**
   * @brief Create a new .TorrentFile.
   * @param torrentFileName The name of the .torrent-file
   * @param commonPrefix The common name prefix of the manifest file names included in the catalog
   * @param catalog The catalog containing the name of each file manifest
   */
  TorrentFile(const Name& torrentFileName,
              const Name& commonPrefix,
              const std::vector<ndn::Name>& catalog);

  /**
   * @brief Create a new .TorrentFile
   * @param block The block format of the .torrent-file
   */
  explicit
  TorrentFile(const Block& block);

  /**
   * @brief Get the name of the .TorrentFile
   */
  const Name&
  getName() const;

  /**
   * @brief Get the common prefix of the file manifest names of this .torrent-file
   */
  const Name&
  getCommonPrefix() const;

  /**
   * @brief Get a shared pointer to the name of the next segment of the .torrent-file.
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
   * @brief Decode from wire format
   */
  void
  wireDecode(const Block& wire);

  /**
   * @brief Encode from wire format
   */
  const Block&
  wireEncode();

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

protected:
  /**
   * @brief prepend .torrent file as a Content block to the encoder
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
   * @brief Check whether the .torrent-file has a pointer to the next segment
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
   * After decoding a .torrent-file from its wire format, we construct the catalog of
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

private:
  Name m_commonPrefix;
  Name m_torrentFilePtr;
  std::vector<ndn::Name> m_suffixCatalog;
  std::vector<ndn::Name> m_catalog;
};

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

} // namespace ntorrent

} // namespace ndn

#endif // TORRENT_FILE_HPP
