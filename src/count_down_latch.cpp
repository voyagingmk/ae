#include "count_down_latch.h"

using namespace wynet;

CountDownLatch::CountDownLatch(int count)
    : m_mutex(),
      m_condition(m_mutex),
      m_count(count)
{
}

void CountDownLatch::wait()
{
  MutexLockGuard<MutexLock> lock(m_mutex);
  while (m_count > 0)
  {
    m_condition.wait();
  }
}

void CountDownLatch::countDown()
{
  MutexLockGuard<MutexLock> lock(m_mutex);
  --m_count;
  if (m_count == 0)
  {
    m_condition.notifyAll();
  }
}

int CountDownLatch::getCount() const
{
  MutexLockGuard<MutexLock> lock(m_mutex);
  return m_count;
}
