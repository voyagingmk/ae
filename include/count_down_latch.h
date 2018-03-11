#ifndef WY_LATCH_H
#define WY_LATCH_H

#include "noncopyable.h"
#include "mutex.h"
#include "condition.h"

namespace wynet
{

class CountDownLatch : Noncopyable
{
 public:

  explicit CountDownLatch(int count);

  void wait();

  void countDown();

  int getCount() const;

 private:
  mutable MutexLock m_mutex;
  Condition m_condition;
  int m_count;
};

}
#endif
