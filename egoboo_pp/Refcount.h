/* Tactics - RefCount.h
* Simple reference counting mix-in template.
*/

#pragma once

#include <assert.h>

namespace JF
{
  template <class t>
  class RefCount
  {
    private:
      int m_refCount;

    public:
      RefCount()
      {
        m_refCount = 1;
      }

      virtual ~RefCount()
      {
        assert(m_refCount == 0);
      }

      t* retain()
      {
        m_refCount++;
        return (t*)this;
      }

      int retainCount() const
      {
        return m_refCount;
      }

      void release()
      {
        m_refCount--;
        if (m_refCount == 0)
        {
          delete this;
        }
      }
  };

};
