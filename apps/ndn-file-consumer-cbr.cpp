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

#include "ndn-file-consumer-cbr.hpp"
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


NS_LOG_COMPONENT_DEFINE("ndn.FileConsumerCbr");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(FileConsumerCbr);

TypeId
FileConsumerCbr::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::FileConsumerCbr")
      .SetGroupName("Ndn")
      .SetParent<FileConsumer>()
      .AddConstructor<FileConsumerCbr>()
      .AddAttribute("WindowSize", "The amount of interests that are issued per second", UintegerValue(100),
                    MakeUintegerAccessor(&FileConsumerCbr::m_windowSize),
                    MakeUintegerChecker<uint32_t>())
      /*
      .AddAttribute("FileToRequest", "Name of the File to Request", StringValue("/"),
                    MakeNameAccessor(&FileConsumerCbr::m_interestName), MakeNameChecker())
      .AddAttribute("LifeTime", "LifeTime for Interests", StringValue("2s"),
                    MakeTimeAccessor(&FileConsumerCbr::m_interestLifeTime), MakeTimeChecker())
      .AddAttribute("ManifestPostfix", "The manifest string added after a file", StringValue("/manifest"),
                    MakeStringAccessor(&FileConsumerCbr::m_manifestPostfix), MakeStringChecker())
      .AddAttribute("WriteOutfile", "Write the downloaded file to outfile (empty means disabled)", StringValue(""),
                    MakeStringAccessor(&FileConsumerCbr::m_outFile), MakeStringChecker())
      .AddAttribute("MaxPayloadSize", "The maximum size of the payload of a data packet", UintegerValue(1400),
                    MakeUintegerAccessor(&FileConsumerCbr::m_maxPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddTraceSource("FileDownloadFinished", "Trace called every time a download finishes",
                      MakeTraceSourceAccessor(&FileConsumerCbr::m_downloadFinishedTrace))
      .AddTraceSource("ManifestReceived", "Trace called every time a manifest is received",
                      MakeTraceSourceAccessor(&FileConsumerCbr::m_manifestReceivedTrace))
      .AddTraceSource("FileDownloadStarted", "Trace called every time a download starts",
                      MakeTraceSourceAccessor(&FileConsumerCbr::m_downloadStartedTrace)) */
    ;

  return tid;
}

FileConsumerCbr::FileConsumerCbr() : FileConsumer()
{
  NS_LOG_FUNCTION_NOARGS();
}

FileConsumerCbr::~FileConsumerCbr()
{
}


void
FileConsumerCbr::StartApplication()
{
  FileConsumer::StartApplication();
  m_inFlight = 0;
}

void
FileConsumerCbr::OnFileData(uint32_t seq_nr, const uint8_t* data, unsigned length)
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
  }
}

void
FileConsumerCbr::ScheduleNextSendEvent(unsigned int miliseconds)
{
  NS_LOG_FUNCTION(this << miliseconds);
  // Schedule Next Send Event Now
  m_sendEvent = Simulator::Schedule(Seconds(miliseconds/1000.0), &FileConsumerCbr::SendPacket, this);
}


void
FileConsumerCbr::OnFileReceived(unsigned status, unsigned length)
{
  NS_LOG_FUNCTION(this);
  FileConsumer::OnFileReceived(status, length);
  // TODO: Stop Event Loop
}



void
FileConsumerCbr::OnData(shared_ptr<const Data> data)
{
  NS_LOG_FUNCTION(this);
  m_inFlight--;
  FileConsumer::OnData(data);
}




bool
FileConsumerCbr::SendPacket()
{
  NS_LOG_FUNCTION(this);
  NS_LOG_DEBUG("m_inFlight=" << m_inFlight << ", m_windowSize=" << m_windowSize);
  bool okay = false;
  if (m_inFlight < m_windowSize)
  {
    okay = FileConsumer::SendPacket();
    m_inFlight++;
  }


  if (this->m_fileSize > 0)
  {
    if (AreAllSeqReceived())
    {
      NS_LOG_DEBUG("Done, triggering OnFileReceived...");
      OnFileReceived(0, 0);
    } else {
      // schedule next event
      ScheduleNextSendEvent(floor(1000.0 / (double)m_windowSize));
    }
  }


  return okay;
}


} // namespace ndn
} // namespace ns3
