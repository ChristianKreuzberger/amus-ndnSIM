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

#ifndef DASH_MULTIMEDIA_PLAYER
#define DASH_MULTIMEDIA_PLAYER

#include "adaptation-logic.hpp"

#include <string>
#include <typeinfo>
#include <map>
#include <memory>

namespace dash
{
namespace player
{


class MultimediaPlayer
{
friend class AdaptationLogic;
public:
  MultimediaPlayer();
  MultimediaPlayer(std::string AdaptationLogicStr);
  ~MultimediaPlayer();

  void
  AddToBuffer(unsigned int seconds);

  unsigned int
  GetBufferLevel();

  bool
  ConsumeFromBuffer(unsigned int seconds);

  void
  SetAvailableRepresentations(std::map<std::string, IRepresentation*>* availableRepresentations);

  std::shared_ptr<AdaptationLogic>&
  GetAdaptationLogic();

protected:
  unsigned int m_buffer;
  std::shared_ptr<AdaptationLogic> m_adaptLogic;
  std::map<std::string, IRepresentation*>* m_availableRepresentations;
};
}
}

#endif // DASH_MULTIMEDIA_PLAYER
