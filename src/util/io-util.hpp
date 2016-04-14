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

#ifndef INCLUDED_UTIL_IO_UTIL_H
#define INCLUDED_UTIL_IO_UTIL_H

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/util/io.hpp>

#include <set>
#include <string>
#include <vector>

namespace fs = boost::filesystem;

namespace ndn {
namespace ntorrent {

class TorrentFile;
class FileManifest;

class IoUtil {
 public:
  /*
   * An enum used to identify the type of content to which a name refers
   */
  enum NAME_TYPE
  {
    FILE_MANIFEST,
    TORRENT_FILE,
    DATA_PACKET,
    UNKNOWN
  };

  template<typename T>
  static std::vector<T>
  load_directory(const std::string& dirPath,
                 ndn::io::IoEncoding encoding = ndn::io::IoEncoding::BASE_64);

  static std::vector<ndn::Data>
  packetize_file(const fs::path& filePath,
                 const ndn::Name& commonPrefix,
                 size_t dataPacketSize,
                 size_t subManifestSize,
                 size_t subManifestNum);

  /*
   * @brief Write the @p segment torrent segment to disk at the specified path.
   * @param segment The torrent file segment to be written to disk
   * @param path The path at which to write the torrent file segment
   * Write the segment to disk, return 'true' if data successfully written to disk 'false'
   * otherwise. Behavior is undefined unless @segment is a correct segment for the torrent file of
   * this manager and @p path is the directory used for all segments of this torrent file.
   */
  static bool
  writeTorrentSegment(const TorrentFile& segment, const std::string& path);

    /*
   * @brief Write the @p manifest file manifest to disk at the specified @p path.
   * @param manifest The file manifest  to be written to disk
   * @param path The path at which to write the file manifest
   * Write the file manifest to disk, return 'true' if data successfully written to disk 'false'
   * otherwise. Behavior is undefined unless @manifest is a correct file manifest for a file in the
   * torrent file of this manager and @p path is the directory used for all file manifests of this
   * torrent file.
   */
  static bool
  writeFileManifest(const FileManifest& manifest, const std::string& path);

  /*
   * @brief Write @p packet composed of torrent date to disk.
   * @param packet The data packet to be written to the disk
   * @param os The output stream to which to write
   * Write the Data packet to disk, return 'true' if data successfully written to disk 'false'
   * otherwise. Behavior is undefined unless the corresponding file manifest has already been
   * downloaded.
   */
  static bool
  writeData(const Data&         packet,
            const FileManifest& manifest,
            size_t              subManifestSize,
            fs::fstream&        os);

  /*
   * @brief Read a data packet from the provided stream
   * @param packetFullName The fullname of the expected Data packet
   * @param manifest The file manifest for the requested  Data packet
   * @param subManifestSize The number of Data packets in each catalog for this Data packet
   * @param is The stream from which to read the  packet
   * Read the data  packet from the @p is stream, validate it against the provided @p packetFullName
   * and @p manifest, if successful return a pointer to the packet, otherwise return nullptr.
   */
  static std::shared_ptr<Data>
  readDataPacket(const Name&         packetFullName,
                 const FileManifest& manifest,
                 size_t              subManifestSize,
                 fs::fstream&        is);

  /*
   * @brief Return the type of the specified name
   */
  static IoUtil::NAME_TYPE
  findType(const Name& name);

};

template<typename T>
inline std::vector<T>
IoUtil::load_directory(const std::string& dirPath,
                       ndn::io::IoEncoding encoding) {
  std::vector<T> structures;
  std::set<std::string> fileNames;
  if (fs::exists(dirPath)) {
    for(fs::recursive_directory_iterator it(dirPath);
      it !=  fs::recursive_directory_iterator();
      ++it)
    {
      fileNames.insert(it->path().string());
    }
    for (const auto& f : fileNames) {
      auto data_ptr = ndn::io::load<T>(f, encoding);
      if (nullptr != data_ptr) {
        structures.push_back(*data_ptr);
      }
    }
  }
  structures.shrink_to_fit();
  return structures;
}

} // namespace ntorrent
} // namespace ndn

#endif // INCLUDED_UTIL_IO_UTIL_H