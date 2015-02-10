/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015 - Christian Kreuzberger - based on ndnSIM
 *
 * This file is part of amus-ndnSIM. See AUTHORS for complete list of ndnSIM authors and
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

#include "ndn-file-consumer.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "model/ndn-app-face.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include "model/ndn-app-face.hpp"

#include <math.h>


NS_LOG_COMPONENT_DEFINE("ndn.FileConsumer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(FileConsumer);

TypeId
FileConsumer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::FileConsumer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<FileConsumer>()
      .AddAttribute("FileToRequest", "Name of the File to Request", StringValue("/"),
                    MakeNameAccessor(&FileConsumer::m_interestName), MakeNameChecker())
      .AddAttribute("LifeTime", "LifeTime for Interests", StringValue("2s"),
                    MakeTimeAccessor(&FileConsumer::m_interestLifeTime), MakeTimeChecker())
      .AddAttribute("ManifestPostfix", "The manifest string added after a file", StringValue("/manifest"),
                    MakeStringAccessor(&FileConsumer::m_manifestPostfix), MakeStringChecker())
      .AddAttribute("WriteOutfile", "Write the downloaded file to outfile (empty means disabled)", StringValue(""),
                    MakeStringAccessor(&FileConsumer::m_outFile), MakeStringChecker())
      .AddAttribute("MaxPayloadSize", "The maximum size of the payload of a data packet", UintegerValue(1400),
                    MakeUintegerAccessor(&FileConsumer::m_maxPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddTraceSource("FileDownloadFinished", "Trace called every time a download finishes",
                      MakeTraceSourceAccessor(&FileConsumer::m_downloadFinishedTrace))
      .AddTraceSource("ManifestReceived", "Trace called every time a manifest is received",
                      MakeTraceSourceAccessor(&FileConsumer::m_manifestReceivedTrace))
      .AddTraceSource("FileDownloadStarted", "Trace called every time a download starts",
                      MakeTraceSourceAccessor(&FileConsumer::m_downloadStartedTrace))
    ;

  return tid;
}

FileConsumer::FileConsumer()
{
  NS_LOG_FUNCTION_NOARGS();
}

FileConsumer::~FileConsumer()
{
}

///////////////////////////////////////////////////
//             Application Methods               //
///////////////////////////////////////////////////

// Start Application - initialize variables etc...
void
FileConsumer::StartApplication() // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS();

  // do base stuff
  App::StartApplication();

  // initialize variables
  m_hasReceivedManifest = false;
  m_hasRequestedManifest = false;
  m_fileSize = 0;
  m_curSeqNo = -1;
  m_maxSeqNo = -1;
  m_lastSeqNoReceived = -1;
  m_sequenceStatus.clear();

  if (!m_outFile.empty())
  {
    // create outfile
    FILE* fp = fopen(m_outFile.c_str(), "w");
    fclose(fp);
  }

  // set start time
  _start_time = Simulator::Now().GetMilliSeconds ();

  _shared_interestName = make_shared<Name>(m_interestName);

  m_downloadStartedTrace(this, _shared_interestName);

  // Start requester - schedule "SendPacket" method
  ScheduleNextSendEvent();
}


// Stop Application - Cancel any outstanding events
void
FileConsumer::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS();

  // cancel periodic packet generation
  Simulator::Cancel(m_sendEvent);

  // cleanup base stuff
  App::StopApplication();
}


bool
FileConsumer::SendPacket()
{
  // check if active
  if (!m_active)
    return false;

  NS_LOG_FUNCTION_NOARGS();

  // did we request or receive the manifest yet?
  if (!m_hasReceivedManifest && !m_hasRequestedManifest)
    return SendManifestPacket();
  else // if we did, then we can start streaming
    return SendFilePacket();
}


///////////////////////////////////////////////////
//          Process outgoing packets             //
///////////////////////////////////////////////////

bool
FileConsumer::SendManifestPacket()
{
  if (!m_active)
    return false;

  NS_LOG_FUNCTION_NOARGS();

  shared_ptr<Name> interestNameWithManifest = make_shared<Name>(m_interestName);

  // create the interest name: m_interestName + manifest string (postfix)
  interestNameWithManifest->append(m_manifestPostfix);

  // create an interest, set nonce
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand.GetValue());
  interest->setName(*interestNameWithManifest);

  // set the interest lifetime
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  // log that we created the interest
  NS_LOG_INFO("> Creating INTEREST for " << interest->getName());
  m_hasRequestedManifest = true;

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  return true;

}


bool
FileConsumer::SendFilePacket()
{
  if (!m_active)
    return false;

  unsigned seq = GetNextSeqNo();

  NS_LOG_DEBUG("Requesting Sequence " << seq);

  if (seq > m_maxSeqNo || m_fileSize == 0)
    return false;

  m_sequenceStatus[seq] = Requested;

  NS_LOG_FUNCTION_NOARGS();

  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);

  nameWithSequence->appendSequenceNumber(seq);

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand.GetValue());
  interest->setName(*nameWithSequence);

  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  CreateTimeoutEvent(seq, m_interestLifeTime.GetMilliSeconds());

  NS_LOG_INFO("> File INTEREST (Seq: " << seq << "): " << interest->getName());


  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  m_curSeqNo++;

  return true;
}



// find a sequence number that has not been requested yet or timed out
uint32_t
FileConsumer::GetNextSeqNo()
{
  uint32_t seqNo = 0;

  for ( auto &seqStatus : m_sequenceStatus ) {
    if (seqStatus == NotRequested || seqStatus == TimedOut)
    {
      return seqNo;
    }
    seqNo++;
  }

  return seqNo;
}



bool
FileConsumer::AreAllSeqReceived()
{
  for ( auto &seqStatus : m_sequenceStatus ) {
    if (seqStatus != Received)
    {
      return false;
    }
  }

  return true;

}



void
FileConsumer::CreateTimeoutEvent(uint32_t seqNo, uint32_t timeout)
{
  if (m_chunkTimeoutEvents.find( seqNo ) != m_chunkTimeoutEvents.end())
  {
    m_chunkTimeoutEvents[seqNo].Cancel();
  }

  m_chunkTimeoutEvents[seqNo] = Simulator::Schedule(Seconds((double)timeout/1000.0), &FileConsumer::CheckSeqForTimeout, this, seqNo);
}




void
FileConsumer::CheckSeqForTimeout(uint32_t seqNo)
{
  if (m_sequenceStatus[seqNo] != Received)
  {
    m_sequenceStatus[seqNo] = TimedOut;
    NS_LOG_DEBUG("Timeout occured for seq " << seqNo);
    OnTimeout(seqNo);
  }
}



void
FileConsumer::OnTimeout(uint32_t seq_nr)
{
}


///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
FileConsumer::OnData(shared_ptr<const Data> data)
{
  // check if app is active
  if (!m_active)
    return;

  App::OnData(data); // tracing inside

  // Log some infos
  NS_LOG_FUNCTION(this << data);
  NS_LOG_INFO("< DATA for " << data->getName());


  // get the hopcount
  int hopCount = -1;
  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) {
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount = hopCountTag.Get();
      NS_LOG_DEBUG("Hop count: " << hopCount);
    }
  }

  // get interest name
  ndn::Name interestName = data->getName();

  // Check whether this is a Manifest Packet or a Data Packet
  // Manifest packets will end with m_manifestPostfix
  // only check if we haven't received the manifest yet
  if (!m_hasReceivedManifest)
  {
    ndn::Name  lastPostfix = interestName.getSubName(interestName.size() -1 );

    NS_LOG_INFO("< LastPostfix = " << lastPostfix);

    if (lastPostfix == m_manifestPostfix)
    {
      // this means we have just received the manifest
      // get the content value: cast unit8_t* to long* and then dereference it
      long fileSize =  *((long*)data->getContent().value());

      NS_LOG_DEBUG("Received Manifest! FileSize=" << fileSize);
      m_hasReceivedManifest = true;
      m_fileSize = fileSize;

      if (m_fileSize == -1)
      {
        NS_LOG_UNCOND("ERROR: File not found: " << interestName);
        m_fileSize = 0;
        m_curSeqNo = 0;
        m_maxSeqNo = 0;
      } else
      {
        m_curSeqNo = 0;
        m_maxSeqNo = ceil(m_fileSize/m_maxPayloadSize);
        NS_LOG_DEBUG("Resulting Max Seq Nr = " << m_maxSeqNo);

        // Trigger OnManifest
        OnManifest(fileSize);
      }

      return;
    }
  }
  // else: this is a normal data packet, process it as such a data packet

  // Get seq_nr from Interest Name
  uint32_t seqNo = interestName.at(-1).toSequenceNumber();
  m_lastSeqNoReceived = seqNo;

  // make sure that we mark this sequence as received
  m_sequenceStatus[seqNo] = Received;
  // cancel timeout event
  m_chunkTimeoutEvents[seqNo].Cancel();

  // trigger OnFileData
  NS_LOG_DEBUG("SeqNo: " << seqNo);
  NS_LOG_DEBUG("Contentvaluesize: " << data->getContent().value_size());
  OnFileData(seqNo, data->getContent().value(), data->getContent().value_size());
}




void
FileConsumer::OnManifest(long fileSize)
{
  // reserve elements in sequence status
  m_sequenceStatus.clear();
  m_sequenceStatus.resize(ceil(m_fileSize/m_maxPayloadSize)+1);

  // call trace source
  m_manifestReceivedTrace(this, _shared_interestName, fileSize);

  // Schedule Next Send Event
  ScheduleNextSendEvent();
}


void
FileConsumer::OnFileData(uint32_t seq_nr, const uint8_t* data, unsigned length)
{
  NS_LOG_FUNCTION(this << seq_nr << length);
  // write outfile if defined
  if (!m_outFile.empty())
  {
    FILE * fp = fopen(m_outFile.c_str(), "ab");
    fwrite(data, sizeof(uint8_t), length, fp);
    fclose(fp);
  }

  if (AreAllSeqReceived())
  {
    OnFileReceived(0, 0);
  } else {
    ScheduleNextSendEvent();
  }
}

void
FileConsumer::ScheduleNextSendEvent(unsigned int miliseconds)
{
  NS_LOG_FUNCTION(this << miliseconds);
  // Schedule Next Send Event Now
  m_sendEvent = Simulator::Schedule(Seconds(miliseconds/1000.0), &FileConsumer::SendPacket, this);
}


void
FileConsumer::OnFileReceived(unsigned status, unsigned length)
{
  // do nothing here
  NS_LOG_DEBUG("Finally received the whole file!");

  _finished_time = Simulator::Now().GetMilliSeconds ();
  double downloadSpeed = (double)m_fileSize/ ( (double)(_finished_time - _start_time)/1000.0 );
  NS_LOG_DEBUG("Download finished after " << (_finished_time - _start_time) << "ms; AvgSpeed = " << downloadSpeed << " bytes per second.");

  // call trace source
  this->m_downloadFinishedTrace(this, _shared_interestName, downloadSpeed, (_finished_time - _start_time));
}





} // namespace ndn
} // namespace ns3
