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

#include "ndn-file-consumer-wdw.hpp"
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


NS_LOG_COMPONENT_DEFINE("ndn.FileConsumerWdw");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(FileConsumerWdw);

TypeId
FileConsumerWdw::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::FileConsumerWdw")
      .SetGroupName("Ndn")
      .SetParent<FileConsumer>()
      .AddConstructor<FileConsumerWdw>()
      .AddAttribute("MaxWindowSize", "The max. amount of interests that are issued per second", UintegerValue(100),
                    MakeUintegerAccessor(&FileConsumerWdw::m_maxWindowSize),
                    MakeUintegerChecker<uint32_t>())
      /*
      .AddAttribute("FileToRequest", "Name of the File to Request", StringValue("/"),
                    MakeNameAccessor(&FileConsumerWdw::m_interestName), MakeNameChecker())
      .AddAttribute("LifeTime", "LifeTime for Interests", StringValue("2s"),
                    MakeTimeAccessor(&FileConsumerWdw::m_interestLifeTime), MakeTimeChecker())
      .AddAttribute("ManifestPostfix", "The manifest string added after a file", StringValue("/manifest"),
                    MakeStringAccessor(&FileConsumerWdw::m_manifestPostfix), MakeStringChecker())
      .AddAttribute("WriteOutfile", "Write the downloaded file to outfile (empty means disabled)", StringValue(""),
                    MakeStringAccessor(&FileConsumerWdw::m_outFile), MakeStringChecker())
      .AddAttribute("MaxPayloadSize", "The maximum size of the payload of a data packet", UintegerValue(1400),
                    MakeUintegerAccessor(&FileConsumerWdw::m_maxPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddTraceSource("FileDownloadFinished", "Trace called every time a download finishes",
                      MakeTraceSourceAccessor(&FileConsumerWdw::m_downloadFinishedTrace))
      .AddTraceSource("ManifestReceived", "Trace called every time a manifest is received",
                      MakeTraceSourceAccessor(&FileConsumerWdw::m_manifestReceivedTrace))
      .AddTraceSource("FileDownloadStarted", "Trace called every time a download starts",
                      MakeTraceSourceAccessor(&FileConsumerWdw::m_downloadStartedTrace)) */
    ;

  return tid;
}

FileConsumerWdw::FileConsumerWdw() : FileConsumer()
{
  NS_LOG_FUNCTION_NOARGS();
}

FileConsumerWdw::~FileConsumerWdw()
{
}


void
FileConsumerWdw::StartApplication()
{
  FileConsumer::StartApplication();
  m_inFlight = 0;
  m_windowSize = 2;
  m_windowThreshold = 0;

  averageTimeout = 1000;

  received_packets_during_this_window = 0;
  timeouts_during_this_window = 0;
  lastSeqNoRecv = 0;

  packets_received = packets_sent = packets_timeout = 0;
}

void
FileConsumerWdw::OnFileData(uint32_t seq_nr, const uint8_t* data, unsigned length)
{
  NS_LOG_FUNCTION(this << seq_nr << length);


  if (seq_nr != lastSeqNoRecv+1 && seq_nr != 0 && lastSeqNoRecv != 0)
  {
    m_wrongSeqOrder = true;
    NS_LOG_UNCOND("wrong seq order received, decreasing window size immediately");
    m_windowSize = m_windowThreshold;
  }

  lastSeqNoRecv = seq_nr;



  long diff  = Simulator::Now().GetMilliSeconds() - m_sequenceSendTime[seq_nr];

  NS_LOG_UNCOND("RTT for seq " << seq_nr << " was: " << diff << "ms");


  averageTimeout = (averageTimeout * 2 + diff)/3;

  NS_LOG_UNCOND("Average Timeout: " << averageTimeout);



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
FileConsumerWdw::ScheduleNextSendEvent(double miliseconds)
{
  NS_LOG_FUNCTION(this << miliseconds);
  m_sendEvent.Cancel();
  // Schedule Next Send Event Now
  m_sendEvent = Simulator::Schedule(NanoSeconds(miliseconds*1000000.0), &FileConsumerWdw::SendPacket, this);
}


void
FileConsumerWdw::OnFileReceived(unsigned status, unsigned length)
{
  NS_LOG_FUNCTION(this);
  FileConsumer::OnFileReceived(status, length);
  // TODO: Stop Event Loop
}



void
FileConsumerWdw::OnData(shared_ptr<const Data> data)
{
  NS_LOG_FUNCTION(this);
  m_inFlight--;
  packets_received++;

  received_packets_during_this_window++;

  // if we got window size packets, then all is good
  if (received_packets_during_this_window >= m_windowSize)
  {
    if (m_wrongSeqOrder == false && timeouts_during_this_window < 2)
    {
      NS_LOG_UNCOND("increasing window... (cur=" << m_windowSize << ")");
      IncrementWindow();
      NS_LOG_UNCOND("inFlight=" << m_inFlight << ", Window at: " << m_windowSize << ", last good: " << m_windowThreshold);
    } else
    {
      // something is wrong, need to react on it now --> decrease window size

    }

    m_wrongSeqOrder = false;
    received_packets_during_this_window = 0;
    timeouts_during_this_window = 0;

  }

  FileConsumer::OnData(data);
}


void
FileConsumerWdw::IncrementWindow()
{
  if (2*m_windowSize < m_maxWindowSize)
  {
    m_windowSize = m_windowSize * 2;
  } else
  {
    m_windowSize = m_windowSize + m_windowSize/4;
  }

  // prevent becoming bigger than the reshold
  if (m_windowSize > m_maxWindowSize)
    m_windowSize = m_maxWindowSize;


  if (m_windowSize > 2 * m_windowThreshold)
  {
    m_windowThreshold = m_windowSize/2;
  }
}

void
FileConsumerWdw::DecrementWindow()
{
  if (m_windowSize > m_windowThreshold)
  {
    m_windowSize = m_windowThreshold;
  }
}




bool
FileConsumerWdw::SendPacket()
{
  NS_LOG_FUNCTION(this);
  NS_LOG_DEBUG("m_inFlight=" << m_inFlight << ", m_windowSize=" << m_windowSize);
  NS_LOG_DEBUG("Packets_Sent=" << packets_sent << ", packets_Recv=" << packets_received << ", packets_Timeout=" << packets_timeout);
  bool okay = false;

  okay = FileConsumer::SendPacket();
  packets_sent++;
  m_inFlight++;

  if (this->m_fileSize > 0)
  {
    if (AreAllSeqReceived())
    {
      NS_LOG_DEBUG("Done, triggering OnFileReceived...");
      OnFileReceived(0, 0);
    } else {
      // schedule next event
      ScheduleNextSendEvent((1000.0 / (double)m_windowSize));
    }
  }


  return okay;
}



void
FileConsumerWdw::OnTimeout(uint32_t seqNo)
{
  NS_LOG_UNCOND("timeout occured..." << seqNo);
  timeouts_during_this_window++;

  m_windowSize =m_windowThreshold;

  m_inFlight--;
  packets_timeout++;
}


} // namespace ndn
} // namespace ns3
