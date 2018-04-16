#ifndef WY_COUNT_DOWN_LATCH_H
#define WY_COUNT_DOWN_LATCH_H

#include "noncopyable.h"
#include "mutex.h"
#include "condition.h"

namespace wynet
{
// 多线程离散倒计时器
// 给定初始count，每次调用countDown，count减1，为0时发送通知wait的线程
// Condition和CountDownLatch用的同一个mutex
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
