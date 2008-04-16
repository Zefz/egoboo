// Egoboo - Task.h
// Interface for the task-handling code

#pragma once

#define egoboo_Task_h
#include "JF_Scheduler.h"
#include "egobootypedef.h"

namespace JF
{
  class Scheduler;

  /**
  * An interface that represents a single Task in a Scheduler.  If you want to
  * create a process, derive from the class and implement the run method.  It'll be
  * called once per run through the process list (at the moment).  When you don't
  * want to run anymore, call this->kill()
  */

  class Task;

  struct ITask
  {
      virtual Task * getTask()        = 0;

    protected:
	    virtual void   run(float dTime) = 0;
  };

  class Task : public ITask
  {
    typedef GID<Scheduler>::ID ID;

    public:

      enum State
      {
        PS_Dead,
        PS_Active,
        PS_Paused,
      };

      Task(const Scheduler *, const char *taskName, int runFrequency = 1000, const Task * parent = NULL);
      virtual ~Task();

      virtual Task * getTask()         { return this; }
      const   Task * getParent() const { return _parent; }


      // State query/control methods
      Task::State getState() const { return m_state; }
      const ID    getID()    const { return _id; };
      void kill();
      void pause();
      void start(unsigned int currentTime);
      void resume(unsigned int currentTime);

      bool isPaused()  { return PS_Paused==m_state; }
      bool isRunning() { return PS_Active==m_state; }
      bool isDead()    { return PS_Dead==m_state; }

      const char * getName() { return _name; };
      const char * getParentName()
      {
        if(NULL==_parent) return "NULL";
        return _parent->_name;
      };

      void spawn(Task * child);
      void spawn_subtask(Task * child);

      Scheduler * GetSubScheduler() { return & m_scheduler; }

      // The process needs to know what scheduler is controlling it.  This allows one process
      // to schedule new ones.
      void setScheduler(const Scheduler *s) { _scheduler = s;    };
      const Scheduler * getScheduler()      { return _scheduler; };

      // Run frequency is specified in updates per second.  Setting it to
      // zero means run as fast as the scheduler does.
      void setRunFrequency(int updatesPerSecond);

      // Update the internal counters; this is done in enough places to be a separate function
      void updateTime(unsigned int currentTime);

      // Run through one iteration of the process.  This updates the timer values within
      // the process, and calls 'run' if it's time for the process to run
      void update(unsigned int currentTime);

      // let another process ask us to politely finish what we're doing
      virtual bool requestClose() { m_state = PS_Dead; return m_scheduler.requestClose(); }

    protected:
      State        m_state;
      unsigned int m_lastRunTime;
      unsigned int m_currentTime;
      unsigned int m_nextRunTime;
      int          m_runFrequency;

      int          m_update_count;


      // Must be implemented in a derived class.
      virtual void run(float dTime) = 0;

      void run_sub(float dTime)
      {
        m_scheduler.run();
      };

      const Scheduler * getSubScheduler()      { return &m_scheduler; };
      void  addSubTask(Task * w, Task * parent) { m_scheduler.addTask(w, parent);  }

    private:
      Scheduler       m_scheduler;     // each task can have sub-tasks

      const Scheduler *_scheduler;
      char            *_name;
      const ID         _id;
      const Task      *_parent;
  };
}