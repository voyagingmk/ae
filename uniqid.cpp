#include "uniqid.h"

namespace wynet {

	UniqIDGenerator::UniqIDGenerator() :
		count(0),
        recycleThreshold(100000),
        recycleEnabled(false)
	{
	};

	UniqIDGenerator::~UniqIDGenerator() {
		recycled.clear();
	}

	UniqID UniqIDGenerator::getNewID()  {
		if (recycleEnabled && count > recycleThreshold) {
			if (recycled.size() > 0) {
                std::set<UniqID>::iterator it = recycled.begin();
				UniqID id = *it;
                recycled.erase(it);
				return id;
			}
		}
		count++;
		return count;
	}
	void UniqIDGenerator::recycleID(UniqID id)  {
        if(!recycleEnabled) {
            return;
        }
		recycled.insert(id);
	}
};
