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

#ifndef DASH_RATE_BASED_ADAPTATION_LOGIC
#define DASH_RATE_BASED_ADAPTATION_LOGIC

#include "adaptation-logic.hpp"
#include "multimedia-player.hpp"
#include <stack>

//#define RateBasedMinBufferLevel 20.0 //Seconds
#define RateBasedEMA_W 0.2

namespace dash
{
namespace player
{
class SVCRateBasedAdaptationLogic : public AdaptationLogic
{
public:
  SVCRateBasedAdaptationLogic(MultimediaPlayer* mPlayer) : AdaptationLogic (mPlayer)
  {
    ema_download_bitrate = m_multimediaPlayer->GetLastDownloadBitRate();
    curSegmentNumber = 0;
  }

  virtual std::string GetName() const
  {
    return "dash::player::SVCRateBasedAdaptationLogic";
  }

  static std::shared_ptr<AdaptationLogic> Create(MultimediaPlayer* mPlayer)
  {
    return std::make_shared<SVCRateBasedAdaptationLogic>(mPlayer);
  }

  virtual ISegmentURL*
  GetNextSegment(unsigned int* requested_segment_number, const dash::mpd::IRepresentation **usedRepresentation, bool* hasDownloadedAllSegments);

  virtual void SetAvailableRepresentations(std::map<std::string, IRepresentation*>* availableRepresentations);

  virtual bool hasMinBufferLevel(const dash::mpd::IRepresentation* rep);


protected:
  static SVCRateBasedAdaptationLogic _staticLogic;

  SVCRateBasedAdaptationLogic()
  {
    ENSURE_ADAPTATION_LOGIC_REGISTERED(SVCRateBasedAdaptationLogic);
  }

  void updateEMA();

  void orderRepresentationsByDepIds();
  bool hasMinBufferLevel();

  std::map<int /*level/layer*/, IRepresentation*> m_orderdByDepIdReps;

  //unsigned int getNextNeededSegmentNumber(int layer);
  unsigned int curSegmentNumber;

  double segment_duration;
  //double bufMinLevel;

  double ema_download_bitrate; /* moving average*/

  std::stack<dash::mpd::IRepresentation *> repsForCurSegment;

};
}
}


#endif // DASH_RATE_BASED_ADAPTATION_LOGIC
