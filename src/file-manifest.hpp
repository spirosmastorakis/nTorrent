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
#ifndef INCLUDED_FILE_MANIFEST_HPP
#define INCLUDED_FILE_MANIFEST_HPP

#include <cstring>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/name.hpp>

namespace ndn {
namespace ntorrent {

class FileManifest : public Data {
/**
* \class FileManifest
*
* \brief A value semantic type for File manifests
*
*/
 public:
  // TYPES
  class Error : public Data::Error
  {
  public:
    explicit
    Error(const std::string& what)
      : Data::Error(what)
    {
    }
  };

 public:
  // CREATORS
  FileManifest() = delete;

  ~FileManifest() = default;
  /// Destroy this object

  FileManifest(const Name&              name,
               size_t                   dataPacketSize,
               const Name&              commonPrefix,
               const std::vector<Name>& catalog = std::vector<Name>(),
               std::shared_ptr<Name>    subManifestPtr = nullptr);
  /**
  * \brief Creates a new FileManifest with the specified values
  *
  * @param name The Name of this FileManifest
  * @param dataPacketSize The size (except the last) of each data packet in this FileManifest
  * @param commonPrefix The common prefix used for all named in the catalog
  * @param catalog The collection of Names for this FileManifest
  * @param subManifestPtr (optional) The Name for the sub-manifest in this for this file
  */

  FileManifest(const Name&           name,
               size_t                dataPacketSize,
               const Name&           catalogPrefix,
               std::vector<Name>&&   catalog,
               std::shared_ptr<Name> subManifestPtr = nullptr);
  /**
  * \brief Creates a new FileManifest with the specified values
  *
  * @param name The Name of this FileManifest
  * @param dataPacketSize The size (except the last) of each data packet in this FileManifest
  * @param catalogPrefix the common prefix used for all named in the catalog
  * @param catalog The collection of Names for this FileManifest
  * @param subManifestPtr (optional) The Name for the sub-manifest in this for this file
  */

  explicit
  FileManifest(const Block& block);

  FileManifest(const FileManifest& other) = default;
  /// Creates a new FileManifest with same  value as the specified 'other'

  // ACCESSORS
  const Name&
  name() const;
  /// Returns the 'name' of this FileManifest

  const Name&
  catalog_prefix() const;
  /// Returns the 'catalogPrefix' of this FileManfiest.

  size_t
  data_packet_size() const;
  /// Returns the 'data_packet_size' of this FileManifest

  std::shared_ptr<Name>
  submanifest_ptr() const;
  /// Returns the 'submanifest_ptr' of this FileManifest, or 'nullptr' is none exists

  const std::vector<Name>&
  catalog() const;
  /// Returns an unmodifiable reference to the 'catalog' of this FileManifest

 private:
  template<encoding::Tag TAG>
  size_t
  encodeContent(EncodingImpl<TAG>& encoder) const;
  /// Encodes the contents of this object and append the contents to the 'encoder'

 public:
  // MANIPULATORS
  FileManifest&
  operator=(const FileManifest& rhs) = default;
  /// Assigns the value of the specific 'rhs' object to this object.

  void
  push_back(const Name& name);
  /// Appends a Name to the catalog

  bool
  remove(const Name& name);
  /// If 'name' in catalog, removes first instance and returns 'true', otherwise returns 'false'.

  void
  wireDecode(const Block& wire);
  /**
  * \brief Decodes the wire and assign its contents to this FileManifest
  *
  * @param wire the write to be decoded
  * @throws Error if decoding fails
  */

  void
  finalize();
  /*
   * \brief Performs all final processing on this manifest
   *
   * This method should be called once this manifest is populated and *must* be called before
   * signing it or  calling wireEncode().
   */

private:
  void
  decodeContent();
  /// Decodes the contents of this Data packet, assigning its contents to this FileManifest.

  void
  encodeContent();
  /// Encodes the contents of this FileManifest into the content section of its Data packet.

// DATA
 private:
  size_t                 m_dataPacketSize;
  Name                   m_catalogPrefix;
  std::vector<Name>      m_catalog;
  std::shared_ptr<Name>  m_submanifestPtr;
};

/// Non-member functions
bool operator==(const FileManifest& lhs, const FileManifest& rhs);
/// Returns 'true' if 'lhs' and 'rhs' have the same value, 'false' otherwise.

bool operator!=(const FileManifest& lhs, const FileManifest& rhs);
/// Returns 'true' if 'lhs' and 'rhs' have different values, and 'false' otherwise.

inline
FileManifest::FileManifest(
               const Name&              name,
               size_t                   dataPacketSize,
               const Name&              catalogPrefix,
               const std::vector<Name>& catalog,
               std::shared_ptr<Name>    subManifestPtr)
: Data(name)
, m_dataPacketSize(dataPacketSize)
, m_catalogPrefix(catalogPrefix)
, m_catalog(catalog)
, m_submanifestPtr(subManifestPtr)
{
}

inline
FileManifest::FileManifest(
               const Name&           name,
               size_t                dataPacketSize,
               const Name&           catalogPrefix,
               std::vector<Name>&&   catalog,
               std::shared_ptr<Name> subManifestPtr)
: Data(name)
, m_dataPacketSize(dataPacketSize)
, m_catalogPrefix(catalogPrefix)
, m_catalog(catalog)
, m_submanifestPtr(subManifestPtr)
{
}

inline
FileManifest::FileManifest(const Block& block)
: Data()
, m_dataPacketSize(0)
, m_catalogPrefix("")
, m_catalog()
, m_submanifestPtr(nullptr)
{
  wireDecode(block);
}

inline const Name&
FileManifest::name() const
{
  return getName();
}

inline size_t
FileManifest::data_packet_size() const
{
  return m_dataPacketSize;
}

inline const Name&
FileManifest::catalog_prefix() const
{
  return m_catalogPrefix;
}

inline const std::vector<Name>&
FileManifest::catalog() const
{
  return m_catalog;
}

inline std::shared_ptr<Name>
FileManifest::submanifest_ptr() const
{
  return m_submanifestPtr;
}

}  // end ntorrent
}  // end ndn

#endif // INCLUDED_FILE_MANIFEST_HPP
