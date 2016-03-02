#include "torrent-manager.hpp"

#include "file-manifest.hpp"
#include "torrent-file.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <ndn-cxx/data.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/io.hpp>

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = boost::filesystem;

using std::string;
using std::vector;

namespace {
// TODO(msweatt) Move this to a utility
template<typename T>
static vector<T>
load_directory(const string& dirPath,
               ndn::io::IoEncoding encoding = ndn::io::IoEncoding::BASE_64) {
  vector<T> structures;

  if (fs::exists(dirPath)) {
    for(fs::directory_iterator it(dirPath);
      it !=  fs::directory_iterator();
      ++it)
    {
      auto data_ptr = ndn::io::load<T>(it->path().string(), encoding);
      if (nullptr != data_ptr) {
        structures.push_back(*data_ptr);
      }
    }
  }
  structures.shrink_to_fit();
  return structures;
}

} // end anonymous

namespace ndn {
namespace ntorrent {

// TODO(msweatt) Move this to a utility
static vector<ndn::Data>
packetize_file(const fs::path& filePath,
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
    BOOST_THROW_EXCEPTION(FileManifest::Error("IO Error when opening" + filePath.string()));
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
      BOOST_THROW_EXCEPTION(FileManifest::Error("IO Error when reading" + filePath.string()));
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

static vector<TorrentFile>
intializeTorrentSegments(const string& torrentFilePath, const Name& initialSegmentName)
{
  security::KeyChain key_chain;
  Name currSegmentFullName = initialSegmentName;
  vector<TorrentFile> torrentSegments = load_directory<TorrentFile>(torrentFilePath);
  // Starting with the initial segment name, verify the names, loading next name from torrentSegment
  for (auto it = torrentSegments.begin(); it != torrentSegments.end(); ++it) {
    TorrentFile& segment = *it;
    key_chain.sign(segment, signingWithSha256());
    if (segment.getFullName() != currSegmentFullName) {
      vector<TorrentFile> correctSegments(torrentSegments.begin(), it);
      torrentSegments.swap(correctSegments);
      break;
    }
    // load the next full name
    if (nullptr == segment.getTorrentFilePtr()) {
      break;
    }
    currSegmentFullName = *segment.getTorrentFilePtr();
  }
  return torrentSegments;
}

static vector<FileManifest>
intializeFileManifests(const string& manifestPath, vector<TorrentFile> torrentSegments)
{
  security::KeyChain key_chain;

  vector<FileManifest> manifests = load_directory<FileManifest>(manifestPath);

  // sign the manifests
  std::for_each(manifests.begin(), manifests.end(),
                [&key_chain](FileManifest& m){
                  key_chain.sign(m,signingWithSha256());
                });

  // put all names of manifests from the valid torrent files into a set
  std::set<ndn::Name> validManifestNames;
  for (const auto& segment : torrentSegments) {
    const auto& catalog = segment.getCatalog();
    validManifestNames.insert(catalog.begin(), catalog.end());
  }

  // put all names of file manifests from disk into a set
  std::set<ndn::Name> loadedManifestNames;
  std::for_each(manifests.begin(), manifests.end(),
                [&loadedManifestNames](const FileManifest& m){
                  loadedManifestNames.insert(m.getFullName());
                });

  // the set of fileManifests that we have is simply the intersection
  std::set<Name> output;
  std::set_intersection(validManifestNames.begin() , validManifestNames.end(),
                        loadedManifestNames.begin(), loadedManifestNames.end(),
                        std::inserter(output, output.begin()));

  // filter out those manifests that are not in this set
  std::remove_if(manifests.begin(),
                 manifests.end(),
                 [&output](const FileManifest& m) {
                   return (output.end() == output.find(m.name()));
                 });

  // order the manifests in the same order they are in the torrent
  std::vector<Name> catalogNames;
  for (const auto& segment : torrentSegments) {
    const auto& catalog = segment.getCatalog();
    catalogNames.insert(catalogNames.end(), catalog.begin(), catalog.end());
  }
  size_t curr_index = 0;
  for (auto name : catalogNames) {
    auto it = std::find_if(manifests.begin(), manifests.end(),
                          [&name](const FileManifest& m) {
                            return m.getFullName() == name;
                           });
    if (it != manifests.end()) {
      // not already in the correct position
      if (it != manifests.begin() + curr_index) {
        std::swap(manifests[curr_index], *it);
      }
      ++curr_index;
    }
  }

  return manifests;
}

static vector<Data>
intializeDataPackets(const string&      filePath,
                     const FileManifest manifest,
                     const TorrentFile& torrentFile)
{
  vector<Data> packets;
  auto subManifestNum = manifest.name().get(manifest.name().size() - 1).toSequenceNumber();

  packets =  packetize_file(filePath,
                             manifest.name(),
                             manifest.data_packet_size(),
                             manifest.catalog().size(),
                             subManifestNum);

  auto catalog = manifest.catalog();

  // Filter out invalid packet names
  std::remove_if(packets.begin(), packets.end(),
                [&packets, &catalog](const Data& p) {
                  return catalog.end() == std::find(catalog.begin(),
                                                     catalog.end(),
                                                     p.getFullName());
                });
  return packets;
}

void TorrentManager::Initialize()
{
  // .../<torrent_name>/torrent-file/<implicit_digest>
  string dataPath = ".appdata/" + m_torrentFileName.get(-3).toUri();
  string manifestPath = dataPath +"/manifests";
  string torrentFilePath = dataPath +"/torrent_files";

  // get the torrent file segments and manifests that we have.
  if (!fs::exists(torrentFilePath)) {
    return;
  }
  m_torrentSegments = intializeTorrentSegments(torrentFilePath, m_torrentFileName);
  if (m_torrentSegments.empty()) {
    return;
  }
  m_fileManifests   = intializeFileManifests(manifestPath, m_torrentSegments);
  auto currTorrentFile_it = m_torrentSegments.begin();
  for (const auto& m : m_fileManifests) {
    // find the appropriate torrent file
    auto currCatalog = currTorrentFile_it->getCatalog();
    while (currCatalog.end() == std::find(currCatalog.begin(), currCatalog.end(), m.getFullName()))
    {
      ++currTorrentFile_it;
      currCatalog = currTorrentFile_it->getCatalog();
    }
    // construct the file name
    auto fileName = m.name().getSubName(1, m.name().size() - 2).toUri();
    auto filePath = m_dataPath + fileName;
    // If there are any valid packets, add corresponding state to manager
    auto packets = intializeDataPackets(filePath, m, *currTorrentFile_it);
    if (!packets.empty()) {
      // build the bit map
      auto catalog =  m.catalog();
      vector<bool> fileBitMap(catalog.size());
      auto read_it = packets.begin();
      size_t i = 0;
      for (auto name : catalog) {
        if (name == read_it->getFullName()) {
          ++read_it;
          fileBitMap[i]= true;
        }
        ++i;
      }
      for (const auto& d : packets) {
        seed(d);
      }
      auto s = std::make_shared<fs::fstream>(filePath, fs::fstream::binary
                                                     | fs::fstream::in
                                                     | fs::fstream::out);
      m_fileStates[m.getFullName()] = std::make_pair(s, fileBitMap);
    }
  }
  for (const auto& t : m_torrentSegments) {
    seed(t);
  }
  for (const auto& m : m_fileManifests) {
    seed(m);
  }

}

void TorrentManager::seed(const Data& data) const {
  // TODO(msweatt) IMPLEMENT ME
}

}  // end ntorrent
}  // end ndn