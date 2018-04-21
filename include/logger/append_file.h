#ifndef WY_APPEND_FILE_H
#define WY_APPEND_FILE_H

#include "../common.h"
#include "../mutex.h"
#include "../noncopyable.h"

namespace wynet
{
using namespace std;

// 纯粹的磁盘文件IO
class AppendFile : Noncopyable
{
public:
  explicit AppendFile(std::string &filename);

  ~AppendFile();

  void append(const char *logline, const size_t len);

  void flush();

  off_t writtenBytes() const { return m_writtenBytes; }

private:
  size_t write(const char *logline, size_t len);

  FILE *m_fp;
  off_t m_writtenBytes;
};
};

#endif
