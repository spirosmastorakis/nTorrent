#include "util/io-util.hpp"

#include "file-manifest.hpp"
#include "torrent-file.hpp"
#include "util/logging.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>

namespace fs = boost::filesystem;

using std::string;
using std::vector;

namespace ndn {
namespace ntorrent {

std::vector<ndn::Data>
IoUtil::packetize_file(const fs::path& filePath,
                       const ndn::Name& commonPrefix,
                       size_t dataPacketSize,
                       size_t subManifestSize,
                       size_t subManifestNum)
{
  BOOST_ASSERT(0 < dataPacketSize);
  size_t APPROX_BUFFER_SIZE = std::numeric_limits<int>::max(); // 2 * 1024 * 1024 *1024
  auto file_size = fs::file_size(filePath);
  auto start_offset = subManifestNum * subManifestSize * dataPacketSize;
  // determine the number of bytes in this submanifest
  auto subManifestLength = subManifestSize * dataPacketSize;
  auto remainingFileLength = file_size - start_offset;
  subManifestLength = remainingFileLength < subManifestLength
                    ? remainingFileLength
                    : subManifestLength;
  vector<ndn::Data> packets;
  packets.reserve(subManifestLength/dataPacketSize + 1);
  fs::ifstream fs(filePath, fs::ifstream::binary);
  if (!fs) {
    BOOST_THROW_EXCEPTION(Data::Error("IO Error when opening" + filePath.string()));
  }
  // ensure that buffer is large enough to contain whole packets
  // buffer size is either the entire file or the smallest number of data packets >= 2 GB
  auto buffer_size =
    subManifestLength < APPROX_BUFFER_SIZE   ?
    subManifestLength                        :
    APPROX_BUFFER_SIZE % dataPacketSize == 0 ?
    APPROX_BUFFER_SIZE :
    APPROX_BUFFER_SIZE + dataPacketSize - (APPROX_BUFFER_SIZE % dataPacketSize);
  vector<char> file_bytes;
  file_bytes.reserve(buffer_size);
  size_t bytes_read = 0;
  fs.seekg(start_offset);
  while(fs && bytes_read < subManifestLength && !fs.eof()) {
    // read the file into the buffer
    fs.read(&file_bytes.front(), buffer_size);
    auto read_size = fs.gcount();
    if (fs.bad() || read_size < 0) {
      BOOST_THROW_EXCEPTION(Data::Error("IO Error when reading" + filePath.string()));
    }
    bytes_read += read_size;
    char *curr_start = &file_bytes.front();
    for (size_t i = 0u; i < buffer_size; i += dataPacketSize) {
      // Build a packet from the data
      Name packetName = commonPrefix;
      packetName.appendSequenceNumber(packets.size());
      Data d(packetName);
      auto content_length = i + dataPacketSize > buffer_size ? buffer_size - i : dataPacketSize;
      d.setContent(encoding::makeBinaryBlock(tlv::Content, curr_start, content_length));
      curr_start += content_length;
      // append to the collection
      packets.push_back(d);
    }
    file_bytes.clear();
    // recompute the buffer_size
    buffer_size =
      subManifestLength - bytes_read < APPROX_BUFFER_SIZE ?
      subManifestLength - bytes_read                      :
      APPROX_BUFFER_SIZE % dataPacketSize == 0            ?
      APPROX_BUFFER_SIZE                                  :
      APPROX_BUFFER_SIZE + dataPacketSize - (APPROX_BUFFER_SIZE % dataPacketSize);
  }
  fs.close();
  packets.shrink_to_fit();
  ndn::security::KeyChain key_chain;
  // sign all the packets
  for (auto& p : packets) {
    key_chain.sign(p, signingWithSha256());
  }
  return packets;
}

bool IoUtil::writeTorrentSegment(const TorrentFile& segment, const std::string& path)
{
  // validate that this torrent segment belongs to our torrent
  auto segmentNum = segment.getSegmentNumber();
  // write to disk at path
  if (!fs::exists(path)) {
    fs::create_directories(path);
  }
  auto filename = path + to_string(segmentNum);
  // if there is already a file on disk for this torrent segment, determine if we should override
  if (fs::exists(filename)) {
    auto segmentOnDisk_ptr = io::load<TorrentFile>(filename);
    if (nullptr != segmentOnDisk_ptr && *segmentOnDisk_ptr == segment) {
      return false;
    }
  }
  io::save(segment, filename);
  // add to collection
  return true;
}

bool IoUtil::writeFileManifest(const FileManifest& manifest, const std::string& path)
{
  auto subManifestNum = manifest.submanifest_number();
  fs::path filename = path + manifest.file_name() + "/" + to_string(subManifestNum);

  // write to disk at path
  if (!fs::exists(filename.parent_path())) {
    boost::filesystem::create_directories(filename.parent_path());
  }
  // if there is already a file on disk for this file manifest, determine if we should override
  if (fs::exists(filename)) {
    auto submanifestOnDisk_ptr = io::load<FileManifest>(filename.string());
    if (nullptr != submanifestOnDisk_ptr && *submanifestOnDisk_ptr == manifest) {
      return false;
    }
  }
  io::save(manifest, filename.string());
  return true;
}
bool
IoUtil::writeData(const Data& packet, const FileManifest& manifest, size_t subManifestSize, fs::fstream& os)
{
  auto packetName = packet.getName();
  auto packetNum = packetName.get(packetName.size() - 1).toSequenceNumber();
  auto dataPacketSize = manifest.data_packet_size();
  auto initial_offset = manifest.submanifest_number() * subManifestSize * dataPacketSize;
  auto packetOffset =  initial_offset + packetNum * dataPacketSize;
  // write data to disk
  os.seekp(packetOffset);
  try {
    auto content = packet.getContent();
    std::vector<char> data(content.value_begin(), content.value_end());
    os.write(&data[0], data.size());
    return true;
  }
  catch (io::Error &e) {
    LOG_ERROR << e.what() << std::endl;
    return false;
  }
}

std::shared_ptr<Data>
IoUtil::readDataPacket(const Name& packetFullName,
                       const FileManifest& manifest,
                       size_t subManifestSize,
                       fs::fstream& is)
{
  auto dataPacketSize = manifest.data_packet_size();
  auto start_offset = manifest.submanifest_number() * subManifestSize * dataPacketSize;
  auto packetNum = packetFullName.get(packetFullName.size() - 2).toSequenceNumber();
  // seek to packet
  is.sync();
  is.seekg(start_offset + packetNum * dataPacketSize);
  if (is.tellg() < 0) {
    LOG_ERROR << "bad seek" << std::endl;
  }
 // read contents
 std::vector<char> bytes(dataPacketSize);
 is.read(&bytes.front(), dataPacketSize);
 auto read_size = is.gcount();
 if (is.bad() || read_size < 0) {
  LOG_ERROR << "Bad read" << std::endl;
  return nullptr;
 }
 // construct packet
 auto packetName = packetFullName.getSubName(0, packetFullName.size() - 1);
 auto d = make_shared<Data>(packetName);
 d->setContent(encoding::makeBinaryBlock(tlv::Content, &bytes.front(), read_size));
 ndn::security::KeyChain key_chain;
 key_chain.sign(*d, signingWithSha256());
 return d->getFullName() == packetFullName ? d : nullptr;
}

IoUtil::NAME_TYPE
IoUtil::findType(const Name& name)
{
  NAME_TYPE rval = UNKNOWN;
  if (name.get(name.size() - 2).toUri() == "torrent-file" ||
      name.get(name.size() - 3).toUri() == "torrent-file") {
    rval = TORRENT_FILE;
  }
  else if (name.get(name.size() - 2).isSequenceNumber() &&
           name.get(name.size() - 3).isSequenceNumber()) {
    rval = DATA_PACKET;
  }
  else if (name.get(name.size() - 2).isSequenceNumber() &&
           !(name.get(name.size() - 3).isSequenceNumber())) {
    rval = FILE_MANIFEST;
  }
  return rval;
}



} // namespace ntorrent
} // namespace ndn}
