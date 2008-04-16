#pragma once

#include <stdlib.h>
#include "JF_Task.h"

enum StateMachineStates
{
  SM_Unknown = -1,
  SM_Begin,
  SM_Entering,
  SM_Running,
  SM_Leaving,
  SM_Finish
};

struct StateMachine;
struct Widget;

struct IStateMachine
{
  virtual StateMachine  * get_Statemachine()  = 0;
  virtual Widget        * get_Widget()        = 0;

protected:
  virtual void Begin(float deltaTime)  = 0;
  virtual void Enter(float deltaTime)  = 0;
  virtual void Run(float deltaTime)    = 0;
  virtual void Leave(float deltaTime)  = 0;
  virtual void Finish(float deltaTime) = 0;
};

struct StateMachine : public IStateMachine, public JF::Task
{
  StateMachineStates  state;
  int                 result;

  virtual StateMachine  * get_Statemachine() { return this; };
  virtual Widget        * get_Widget()       { return NULL; };

  virtual void run(float deltaTime);

  virtual bool requestClose()
  {
    bool retval = false;

    if(SM_Unknown == state)
    {
      // only report closed if the scheduler is closed and
      // if we have exited the Finished() state.
      retval = GetSubScheduler()->requestClose();
    }
    else
    {
      // otherwise, just tell the StateMachine to exit gracefully
      state = SM_Finish;
    }

    return retval;
  }


protected:

  virtual void Begin(float deltaTime)  { result =  0; state = SM_Entering;};
  virtual void Enter(float deltaTime)  { result =  0; state = SM_Running; };
  virtual void Run(float deltaTime)    { result =  0; state = SM_Leaving; };
  virtual void Leave(float deltaTime)  { result =  0; state = SM_Finish;  };
  virtual void Finish(float deltaTime) { result = -1; state = SM_Unknown; };

  // making the constructor and destructor protected means that only classes that
  // inherit StateMachine can call new or delete to make a create or destroy an instance of this class
  // spawn and Unregister must be called instead
  StateMachine(const JF::Scheduler * s, const char * TaskName, const JF::Task *parent = NULL);
  StateMachine(const char * TaskName, const JF::Task *parent);
  virtual ~StateMachine();
};
