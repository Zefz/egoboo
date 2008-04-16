// Egoboo - Task.c

#include "JF_Task.h"
#include "JF_Scheduler.h"
#include <assert.h>

using namespace JF;

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

Task::Task(const Scheduler * s, const char *taskName, int freq, const Task * parent) :
_scheduler(s), _id(Scheduler::new_ID()), _parent(parent)
{
	m_state = PS_Active;
	m_lastRunTime = 0;
	setRunFrequency(freq);

  size_t len = strlen(taskName) + 1;
  _name = (char*)malloc(len);
  strncpy(_name, taskName, len);

  m_update_count = 0;
}

//---------------------------------------------------------------------------------------------
Task::~Task()
{
	assert(m_state == PS_Dead && "Task destroyed while still active!");

  cout << "---- destroying == " << _name;
  if(NULL!=_parent) cout << " parent == " << _parent;
  cout << endl;

  free(_name);
}

//---------------------------------------------------------------------------------------------
void Task::start(unsigned int currentTime)
{
  cout << "++++ starting == " << _name;
  if(NULL!=_parent) cout << " parent == " << _parent->_name;
  cout << endl;

	updateTime(currentTime);
}

//---------------------------------------------------------------------------------------------
void Task::pause()
{
  cout << "---- pausing == " << _name;
  if(NULL!=_parent) cout << " parent == " << _parent->_name;
  cout << endl;
	m_state = PS_Paused;
}

//---------------------------------------------------------------------------------------------
void Task::kill()
{
  cout << "---- killing == " << _name;
  if(NULL!=_parent) cout << " parent == " << _parent->_name;
  cout << endl;

	m_state = PS_Dead;
}

//---------------------------------------------------------------------------------------------
void Task::resume(unsigned int currentTime)
{
  cout << "++++ resuming == " << _name;
  if(NULL!=_parent) cout << " parent == " << _parent;
  cout << endl;

	m_state = PS_Active;
	updateTime(currentTime);
}

//---------------------------------------------------------------------------------------------
void Task::setRunFrequency(int updatesPerSecond)
{
	m_runFrequency = (updatesPerSecond > 0) ? updatesPerSecond : 0;
}

//---------------------------------------------------------------------------------------------
void Task::updateTime(unsigned int currentTime)
{
	m_lastRunTime = m_currentTime;
	m_currentTime = currentTime;

	m_nextRunTime = (m_runFrequency == 0) ? m_currentTime : currentTime + (1000 / m_runFrequency);
}

//---------------------------------------------------------------------------------------------
void Task::update(unsigned int currentTime)
{
  // if the state is active, try to execute it
  if (m_state == PS_Active)
  {
	  if (currentTime > m_nextRunTime || m_runFrequency == 0)
	  {
		  updateTime(currentTime);
      this->run( std::max<int>(1,m_currentTime - m_lastRunTime)/1000.0f );
      m_update_count++;

      //if(m_update_count == 1000)
      //{
      //  cout << "updating : " << _name;
      //  if(NULL!=_parent)
      //  {
      //    cout << " : parent " << _parent->_name;
      //  };
      //  cout << endl;
      //  m_update_count = 0;
      //};
	  }
  }


  // in any case, execute any non-paused sub-tasks
  m_scheduler.run();
}

//---------------------------------------------------------------------------------------------
void Task::spawn(Task * child)
{
  assert(NULL!=_scheduler);
  cout << "++++ spawning process == " << child->_name << " : parent == " << _name << endl;
  ((Scheduler *)_scheduler)->addTask(child, this);
}

//---------------------------------------------------------------------------------------------
void Task::spawn_subtask(Task * child)
{
  cout << "++++ spawning sub-process == " << child->_name << " : parent == " << _name << endl;
  m_scheduler.addTask(child, this);
}
