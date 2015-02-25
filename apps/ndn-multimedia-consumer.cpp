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
#include "ns3/system-path.h"

#include "ns3/boolean.h"


#include "utils/ndn-ns3-packet-tag.hpp"
#include "model/ndn-app-face.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include "model/ndn-app-face.hpp"



NS_LOG_COMPONENT_DEFINE("ndn.MultimediaConsumer");


using namespace dash::mpd;

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
                    MakeStringAccessor(&MultimediaConsumer<Parent>::m_deviceType), MakeStringChecker())
      .template AddAttribute("AllowUpscale", "Define whether or not the client has capabilities to upscale content with lower resolutions", BooleanValue(true),
                    MakeBooleanAccessor (&MultimediaConsumer<Parent>::m_allowUpscale), MakeBooleanChecker ())
      .template AddAttribute("AllowDownscale", "Define whether or not the client has capabilities to downscale content with higher resolutions", BooleanValue(false),
                    MakeBooleanAccessor (&MultimediaConsumer<Parent>::m_allowDownscale), MakeBooleanChecker ())
      .template AddAttribute("StartRepresentationId", """Defines the representation ID of the representation to start streaming; "
                          "can be either an ID from the MPD file or one of the following keywords: "
                          "lowest, auto (lowest = the lowest representation available, auto = the best representation that can be streamed based on the estimated bandwidth)",
                          StringValue("auto"),
                    MakeStringAccessor (&MultimediaConsumer<Parent>::m_startRepresentationId), MakeStringChecker ())
                    ;

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

  m_tempDir = ns3::SystemPath::MakeTemporaryDirectoryName();
  NS_LOG_UNCOND("Temporary Directory: " << m_tempDir);
  ns3::SystemPath::MakeDirectories(m_tempDir);

  m_tempMpdFile = m_tempDir + "/mpd.xml.gz";

  super::SetAttribute("FileToRequest", StringValue(m_mpdInterestName.toUri()));
  super::SetAttribute("WriteOutfile", StringValue(m_tempMpdFile));


  m_availableRepresentations.clear();

  // do base stuff
  super::StartApplication();
}


// Stop Application - Cancel any outstanding events
template<class Parent>
void
MultimediaConsumer<Parent>::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS();

  m_availableRepresentations.clear();

  this->mpd = NULL;

  // cleanup base stuff
  super::StopApplication();
}


template<class Parent>
void
MultimediaConsumer<Parent>::OnFileReceived(unsigned status, unsigned length)
{
  // make sure that the file is being properly retrieved by the super class first!
  super::OnFileReceived(status, length);

  // check if file was gziped
  if (m_tempMpdFile.find(".gz") != std::string::npos)
  {
    // file was compressed, decompress it
    std::cerr << "GZIP MPD File " << m_tempMpdFile << " received. Decompressing..." << std::endl;
    std::string newFileName = m_tempMpdFile.substr(0, m_tempMpdFile.find(".gz"));
    FileConsumer::DecompressFile(m_tempMpdFile, newFileName);
    m_tempMpdFile = newFileName;
  }


  std::cerr << "MPD File " << m_tempMpdFile << " received. Parsing now..." << std::endl;


  dash::IDASHManager *manager;
  manager = CreateDashManager();
  mpd = manager->Open((char*)m_tempMpdFile.c_str());

  if (mpd == NULL)
  {
    NS_LOG_ERROR("Error parsing mpd " << m_tempMpdFile);
    return;
  }


  // we are assuming there is only 1 period, get the first one
  IPeriod *currentPeriod = mpd->GetPeriods().at(0);

  // get base URLs
  m_baseURL = "";
  std::vector<dash::mpd::IBaseUrl*> baseUrls = mpd->GetBaseUrls ();

  if (baseUrls.size() > 0)
  {
    if (baseUrls.size() == 1)
    {
      m_baseURL = baseUrls.at(0)->GetUrl();
    } else
    {
      int randUrl = rand() % baseUrls.size();
      std::cerr << "Mutliple base URLs available, selecting a random one... " << std::endl;
      m_baseURL = baseUrls.at(randUrl)->GetUrl();
    }

    std::cerr << "Base URL: " << m_baseURL << std::endl;
  } else
  {
    NS_LOG_ERROR("No Base URL provided in MPD file... exiting.");
    return;
  }

  // Get the adaptation sets, though we are only takeing the first one
  std::vector<IAdaptationSet *> allAdaptationSets = currentPeriod->GetAdaptationSets();

  // we are assuming that allAdaptationSets.size() == 1
  if (allAdaptationSets.size() == 0)
  {
    NS_LOG_ERROR("No adaptation sets found in MPD file... exiting.");
    return;
  }

  // use first adaptation set
  IAdaptationSet* adaptationSet = allAdaptationSets.at(0);

  // check if the adaptation set has an init segment
  // alternatively, the init segment is representation-specific
  std::cerr << "Checking for init segment in adaptation set..." << std::endl;
  std::string initSegment = "";

  if (adaptationSet->GetSegmentBase () && adaptationSet->GetSegmentBase ()->GetInitialization ())
  {
    std::cerr << "Adaptation Set has INIT Segment" << std::endl;
    // get URL to init segment
    initSegment = adaptationSet->GetSegmentBase ()->GetInitialization ()->GetSourceURL ();
    // TODO: request init segment
  }
  else
  {
    std::cerr << "Adaptation Set does not have INIT Segment" << std::endl;
    /*if (adaptationSet->GetRepresentation().at(0)->GetSegmentBase())
    {
      std::cerr << "Alternative: " << adaptationSet->GetRepresentation().at(0)->GetSegmentBase()->GetInitialization()->GetSourceURL() << std::endl;
    }*/
  }

  // get all representations
  std::vector<IRepresentation*> reps = adaptationSet->GetRepresentation();

  std::cerr << "MPD file contains " << reps.size() << " Representations: "  << std::endl;
  std::cerr << "Start Representation: " << m_startRepresentationId << std::endl;

  bool startRepresentationSelected = false;

  std::string firstRepresentationId = "";
  std::string bestRepresentationBasedOnBandwidth = "";

  double downloadSpeed = super::CalculateDownloadSpeed() * 8;

  std::cerr << "Download Speed of MPD file was : " << downloadSpeed << " bits per second" << std::endl;


  for (IRepresentation* rep : reps)
  {
    unsigned int width = rep->GetWidth();
    unsigned int height = rep->GetHeight();

    // if not allowed to upscale, skip this representation
    if (!m_allowUpscale && width < this->m_screenWidth && height < this->m_screenHeight)
    {
      continue;
    }

    // if not allowed to downscale and width/height are too large, skip this representation
    if (!m_allowDownscale && width > this->m_screenWidth && height > this->m_screenHeight)
    {
      continue;
    }

    std::string repId = rep->GetId();

    if (firstRepresentationId == "")
      firstRepresentationId = repId;

    // else: Use this representation and add it to available representations
    std::vector<std::string> dependencies = rep->GetDependencyId ();

    unsigned int requiredDownloadSpeed = rep->GetBandwidth();

    std::cerr << "ID = " << repId << ", DepId=" <<
        dependencies.size() << ", width=" << width << ", height=" << height << ", bwReq=" << requiredDownloadSpeed <<  std::endl;


    if (!startRepresentationSelected && m_startRepresentationId != "lowest")
    {
      if (m_startRepresentationId == "auto")
      {
        // do we have enough bandwidth available?
        if (downloadSpeed > requiredDownloadSpeed)
        {
          // yes we do!
          bestRepresentationBasedOnBandwidth = repId;
        }
      }
      else if (rep->GetId() == m_startRepresentationId)
      {
        std::cerr << "The last representation is the start representation!" << std::endl;
        startRepresentationSelected = true;
      }
    }

    m_availableRepresentations[repId] = rep;
  }

  // check m_startRepresentationId
  if (m_startRepresentationId == "lowest")
  {
    std::cerr << "Using Lowest available representation; ID = " << firstRepresentationId << std::endl;
    m_startRepresentationId = firstRepresentationId;
    startRepresentationSelected = true;
  } else if (m_startRepresentationId == "auto")
  {
    // select representation based on bandwidth
    if (bestRepresentationBasedOnBandwidth != "")
    {
      std::cerr << "Using best representation based on bandwidth; ID = " << bestRepresentationBasedOnBandwidth << std::endl;
      m_startRepresentationId = bestRepresentationBasedOnBandwidth;
      startRepresentationSelected = true;
    }
  }

  // was there a start representation selected?
  if (!startRepresentationSelected)
  {
    // IF NOT, default to lowest
    std::cerr << "No start representation selected, default to lowest available representation; ID = " << firstRepresentationId << std::endl;
    m_startRepresentationId = firstRepresentationId;
    startRepresentationSelected = true;
  }

  // okay, check init segment
  if (initSegment == "")
  {
    std::cerr << "Using init segment of representation " << m_startRepresentationId << std::endl;
    initSegment = m_availableRepresentations[m_startRepresentationId]->GetSegmentBase()->GetInitialization()->GetSourceURL();
    std::cerr << "Init Segment URL = " << initSegment << std::endl;
  }


}





} // namespace ndn
} // namespace ns3