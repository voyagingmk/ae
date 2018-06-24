#ifndef WY_UNIQID_H
#define WY_UNIQID_H

#include "common.h"
#include "noncopyable.h"

namespace wynet
{

using UniqID = int32_t;

class UniqIDGenerator : public Noncopyable
{
  public:
	UniqIDGenerator();

	~UniqIDGenerator();

	UniqID getNewID();

	void setRecycleThreshold(int threshold)
	{
		recycleThreshold = threshold;
	}

	void setRecycleEnabled(bool b)
	{
		recycleEnabled = b;
	}

	bool isRecycleEnabled()
	{
		return recycleEnabled;
	}

	void recycleID(UniqID id);

	inline size_t getCount() const
	{
		return count;
	}

	inline size_t getRecycledLength() const
	{
		return recycled.size();
	}

	void popRecycleIDs(std::vector<UniqID> &vec, int num);

  private:
	std::set<UniqID> recycled;
	UniqID count;
	int recycleThreshold;
	bool recycleEnabled;
};
}; // namespace wynet

#endif
