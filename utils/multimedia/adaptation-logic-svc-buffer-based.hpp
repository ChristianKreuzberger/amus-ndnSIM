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
