#ifndef ADAPTATIONLOGICSVCBUFFERBASED_HPP
#define ADAPTATIONLOGICSVCBUFFERBASED_HPP

#include "adaptation-logic.hpp"

namespace dash
{
namespace player
{
class SVCBufferBasedAdaptationLogic : public AdaptationLogic
{
public:
  SVCBufferBasedAdaptationLogic(MultimediaPlayer* mPlayer) : AdaptationLogic (mPlayer)
  {
    currentSegmentNumber=0;
  }

  virtual std::string GetName() const
  {
    return "dash::player::SVCBufferBasedAdaptationLogic";
  }

  static std::shared_ptr<AdaptationLogic> Create(MultimediaPlayer* mPlayer)
  {
    return std::make_shared<SVCBufferBasedAdaptationLogic>(mPlayer);
  }

  virtual ISegmentURL*
  GetNextSegment(unsigned int* requested_segment_number, const dash::mpd::IRepresentation** usedRepresentation);


protected:
  static SVCBufferBasedAdaptationLogic _staticLogic;

  SVCBufferBasedAdaptationLogic()
  {
    ENSURE_ADAPTATION_LOGIC_REGISTERED(SVCBufferBasedAdaptationLogic);
  }

  unsigned int currentSegmentNumber;

};
}
}
#endif // ADAPTATIONLOGICSVCBUFFERBASED_HPP
