#ifndef WY_UNIQID_H
#define WY_UNIQID_H

#include "common.h"

namespace wynet {

	typedef unsigned int UniqID;

	class UniqIDGenerator
	{
	public:
		UniqIDGenerator();
		~UniqIDGenerator();
		UniqID getNewID();
        void setRecycleThreshold(int threshold) {
            recycleThreshold = threshold;
        }
        void setRecycleEnabled(bool b) {
            recycleEnabled = b;
        }
        bool isRecycleEnabled()  {
            return recycleEnabled;
        }
		void recycleID(UniqID id);
		inline size_t getCount() const {
			return count;
		}
		inline size_t getRecycledLength() const {
			return recycled.size();
		}
    private:
        std::set<UniqID> recycled;
        UniqID count;
        int recycleThreshold;
        bool recycleEnabled;
	};
};

#endif
