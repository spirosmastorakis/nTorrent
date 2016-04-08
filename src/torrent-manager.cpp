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
  std::set<string> fileNames;
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

// =================================================================================================
//                                      Torrent Manager Utility Functions
// ==================================================================================================

static std::shared_ptr<Data>
readDataPacket(const Name& packetFullName,
               const FileManifest& manifest,
               size_t subManifestSize,
               fs::fstream& is) {
  auto dataPacketSize = manifest.data_packet_size();
  auto start_offset = manifest.submanifest_number() * subManifestSize * dataPacketSize;
  auto packetNum = packetFullName.get(packetFullName.size() - 2).toSequenceNumber();
  // seek to packet
  is.sync();
  is.seekg(start_offset + packetNum * dataPacketSize);
  if (is.tellg() < 0) {
    std::cerr << "bad seek" << std::endl;
  }
 // read contents
 std::vector<char> bytes(dataPacketSize);
 is.read(&bytes.front(), dataPacketSize);
 auto read_size = is.gcount();
 if (is.bad() || read_size < 0) {
  std::cerr << "Bad read" << std::endl;
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
intializeFileManifests(const string& manifestPath, const vector<TorrentFile>& torrentSegments)
{
  security::KeyChain key_chain;

  vector<FileManifest> manifests = load_directory<FileManifest>(manifestPath);
  if (manifests.empty()) {
    return manifests;
  }
  // sign the manifests
  std::for_each(manifests.begin(), manifests.end(),
                [&key_chain](FileManifest& m){
                  key_chain.sign(m,signingWithSha256());
                });

  // put all names of initial manifests from the valid torrent files into a set
  std::vector<ndn::Name> validInitialManifestNames;
  for (const auto& segment : torrentSegments) {
    const auto& catalog = segment.getCatalog();
    validInitialManifestNames.insert(validInitialManifestNames.end(),
                                    catalog.begin(),
                                    catalog.end());
  }
  auto manifest_it =  manifests.begin();
  std::vector<FileManifest> output;
  output.reserve(manifests.size());
  auto validIvalidInitialManifestNames_it = validInitialManifestNames.begin();

  for (auto& initialName : validInitialManifestNames) {
    // starting from the initial segment
    auto& validName = initialName;
    if (manifests.end() == manifest_it) {
      break;
    }
    auto fileName = manifest_it->file_name();
    // sequential collect all valid segments
    while (manifest_it != manifests.end() && manifest_it->getFullName() == validName) {
      output.push_back(*manifest_it);
      if (manifest_it->submanifest_ptr() != nullptr) {
        validName = *manifest_it->submanifest_ptr();
        ++manifest_it;
      }
      else {
        ++manifest_it;
        break;
      }
    }
    // skip the remain segments for this file (all invalid)
    while (manifests.end() != manifest_it && manifest_it->file_name() == fileName) {
      ++manifest_it;
    }
  }
  return output;
}

static vector<Data>
initializeDataPackets(const string&      filePath,
                      const FileManifest manifest,
                      size_t             subManifestSize)
{
  vector<Data> packets;
  auto subManifestNum = manifest.submanifest_number();

  packets =  packetize_file(filePath,
                            manifest.name(),
                            manifest.data_packet_size(),
                            subManifestSize,
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

static std::pair<std::shared_ptr<fs::fstream>, std::vector<bool>>
initializeFileState(const string&       dataPath,
                    const FileManifest& manifest,
                    size_t              subManifestSize)
{
  // construct the file name
  auto fileName = manifest.file_name();
  auto filePath = dataPath + fileName;
  vector<bool> fileBitMap(manifest.catalog().size());
  // if the file does not exist, create an empty placeholder (otherwise cannot set read-bit)
  if (!fs::exists(filePath)) {
    fs::ofstream fs(filePath);
    fs << "";
  }
  auto s = std::make_shared<fs::fstream>(filePath,
                                           fs::fstream::out
                                         | fs::fstream::binary
                                         | fs::fstream::in);
  if (!*s) {
    BOOST_THROW_EXCEPTION(io::Error("Cannot open: " + filePath));
  }
  auto start_offset = manifest.submanifest_number() * subManifestSize * manifest.data_packet_size();
  s->seekg(start_offset);
  s->seekp(start_offset);
  return std::make_pair(s, fileBitMap);
}

//==================================================================================================
//                                    TorrentManager Implementation
//==================================================================================================

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

  // get the submanifest sizes
  for (const auto& m : m_fileManifests) {
    if (m.submanifest_number() == 0) {
      auto manifestFileName = m.file_name();
      m_subManifestSizes[manifestFileName] = m.catalog().size();
    }
  }

  for (const auto& m : m_fileManifests) {
    // construct the file name
    auto fileName = m.file_name();
    fs::path filePath = m_dataPath + fileName;
    // If there are any valid packets, add corresponding state to manager
    if (!fs::exists(filePath)) {
      if (!fs::exists(filePath.parent_path())) {
        boost::filesystem::create_directories(filePath.parent_path());
      }
      continue;
    }
    auto packets = initializeDataPackets(filePath.string(), m, m_subManifestSizes[m.file_name()]);
    if (!packets.empty()) {
      m_fileStates[m.getFullName()] = initializeFileState(m_dataPath,
                                                          m,
                                                          m_subManifestSizes[m.file_name()]);
      auto& fileBitMap = m_fileStates[m.getFullName()].second;
      auto read_it = packets.begin();
      size_t i = 0;
      for (auto name : m.catalog()) {
        if (read_it == packets.end()) {
          break;
        }
        if (name == read_it->getFullName()) {
          ++read_it;
          fileBitMap[i] = true;
        }
        ++i;
      }
      for (const auto& d : packets) {
        seed(d);
      }
    }
  }
  for (const auto& t : m_torrentSegments) {
    seed(t);
  }
  for (const auto& m : m_fileManifests) {
   seed(m);
  }
}

std::vector<Name>
TorrentManager::downloadTorrentFile(const std::string& path)
{
  shared_ptr<Name> searchRes = this->findTorrentFileSegmentToDownload();
  auto manifestNames = make_shared<std::vector<Name>>();
  if (searchRes == nullptr) {
    this->findFileManifestsToDownload(*manifestNames);
    if (manifestNames->empty()) {
      auto packetNames = make_shared<std::vector<Name>>();
      this->findAllMissingDataPackets(*packetNames);
      return *packetNames;
    }
    else {
      return *manifestNames;
    }
  }
  this->downloadTorrentFileSegment(m_torrentFileName, path, manifestNames,
                                  false, {}, {});
  return *manifestNames;
}

void
TorrentManager::downloadTorrentFileSegment(const ndn::Name& name,
                                           const std::string& path,
                                           std::shared_ptr<std::vector<Name>> manifestNames,
                                           bool async,
                                           TorrentFileReceivedCallback onSuccess,
                                           FailedCallback onFailed)
{
  shared_ptr<Interest> interest = createInterest(name);

  auto dataReceived = [manifestNames, path, async, onSuccess, onFailed, this]
                                            (const Interest& interest, const Data& data) {
      // Stats Table update here...
      m_stats_table_iter->incrementReceivedData();
      m_retries = 0;

      if (async) {
        manifestNames->clear();
      }

      TorrentFile file(data.wireEncode());

      // Write the torrent file segment to disk...
      if (writeTorrentSegment(file, path)) {
        // if successfully written, seed this data
        seed(data);
      }

      const std::vector<Name>& manifestCatalog = file.getCatalog();
      manifestNames->insert(manifestNames->end(), manifestCatalog.begin(), manifestCatalog.end());

      shared_ptr<Name> nextSegmentPtr = file.getTorrentFilePtr();

      if (async) {
        onSuccess(*manifestNames);
      }
      if (nextSegmentPtr != nullptr) {
        this->downloadTorrentFileSegment(*nextSegmentPtr, path, manifestNames,
                                         async, onSuccess, onFailed);
      }
  };

  auto dataFailed = [manifestNames, path, name, async, onSuccess, onFailed, this]
                                                (const Interest& interest) {
    ++m_retries;
    if (m_retries >= MAX_NUM_OF_RETRIES) {
      ++m_stats_table_iter;
      if (m_stats_table_iter == m_statsTable.end()) {
        m_stats_table_iter = m_statsTable.begin();
      }
    }
    if (async) {
      onFailed(interest.getName(), "Unknown error");
    }
    this->downloadTorrentFileSegment(name, path, manifestNames, async, onSuccess, onFailed);
  };

  m_face->expressInterest(*interest, dataReceived, dataFailed);

  if (!async) {
    m_face->processEvents();
  }
}

void
TorrentManager::downloadTorrentFile(const std::string& path,
                                    TorrentFileReceivedCallback onSuccess,
                                    FailedCallback onFailed)
{
  shared_ptr<Name> searchRes = this->findTorrentFileSegmentToDownload();
  auto manifestNames = make_shared<std::vector<Name>>();
  if (searchRes == nullptr) {
    this->findFileManifestsToDownload(*manifestNames);
    if (manifestNames->empty()) {
      auto packetNames = make_shared<std::vector<Name>>();
      this->findAllMissingDataPackets(*packetNames);
      onSuccess(*packetNames);
      return;
    }
    else {
      onSuccess(*manifestNames);
      return;
    }
  }
  this->downloadTorrentFileSegment(*searchRes, path, manifestNames,
                                   true, onSuccess, onFailed);
}

void
TorrentManager::download_file_manifest(const Name&              manifestName,
                                       const std::string&       path,
                                       TorrentManager::ManifestReceivedCallback onSuccess,
                                       TorrentManager::FailedCallback           onFailed)
{
  shared_ptr<Name> searchRes = findManifestSegmentToDownload(manifestName);
  auto packetNames = make_shared<std::vector<Name>>();
  if (searchRes == nullptr) {
    this->findDataPacketsToDownload(manifestName, *packetNames);
    onSuccess(*packetNames);
    return;
  }
  this->downloadFileManifestSegment(*searchRes, path, packetNames, onSuccess, onFailed);
}

void
TorrentManager::download_data_packet(const Name& packetName,
                                     DataReceivedCallback onSuccess,
                                     FailedCallback onFailed)
{
  if (this->dataAlreadyDownloaded(packetName)) {
    onSuccess(packetName);
    return;
  }

  shared_ptr<Interest> interest = this->createInterest(packetName);

  auto dataReceived = [onSuccess, onFailed, this]
                                          (const Interest& interest, const Data& data) {
    // Write data to disk...
    if(writeData(data)) {
      seed(data);
    }

    // Stats Table update here...
    m_stats_table_iter->incrementReceivedData();
    m_retries = 0;
    onSuccess(data.getName());
  };
  auto dataFailed = [onFailed, this]
                             (const Interest& interest) {
    m_retries++;
    if (m_retries >= MAX_NUM_OF_RETRIES) {
      m_stats_table_iter++;
      if (m_stats_table_iter == m_statsTable.end())
        m_stats_table_iter = m_statsTable.begin();
    }
    onFailed(interest.getName(), "Unknown failure");
  };

  m_face->expressInterest(*interest, dataReceived, dataFailed);
}

void TorrentManager::seed(const Data& data) {
  m_face->setInterestFilter(data.getFullName(),
                           bind(&TorrentManager::onInterestReceived, this, _1, _2),
                           RegisterPrefixSuccessCallback(),
                           bind(&TorrentManager::onRegisterFailed, this, _1, _2));
}

// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
//                                Protected Helpers
// = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =

bool TorrentManager::writeData(const Data& packet)
{
  // find correct manifest
  const auto& packetName = packet.getName();
  auto manifest_it = std::find_if(m_fileManifests.begin(), m_fileManifests.end(),
                                 [&packetName](const FileManifest& m) {
                                   return m.getName().isPrefixOf(packetName);
                                 });
  if (m_fileManifests.end() == manifest_it) {
    return false;
  }
  // get file state out
  auto& fileState = m_fileStates[manifest_it->getFullName()];

  // if there is no open stream to the file
  if (nullptr == fileState.first) {
    fs::path filePath = m_dataPath + manifest_it->file_name();
    if (!fs::exists(filePath)) {
      fs::create_directories(filePath.parent_path());
    }

    fileState = initializeFileState(m_dataPath,
                                    *manifest_it,
                                    m_subManifestSizes[manifest_it->file_name()]);
  }
  auto packetNum = packetName.get(packetName.size() - 1).toSequenceNumber();
  // if we already have the packet, do not rewrite it.
  if (fileState.second[packetNum]) {
    return false;
  }
  auto dataPacketSize = manifest_it->data_packet_size();
  auto initial_offset = manifest_it->submanifest_number() * m_subManifestSizes[manifest_it->file_name()] * dataPacketSize;
  auto packetOffset =  initial_offset + packetNum * dataPacketSize;
  // write data to disk
  fileState.first->seekp(packetOffset);
  try {
    auto content = packet.getContent();
    std::vector<char> data(content.value_begin(), content.value_end());
    fileState.first->write(&data[0], data.size());
  }
  catch (io::Error &e) {
    std::cerr << e.what() << std::endl;
    return false;
  }
  // update bitmap
  fileState.second[packetNum] = true;
  return true;
}

bool TorrentManager::writeTorrentSegment(const TorrentFile& segment, const std::string& path)
{
  // validate that this torrent segment belongs to our torrent
  auto torrentPrefix = m_torrentFileName.getSubName(0, m_torrentFileName.size() - 1);
  if (!torrentPrefix.isPrefixOf(segment.getName())) {
    return false;
  }

  auto segmentNum = segment.getSegmentNumber();
  // check if we already have it
  if (m_torrentSegments.end() != std::find(m_torrentSegments.begin(), m_torrentSegments.end(),
                                           segment))
  {
    return false;
  }
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
  auto it = std::find_if(m_torrentSegments.begin(), m_torrentSegments.end(),
                         [segmentNum](const TorrentFile& t){
                           return t.getSegmentNumber() > segmentNum;
                        });
  m_torrentSegments.insert(it, segment);
  return true;
}

bool TorrentManager::writeFileManifest(const FileManifest& manifest, const std::string& path)
{
  auto subManifestNum = manifest.submanifest_number();
  fs::path filename = path + manifest.file_name() + "/" + to_string(subManifestNum);
  // check if we already have it
  if (m_fileManifests.end() != std::find(m_fileManifests.begin(), m_fileManifests.end(),
                                         manifest))
  {
    return false;
  }

  // write to disk at path
  if (!fs::exists(filename.parent_path())) {
    boost::filesystem::create_directories(filename.parent_path());
  }
  // if there is already a file on disk for this torrent segment, determine if we should override
  if (fs::exists(filename)) {
    auto submanifestOnDisk_ptr = io::load<FileManifest>(filename.string());
    if (nullptr != submanifestOnDisk_ptr && *submanifestOnDisk_ptr == manifest) {
      return false;
    }
  }
  io::save(manifest, filename.string());
  // add to collection
  auto it = std::find_if(m_fileManifests.begin(), m_fileManifests.end(),
                         [&manifest](const FileManifest& m){
                           return m.file_name() >  manifest.file_name()
                           ||    (m.file_name() == manifest.file_name()
                              && (m.submanifest_number() > manifest.submanifest_number()));
                        });
  // update the state of the manager
  m_fileManifests.insert(it, manifest);
  if (0 == manifest.submanifest_number()) {
    m_subManifestSizes[manifest.file_name()] = manifest.catalog().size();
  }
  return true;
}

void
TorrentManager::downloadFileManifestSegment(const Name& manifestName,
                                            const std::string& path,
                                            std::shared_ptr<std::vector<Name>> packetNames,
                                            TorrentManager::ManifestReceivedCallback onSuccess,
                                            TorrentManager::FailedCallback onFailed)
{
  shared_ptr<Interest> interest = this->createInterest(manifestName);

  auto dataReceived = [packetNames, path, onSuccess, onFailed, this]
                                          (const Interest& interest, const Data& data) {
    // Stats Table update here...
    m_stats_table_iter->incrementReceivedData();
    m_retries = 0;

    FileManifest file(data.wireEncode());

    // Write the file manifest segment to disk...
    if( writeFileManifest(file, path)) {
      seed(file);
    }

    const std::vector<Name>& packetsCatalog = file.catalog();
    packetNames->insert(packetNames->end(), packetsCatalog.begin(), packetsCatalog.end());
    shared_ptr<Name> nextSegmentPtr = file.submanifest_ptr();
    if (nextSegmentPtr != nullptr) {
      this->downloadFileManifestSegment(*nextSegmentPtr, path, packetNames, onSuccess, onFailed);
    }
    else
      onSuccess(*packetNames);
  };

  auto dataFailed = [packetNames, path, manifestName, onFailed, this]
                                                (const Interest& interest) {
    m_retries++;
    if (m_retries >= MAX_NUM_OF_RETRIES) {
      m_stats_table_iter++;
      if (m_stats_table_iter == m_statsTable.end())
        m_stats_table_iter = m_statsTable.begin();
    }
    onFailed(interest.getName(), "Unknown failure");
  };

  m_face->expressInterest(*interest, dataReceived, dataFailed);
}

void
TorrentManager::onInterestReceived(const InterestFilter& filter, const Interest& interest)
{
  // handle if it is a torrent-file
  const auto& interestName = interest.getName();
  std::shared_ptr<Data> data = nullptr;
  auto cmp = [&interestName](const Data& t){return t.getFullName() == interestName;};

  // determine if it is torrent file (that we have)
  auto torrent_it =  std::find_if(m_torrentSegments.begin(), m_torrentSegments.end(), cmp);
  if (m_torrentSegments.end() != torrent_it) {
    data = std::make_shared<Data>(*torrent_it);
  }
  else {
    // determine if it is manifest (that we have)
    auto manifest_it = std::find_if(m_fileManifests.begin(), m_fileManifests.end(), cmp);
    if (m_fileManifests.end() != manifest_it) {
      data = std::make_shared<Data>(*manifest_it) ;
    }
    else {
      // determine if it is data packet (that we have)
      auto manifestName = interestName.getSubName(0, interestName.size() - 2);
      auto map_it = std::find_if(m_fileStates.begin(), m_fileStates.end(),
                                       [&manifestName](const std::pair<Name,
                                                          std::pair<std::shared_ptr<fs::fstream>,
                                                                    std::vector<bool>>>& kv){
                                        return manifestName.isPrefixOf(kv.first);
                                      });
      if (m_fileStates.end() != map_it) {
        auto packetName = interestName.getSubName(0, interestName.size() - 1);
        // get out the bitmap to be sure we have the packet
        auto& fileState = map_it->second;
        const auto &bitmap = fileState.second;
        auto packetNum = packetName.get(packetName.size() - 1).toSequenceNumber();
        if (bitmap[packetNum]) {
          // get the manifest
          auto manifest_it = std::find_if(m_fileManifests.begin(), m_fileManifests.end(),
                                          [&manifestName](const FileManifest& m) {
                                            return manifestName.isPrefixOf(m.name());
                                          });
          auto manifestFileName = manifest_it->file_name();
          auto filePath = m_dataPath + manifestFileName;
          // TODO(msweatt) Explore why fileState stream does not work
          fs::fstream is (filePath, fs::fstream::in | fs::fstream::binary);
          data = readDataPacket(interestName,
                                *manifest_it,
                                m_subManifestSizes[manifestFileName],
                                is);
        }
      }
    }
  }
  if (nullptr != data) {
    m_face->put(*data);
  }
  else {
    // TODO(msweatt) NACK
    std::cerr << "NACK: " << interest << std::endl;
  }
  return;
}

void
TorrentManager::onRegisterFailed(const Name& prefix, const std::string& reason)
{
  std::cerr << "ERROR: Failed to register prefix \""
            << prefix << "\" in local hub's daemon (" << reason << ")"
            << std::endl;
  m_face->shutdown();
}

shared_ptr<Name>
TorrentManager::findTorrentFileSegmentToDownload()
{
  // if we have no segments
  if (m_torrentSegments.empty()) {
    return make_shared<Name>(m_torrentFileName);
  }
  // otherwise just return the next segment ptr of the last segment we have
  return m_torrentSegments.back().getTorrentFilePtr();
}

shared_ptr<Name>
TorrentManager::findManifestSegmentToDownload(const Name& manifestName)
{
  //sequentially find whether we have downloaded any segments of this manifest file
  Name manifestPrefix = manifestName.getSubName(0, manifestName.size() - 2);
  auto it = std::find_if(m_fileManifests.rbegin(), m_fileManifests.rend(),
                      [&manifestPrefix] (const FileManifest& f) {
                        return manifestPrefix.isPrefixOf(f.getName());
                      });

  // if we do not have any segments of the file manifest
  if (it == m_fileManifests.rend()) {
    return make_shared<Name>(manifestName);
  }

  // if we already have the requested segment of the file manifest
  if (it->submanifest_number() >= manifestName.get(manifestName.size() - 2).toSequenceNumber()) {
    return it->submanifest_ptr();
  }
  // if we do not have the requested segment
  else {
    return make_shared<Name>(manifestName);
  }
}

void
TorrentManager::findFileManifestsToDownload(std::vector<Name>& manifestNames)
{
  std::vector<Name> manifests;
  // insert the first segment name of all the file manifests to the vector
  for (auto i = m_torrentSegments.begin(); i != m_torrentSegments.end(); i++) {
    manifests.insert(manifests.end(), i->getCatalog().begin(), i->getCatalog().end());
  }
  // for each file
  for (const auto& manifestName : manifests) {
    // find the first (if any) segment we are missing
    shared_ptr<Name> manifestSegmentName = findManifestSegmentToDownload(manifestName);
    if (nullptr != manifestSegmentName) {
      manifestNames.push_back(*manifestSegmentName);
    }
  }
}

bool
TorrentManager::dataAlreadyDownloaded(const Name& dataName)
{

  auto manifest_it = std::find_if(m_fileManifests.begin(), m_fileManifests.end(),
                                 [&dataName](const FileManifest& m) {
                                   return m.getName().isPrefixOf(dataName);
                                 });

  // if we do not have the file manifest, just return false
  if (manifest_it == m_fileManifests.end()) {
    return false;
  }

  // find the pair of (std::shared_ptr<fs::fstream>, std::vector<bool>)
  // that corresponds to the specific submanifest
  auto& fileState = m_fileStates[manifest_it->getFullName()];

  auto dataNum = dataName.get(dataName.size() - 2).toSequenceNumber();

  // find whether we have the requested packet from the bitmap
  return fileState.second[dataNum];
}

void
TorrentManager::findDataPacketsToDownload(const Name& manifestName, std::vector<Name>& packetNames)
{
  auto manifest_it = std::find_if(m_fileManifests.begin(), m_fileManifests.end(),
                                 [&manifestName](const FileManifest& m) {
                                   return m.name().getSubName(0, m.name().size()
                                                              - 1).isPrefixOf(manifestName);
                                 });

  for (auto j = manifest_it; j != m_fileManifests.end(); j++) {
    auto& fileState = m_fileStates[j->getFullName()];
    for (size_t dataNum = 0; dataNum < j->catalog().size(); ++dataNum) {
      if (!fileState.second[dataNum]) {
        packetNames.push_back(j->catalog()[dataNum]);
      }
    }

    // check that the next manifest in the vector refers to the next segment of the same file
    if ((j + 1) != m_fileManifests.end() && (j+1)->file_name() != manifest_it->file_name()) {
      break;
    }
  }
}

void
TorrentManager::findAllMissingDataPackets(std::vector<Name>& packetNames)
{
  for (auto j = m_fileManifests.begin(); j != m_fileManifests.end(); j++) {
    auto& fileState = m_fileStates[j->getFullName()];
    for (auto i = j->catalog().begin(); i != j->catalog().end(); i++) {
      auto dataNum = i->get(i->size() - 2).toSequenceNumber();
      if (!fileState.second[dataNum]) {
        packetNames.push_back(*i);
      }
    }
  }
}

shared_ptr<Interest>
TorrentManager::createInterest(Name name)
{
  shared_ptr<Interest> interest = make_shared<Interest>(name);
  interest->setInterestLifetime(time::milliseconds(2000));
  interest->setMustBeFresh(true);

  // Select routable prefix
  Link link(name, { {1, m_stats_table_iter->getRecordName()} });
  m_keyChain->sign(link, signingWithSha256());
  Block linkWire = link.wireEncode();

  // Stats Table update here...
  m_stats_table_iter->incrementSentInterests();

  m_sortingCounter++;
  if (m_sortingCounter >= SORTING_INTERVAL) {
    m_sortingCounter = 0;
    m_statsTable.sort();
    m_stats_table_iter = m_statsTable.begin();
    m_retries = 0;
  }

  interest->setLink(linkWire);

  return interest;
}

}  // end ntorrent
}  // end ndn