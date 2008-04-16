/* Scheduler.h
 * Simple cooperative multitasking.
 */

#pragma once

#define Scheduler_h

#include "egobootypedef.h"
#include "JF_Timer.h"
#include <list>

namespace JF
{
  class Task;

  class Scheduler : public GID<Scheduler>
  {
    public:
	    Scheduler();
	    ~Scheduler();

	    void addTask(Task *t, Task * parent=NULL);
	    int run();

      bool isEmpty() { return m_processes.empty(); };

      bool requestClose();

    private:
	    Timer m_timer;
	    std::list<Task*> m_processes;

	    void updateTasks(unsigned int timestamp);
	    void pruneTasks();
  };
};