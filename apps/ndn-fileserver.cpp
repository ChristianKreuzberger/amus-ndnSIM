/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ndn-fileserver.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "model/ndn-app-face.hpp"
#include "model/ndn-ns3.hpp"
#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>
#include <sys/types.h>
#include <sys/stat.h>

#include <math.h>


NS_LOG_COMPONENT_DEFINE("ndn.FileServer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(FileServer);




TypeId
FileServer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::FileServer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<FileServer>()
      .AddAttribute("Prefix", "Prefix, for which FileServer serves the data", StringValue("/"),
                    MakeStringAccessor(&FileServer::m_prefix), MakeStringChecker())
      .AddAttribute("ContentDirectory", "The directory of which FileServer serves the files", StringValue("/"),
                    MakeStringAccessor(&FileServer::m_contentDir), MakeStringChecker())
      .AddAttribute("ManifestPostfix", "The manifest string added after a file", StringValue("/manifest"),
                    MakeStringAccessor(&FileServer::m_postfixManifest), MakeStringChecker())
      .AddAttribute("MaxPayloadSize", "The maximum size of the payload of a data packet", UintegerValue(1400),
                    MakeUintegerAccessor(&FileServer::m_maxPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)), MakeTimeAccessor(&FileServer::m_freshness),
                    MakeTimeChecker())
      .AddAttribute(
         "Signature",
         "Fake signature, 0 valid signature (default), other values application-specific",
         UintegerValue(0), MakeUintegerAccessor(&FileServer::m_signature),
         MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
                    "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&FileServer::m_keyLocator), MakeNameChecker());
  return tid;
}

FileServer::FileServer()
{
  NS_LOG_FUNCTION_NOARGS();
}



// inherited from Application base class.
void
FileServer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
}

void
FileServer::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}




void
FileServer::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;

  Name dataName(interest->getName());
  // get last postfix
  ndn::Name  lastPostfix = dataName.getSubName(dataName.size() -1 );
  NS_LOG_INFO("> LastPostfix = " << lastPostfix);

  bool isManifest = false;
  uint32_t seqNo = -1;

  if (lastPostfix == m_postfixManifest)
  {
    isManifest = true;
  }
  else
  {
    seqNo = dataName.at(-1).toSequenceNumber();
  }

  // remove the last postfix
  dataName = dataName.getPrefix(-1);

  // extract filename and get path to file
  std::string fname = dataName.toUri();  // get the uri from interest
  fname = fname.substr(m_prefix.length(), fname.length()); // remove the prefix
  fname = std::string(m_contentDir).append(fname); // prepend the data path

  // handle manifest or data
  if (isManifest)
  {
    NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Manifest for file " << fname);
    ReturnManifestData(interest, fname);
  } else
  {
    NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Payload for file " << fname);
    NS_LOG_DEBUG("FileName: " << fname);
    NS_LOG_DEBUG("SeqNo: " << seqNo);

    // check if file exists and the sanity of the sequence number requested
    long fileSize = GetFileSize(fname);

    if (fileSize == -1)
      return; // file does not exist, just quit

    if (seqNo > ceil(fileSize / m_maxPayloadSize))
      return; // sequence not available

    // else:
    ReturnPayloadData(interest, fname, seqNo);
  }
}



void
FileServer::ReturnManifestData(shared_ptr<const Interest> interest, std::string& fname)
{
  long fileSize = GetFileSize(fname);

  auto data = make_shared<Data>();
  data->setName(interest->getName());
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  // create content with the file size in it
  data->setContent(reinterpret_cast<const uint8_t*>(&fileSize), sizeof(long));

  NS_LOG_INFO("Manifest: " << fileSize);

  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  signature.setInfo(signatureInfo);
  signature.setValue(Block(&m_signature, sizeof(m_signature)));

  data->setSignature(signature);


  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_face->onReceiveData(*data);
}


void
FileServer::ReturnPayloadData(shared_ptr<const Interest> interest, std::string& fname, uint32_t seqNo)
{
  auto data = make_shared<Data>();
  data->setName(interest->getName());
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  // go to pointer seqNo*m_maxPayloadSize in file
  FILE* fp = fopen(fname.c_str(), "rb");
  fseek(fp, seqNo * m_maxPayloadSize, SEEK_SET);

  auto buffer = make_shared< ::ndn::Buffer>(m_maxPayloadSize);
  size_t actualSize = fread(buffer->get(), sizeof(uint8_t), m_maxPayloadSize, fp);
  fclose(fp);

  if (actualSize < m_maxPayloadSize)
    buffer->resize(actualSize);

  data->setContent(buffer);

  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  signature.setInfo(signatureInfo);
  signature.setValue(Block(&m_signature, sizeof(m_signature)));

  data->setSignature(signature);


  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_face->onReceiveData(*data);
}






// GetFileSize either from m_fileSizes map or from disk
long FileServer::GetFileSize(std::string filename)
{
  // check if is already in m_fileSizes
  if (m_fileSizes.find(filename) != m_fileSizes.end())
  {
    return m_fileSizes[filename];
  }
  // else: query disk for file size

  struct stat stat_buf;
  int rc = stat(filename.c_str(), &stat_buf);

  if (rc == 0)
  {
    m_fileSizes[filename] = stat_buf.st_size;
    return stat_buf.st_size;
  }
  // else: file not found
  NS_LOG_UNCOND("ERROR: File not found: " << filename);
  return -1;
}





} // namespace ndn
} // namespace ns3
