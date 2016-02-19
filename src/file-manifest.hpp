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
#include <memory>
#include <string>
#include <utility>
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
  // CLASS METHODS
  static std::vector<FileManifest>
  generate(const std::string& filePath,
           const ndn::Name&   manifestPrefix,
           size_t             subManifestSize,
           size_t             dataPacketSize);


  static std::pair<std::vector<FileManifest>, std::vector<Data>>
  generate(const std::string& filePath,
           const ndn::Name&   manifestPrefix,
           size_t             subManifestSize,
           size_t             dataPacketSize,
           bool               returnData);
  /**
   * \brief Generates the FileManifest(s) and Data packets for the file at the specified 'filePath'
   *
   * @param filePath The path to the file for which we are to create a manifest
   * @param manifestPrefix The prefix to be used for the name of this manifest
   * @param subManifestSize The maximum number of data packets to be included in a sub-manifest
   * @param dataPacketSize The maximum number of bytes per Data packet packets for the file
   * @param returnData If true also return the Data
   *
   * @throws Error if there is any I/O issue when trying to read the filePath.
   *
   * Generates the FileManfiest(s) for the file at the specified 'filePath', splitting the manifest
   * into sub-manifests of size at most the specified 'subManifestSize'. Each sub-manifest is
   * composed of a  catalog of Data packets of at most the specified 'dataPacketSize'. Returns all
   * of the manifests that were created in order. The behavior is undefined unless the
   * trailing component of of the manifestPrefix is a subComponent filePath and
   '* O < subManifestSize' and '0 < dataPacketSize'.
   */

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
  set_submanifest_ptr(std::shared_ptr<Name> subManifestPtr);
  /// Sets the sub-manifest pointer of manifest to the specified 'subManifestPtr'

  void
  push_back(const Name& name);
  /// Appends a Name to the catalog

  void
  reserve(size_t capacity);
  /// Reserve memory in the catalog adequate to hold at least 'capacity' Names.

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
std::vector<FileManifest>
FileManifest::generate(const std::string& filePath,
                       const ndn::Name&   manifestPrefix,
                       size_t             subManifestSize,
                       size_t             dataPacketSize)
{
  return generate(filePath, manifestPrefix, subManifestSize, dataPacketSize, false).first;
}

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

inline void
FileManifest::set_submanifest_ptr(std::shared_ptr<Name> subManifestPtr)
{
  m_submanifestPtr = subManifestPtr;
}

inline void
FileManifest::reserve(size_t capacity)
{
  m_catalog.reserve(capacity);
}

}  // end ntorrent
}  // end ndn

#endif // INCLUDED_FILE_MANIFEST_HPP
