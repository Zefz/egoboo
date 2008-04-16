/* Scheduler.cpp
 */

#include "JF_Scheduler.h"
#include "JF_Task.h"

using namespace JF;

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define sleepFunc Sleep
#else
#include <SDL.h>
#define sleepFunc SDL_Delay
#endif

using namespace std;
using namespace JF;

Scheduler::Scheduler()
{
}

Scheduler::~Scheduler()
{
	while (!m_processes.empty())
	{
		Task *p = m_processes.front();
		delete p;
		m_processes.pop_front();
	}
}

void Scheduler::addTask(Task * t, Task * parent)
{
	if (!t) return;
	m_processes.push_front(t);
	t->setScheduler(this);
	t->start(m_timer.currentTime());

  if(NULL!=parent) parent->pause();
}

void Scheduler::updateTasks(unsigned int timestamp)
{
	list<Task*>::iterator i, end;
	i = m_processes.begin();
	end = m_processes.end();

	while (i != end)
	{
		(*i)->update(timestamp);
		++i;
	}
}

bool Scheduler::requestClose()
{
	list<Task*>::iterator i, end;

  // iterate through all the existing processes
  bool btmp, closed = true;
	for (i=m_processes.begin(); i!=m_processes.end(); i++)
	{
    btmp = (*i)->requestClose();
    closed = closed && btmp;
	}

  // return true, when every sub-process is reporting that it is closed
  // AND all processes have disconnected from us
  return closed && isEmpty();
}



void Scheduler::pruneTasks()
{
	list<Task*>::iterator i, end;
	i = m_processes.begin();
	end = m_processes.end();

	while (i != end)
	{
		if ( (*i)->isDead() )
		{
      Task * t = (*i);

      //restart any parent
      Task * p = const_cast<Task*>(t->getParent());
      if(NULL!=p && Task::PS_Paused==p->getState())
      {
        p->resume(m_timer.currentTime());
      }

      //get rid of this state
      if( t->requestClose() )
      {
        // actually get rid of the item only when all of the sub-tasks have finished
        delete t;
        i = m_processes.erase(i);
      }
      else
        i++;
		}
    else
		{
			++i;
		}
	}


}

int Scheduler::run()
{
	if (m_processes.empty()) return 0;

	m_timer.update();
	updateTasks(m_timer.currentTime());
	pruneTasks();

	// Make sure the operating system gets some CPU time.
	sleepFunc(0);

	return 1;
}
