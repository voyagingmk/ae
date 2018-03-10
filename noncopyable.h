#ifndef WY_NONCOPYABLE_H
#define WY_NONCOPYABLE_H

class Noncopyable
{
  protected:
    Noncopyable(const Noncopyable &) = delete;
    Noncopyable operator=(const Noncopyable &) = delete;
    Noncopyable() = default;
};

#endif
