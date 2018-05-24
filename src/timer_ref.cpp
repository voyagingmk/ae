#include "timer_ref.h"

namespace wynet
{
std::atomic<WyTimerId> TimerRef::g_numCreated;
};