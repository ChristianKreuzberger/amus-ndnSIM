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

#include "ndn-multimedia-consumer.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"


#include "utils/ndn-ns3-packet-tag.hpp"
#include "model/ndn-app-face.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include "model/ndn-app-face.hpp"


NS_LOG_COMPONENT_DEFINE("ndn.MultimediaConsumer");

namespace ns3 {
namespace ndn {

typedef MultimediaConsumer<FileConsumerCbr> MultimediaConsumerCbr;
NS_OBJECT_ENSURE_REGISTERED(MultimediaConsumerCbr);

template<class Parent>
TypeId
MultimediaConsumer<Parent>::GetTypeId(void)
{
  static TypeId tid =
    TypeId((super::GetTypeId ().GetName () + "::MultimediaConsumer").c_str())
      .SetGroupName("Ndn")
      .template SetParent<super>()
      .template AddConstructor<MultimediaConsumer>()
      .template AddAttribute("MpdFileToRequest", "URI to the MPD File to Request", StringValue("/"),
                    MakeNameAccessor(&MultimediaConsumer<Parent>::m_mpdInterestName), MakeNameChecker())
      .template AddAttribute("ScreenWidth", "Width of the screen", UintegerValue(1920),
                    MakeUintegerAccessor(&MultimediaConsumer<Parent>::m_screenWidth), MakeUintegerChecker<uint32_t>())
      .template AddAttribute("ScreenHeight", "Height of the screen", UintegerValue(1080),
                    MakeUintegerAccessor(&MultimediaConsumer<Parent>::m_screenHeight), MakeUintegerChecker<uint32_t>())
      .template AddAttribute("DeviceType", "PC, Laptop, Tablet, Phone, Game Console", StringValue("PC"),
                    MakeStringAccessor(&MultimediaConsumer<Parent>::m_deviceType), MakeStringChecker());

  return tid;
}

template<class Parent>
MultimediaConsumer<Parent>::MultimediaConsumer() : super()
{
  NS_LOG_FUNCTION_NOARGS();
}


template<class Parent>
MultimediaConsumer<Parent>::~MultimediaConsumer()
{
}



///////////////////////////////////////////////////
//             Application Methods               //
///////////////////////////////////////////////////

// Start Application - initialize variables etc...
template<class Parent>
void
MultimediaConsumer<Parent>::StartApplication() // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS();

  NS_LOG_DEBUG("Starting Multimedia Consumer - Device Type: " << m_deviceType);
  NS_LOG_DEBUG("Screen Resolution: " << m_screenWidth << "x" << m_screenHeight);
  NS_LOG_DEBUG("MPD File: " << m_mpdInterestName);
  NS_LOG_DEBUG("SuperClass: " << super::GetTypeId ().GetName ());

  super::SetAttribute("FileToRequest", StringValue(m_mpdInterestName.toUri()));

  // do base stuff
  super::StartApplication();


  dash::mpd::IMPD *mpd;
  dash::IDASHManager *manager;
  manager = CreateDashManager();
}


// Stop Application - Cancel any outstanding events
template<class Parent>
void
MultimediaConsumer<Parent>::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS();

  // cleanup base stuff
  super::StopApplication();
}






} // namespace ndn
} // namespace ns3
