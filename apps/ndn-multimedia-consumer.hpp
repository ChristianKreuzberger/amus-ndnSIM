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

#ifndef NDN_MULTIMEDIACONSUMER_H
#define NDN_MULTIMEDIACONSUMER_H

#include "ndn-file-consumer.hpp"
#include "ndn-file-consumer-cbr.hpp"
#include "ndn-file-consumer-wdw.hpp"

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-app.hpp"

#include "ns3/random-variable.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.hpp"

#include "ns3/traced-callback.h"
#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"

#include "ns3/point-to-point-module.h"

#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "libdash.h"




namespace ns3 {
namespace ndn {




/**
 * @ingroup ndn-apps
 * @brief Ndn application for sending out Interest packets at a "constant" rate (Poisson process)
 */
template<class Parent>
class MultimediaConsumer : public Parent {
  typedef Parent super;
public:
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  MultimediaConsumer();
  virtual ~MultimediaConsumer();

  virtual void
  StartApplication();

  virtual void
  StopApplication();


protected:
  Name m_mpdInterestName;     ///< \brief NDN Name of the Interest (use Name)
  unsigned int m_screenWidth, m_screenHeight;
  std::string m_deviceType;
};

} // namespace ndn
} // namespace ns3

#endif
