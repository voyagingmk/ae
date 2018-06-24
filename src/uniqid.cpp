#include "uniqid.h"
#include "logger/log.h"

namespace wynet
{

UniqIDGenerator::UniqIDGenerator() : count(0),
									 recycleThreshold(100000),
									 recycleEnabled(true){};

UniqIDGenerator::~UniqIDGenerator()
{
	recycled.clear();
}

UniqID UniqIDGenerator::getNewID()
{
	if (recycleEnabled && count > recycleThreshold)
	{
		if (recycled.size() > 0)
		{
			std::set<UniqID>::iterator it = recycled.begin();
			UniqID id = *it;
			recycled.erase(it);
			return id;
		}
	}
	count++;
	return count;
}
void UniqIDGenerator::recycleID(UniqID id)
{
	if (!recycleEnabled)
	{
		return;
	}
	if (id <= 0)
	{
		return;
	}
	if (recycled.find(id) != recycled.end())
	{
		log_fatal("recycleID repeated id = %d", id);
		return;
	}
	recycled.insert(id);
}

void UniqIDGenerator::popRecycleIDs(std::vector<UniqID> &vec, int num)
{
	if (!recycleEnabled)
	{
		return;
	}
	while (recycled.size() > 0 && num > 0)
	{
		num--;
		std::set<UniqID>::iterator it = recycled.begin();
		UniqID id = *it;
		vec.push_back(id);
		recycled.erase(it);
	}
}

}; // namespace wynet
