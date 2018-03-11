#include "exception.h"

#include <execinfo.h>
#include <stdlib.h>

using namespace wynet;
using namespace std;

Exception::Exception(const char *msg)
    : m_message(msg)
{
  fillStackTrace();
}

Exception::Exception(const string &msg)
    : m_message(msg)
{
  fillStackTrace();
}

Exception::~Exception() noexcept
{
}

const char *Exception::what() const noexcept
{
  return m_message.c_str();
}

const char *Exception::stackTrace() const noexcept
{
  return m_stack.c_str();
}

void Exception::fillStackTrace()
{
  const int len = 200;
  void *buffer[len];
  int nptrs = ::backtrace(buffer, len);
  char **strings = ::backtrace_symbols(buffer, nptrs);
  if (strings)
  {
    for (int i = 0; i < nptrs; ++i)
    {
      m_stack.append(strings[i]);
      m_stack.push_back('\n');
    }
    free(strings);
  }
}
