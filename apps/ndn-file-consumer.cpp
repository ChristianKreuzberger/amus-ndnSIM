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
      .AddAttribute("MaxPayloadSize", "The maximum size of the payload of a data packet", UintegerValue(1400),
                    MakeUintegerAccessor(&FileConsumer::m_maxPayloadSize),
                    MakeUintegerChecker<uint32_t>())
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


void
FileConsumer::SendPacket()
{
  // check if active
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  // did we request or receive the manifest yet?
  if (!m_hasReceivedManifest && !m_hasRequestedManifest)
    SendManifestPacket();
  else // if we did, then we can start streaming
    SendFilePacket();
}


///////////////////////////////////////////////////
//          Process outgoing packets             //
///////////////////////////////////////////////////

void
FileConsumer::SendManifestPacket()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  // create the interest name: m_interestName + manifest string (postfix)
  shared_ptr<Name> interestName = make_shared<Name>(m_interestName);
  interestName->append(m_manifestPostfix);

  // create an interest, set nonce
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand.GetValue());
  interest->setName(*interestName);

  // set the interest lifetime
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  // log that we created the interest
  NS_LOG_INFO("> Creating INTEREST for " << interest->getName());
  m_hasRequestedManifest = true;

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

}


void
FileConsumer::SendFilePacket()
{
  if (!m_active)
    return;

  unsigned seq = GetNextSeqNo();

  if (seq > m_maxSeqNo)
    return;

  m_sequenceStatus[seq] = Requested;

  NS_LOG_FUNCTION_NOARGS();

  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);

  nameWithSequence->appendSequenceNumber(seq);

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand.GetValue());
  interest->setName(*nameWithSequence);

  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  NS_LOG_INFO("> File INTEREST (Seq: " << seq << "): " << interest->getName());


  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  m_curSeqNo++;

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
      m_maxSeqNo = ceil(m_fileSize/m_maxPayloadSize)-1;
      m_curSeqNo = 0;

      unsigned int maxSeqNr = m_fileSize/m_maxPayloadSize;
      NS_LOG_DEBUG("Resulting Max Seq Nr = " << maxSeqNr);

      // Trigger OnManifest
      OnManifest(fileSize);

      return;
    }
  }
  // else: this is a normal data packet, process it as such a data packet

  // Get seq_nr from Interest Name
  uint32_t seqNo = interestName.at(-1).toSequenceNumber();
  m_lastSeqNoReceived = seqNo;

  // make sure that we mark this sequence as received
  m_sequenceStatus[seqNo] = Received;

  // trigger OnFileData
  NS_LOG_DEBUG("SeqNo: " << seqNo);
  OnFileData(seqNo, data->getContent().value(), data->getContent().value_size());
}




void
FileConsumer::OnManifest(long fileSize)
{
  // reserve elements in sequence status
  m_sequenceStatus.clear();
  m_sequenceStatus.resize(ceil(m_fileSize/m_maxPayloadSize));

  // Schedule Next Send Event
  m_sendEvent = Simulator::Schedule(Seconds(0.0), &FileConsumer::SendPacket, this);
}


void
FileConsumer::OnFileData(uint32_t seq_nr, const uint8_t* data, unsigned length)
{
  FILE * fp = fopen("out.bin", "ab");
  fwrite(data, sizeof(uint8_t), length, fp);
  fclose(fp);
  if (seq_nr+1 >= ceil(m_fileSize/m_maxPayloadSize))
  {
    OnFileReceived(0,0);
    return;
  }

  ScheduleNextSendEvent();
}

void
FileConsumer::ScheduleNextSendEvent(unsigned int miliseconds)
{
  // Schedule Next Send Event Now
  m_sendEvent = Simulator::Schedule(Seconds(miliseconds), &FileConsumer::SendPacket, this);
}


void
FileConsumer::OnFileReceived(unsigned status, unsigned length)
{
  // do nothing here
  NS_LOG_DEBUG("Finally received the whole file!");
}





} // namespace ndn
} // namespace ns3
