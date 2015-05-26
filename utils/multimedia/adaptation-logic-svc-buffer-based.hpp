/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2015 Christian Kreuzberger and Daniel Posch, Alpen-Adria-University 
 * Klagenfurt
 *
 * This file is part of amus-ndnSIM, based on ndnSIM. See AUTHORS for complete list of 
 * authors and contributors.
 *
 * amus-ndnSIM and ndnSIM are free software: you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the Free Software 
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * amus-ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * amus-ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#ifndef ADAPTATIONLOGICSVCBUFFERBASED_HPP
#define ADAPTATIONLOGICSVCBUFFERBASED_HPP

#include "adaptation-logic.hpp"
#include <cmath>

#define BUFFER_MIN_SIZE 16 // in seconds
#define BUFFER_ALPHA 8 // in seconds

namespace dash
{
namespace player
{
class SVCBufferBasedAdaptationLogic : public AdaptationLogic
{
public:
  SVCBufferBasedAdaptationLogic(MultimediaPlayer* mPlayer) : AdaptationLogic (mPlayer)
  {
    alpha = BUFFER_ALPHA;
    gamma = BUFFER_MIN_SIZE;
  }

  virtual std::string GetName() const
  {
    return "dash::player::SVCBufferBasedAdaptationLogic";
  }

  static std::shared_ptr<AdaptationLogic> Create(MultimediaPlayer* mPlayer)
  {
    return std::make_shared<SVCBufferBasedAdaptationLogic>(mPlayer);
  }

  virtual void SetAvailableRepresentations(std::map<std::string, IRepresentation*>* availableRepresentations);

  virtual ISegmentURL*
  GetNextSegment(unsigned int* requested_segment_number, const dash::mpd::IRepresentation** usedRepresentation,  bool* hasDownloadedAllSegments);

  virtual bool hasMinBufferLevel(const dash::mpd::IRepresentation* rep);


protected:

  void orderRepresentationsByDepIds();
  unsigned int desired_buffer_size(int i, int i_curr);
  unsigned int getNextNeededSegmentNumber(int layer);

  static SVCBufferBasedAdaptationLogic _staticLogic;

  SVCBufferBasedAdaptationLogic()
  {
    ENSURE_ADAPTATION_LOGIC_REGISTERED(SVCBufferBasedAdaptationLogic);
  }

  std::map<int /*level*/, IRepresentation*> m_orderdByDepIdReps;

  double alpha;
  int gamma; //BUFFER_MIN_SIZE

  double segment_duration;

  enum AdaptationPhase
  {
    Steady = 0,
    Growing = 1,
    Upswitching = 2
  };

  AdaptationPhase lastPhase;
  AdaptationPhase allowedPhase;
};
}
}
#endif // ADAPTATIONLOGICSVCBUFFERBASED_HPP
