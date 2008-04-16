#include "StateMachine.h"

StateMachine * SM_List = NULL;

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

StateMachine::StateMachine(const JF::Scheduler * s, const char * TaskName, const JF::Task *parent) :
  Task(s, TaskName, 0, parent)
{
  state   = SM_Begin;
  result  = 0;
  //child   = NULL;
  //sibling = NULL;
};

StateMachine::StateMachine(const char * TaskName, const JF::Task *parent) :
Task( ((JF::Task*)parent)->getScheduler(), TaskName, 0, parent)
{
  state   = SM_Begin;
  result  = 0;
  //child   = NULL;
  //sibling = NULL;
};

//---------------------------------------------------------------------------------------------
StateMachine::~StateMachine()
{
  //delete all of the children of this list

  //StateMachine * machine = child;
  //while (NULL!=machine)
  //{
  //  child = machine->sibling;
  //  delete machine;

  //  machine = child;
  //}

  //sibling = NULL;
  //child = NULL;
};

//---------------------------------------------------------------------------------------------
void StateMachine::run(float deltaTime)
{
  result = 0;


  switch (state)
  {
    case SM_Begin:    Begin(deltaTime);
      cout << "++++ Begin() == " << getName() << " parent == " << getParentName() << endl;
      break;
    case SM_Entering: Enter(deltaTime);  break;
    case SM_Running:  Run(deltaTime);    break;
    case SM_Leaving:  Leave(deltaTime);  break;
    case SM_Finish:   Finish(deltaTime);
      cout << "---- Finish() == " << getName() << " parent == " << getParentName() << endl;
      break;

    case SM_Unknown:
      cout << "---- State Unknown == " << getName() << " parent == " << getParentName()
           << " task state == ";
      switch( getState() )
      {
        case PS_Active: cout << "PS_Active"; break;
        case PS_Paused: cout << "PS_Paused"; break;
        case PS_Dead:   cout << "PS_Dead";   break;
      };
      cout << endl;
    default:          kill();            break;
  };
};

//---------------------------------------------------------------------------------------------
//IStateMachine * StateMachine::run(IStateMachine * list, float deltaTime)
//{
//  if (NULL==list) return NULL;
//
//  // run() this machine. Paused machine will return itself.
//  IStateMachine * tmp = list->run(deltaTime);
//
//  //if the run() function returns a different object, operate on that object
//  if (tmp != list) return tmp;
//
//  //if it does not return a new object, then return the next sibling
//  return list->get_Statemachine()->sibling;
//};

//---------------------------------------------------------------------------------------------
//void StateMachine::AddMachineFront(StateMachine * &list, StateMachine *machine)
//{
//  // insert the machine at the front
//
//  if (NULL!=machine)
//  {
//    machine->sibling = list;
//    list = machine;
//  };
//}

//---------------------------------------------------------------------------------------------
//void StateMachine::AddMachineBack(StateMachine *&list, StateMachine *machine)
//{
//  if (NULL==machine) return;
//
//  // append the machine at the end of list
//
//  if (NULL==list)
//  {
//    list = machine;
//    machine->sibling = NULL;
//  }
//  else
//  {
//    //find the last machine in the list
//    StateMachine * tmp_machine = list;
//    while (NULL!=tmp_machine->sibling)
//      tmp_machine = tmp_machine->sibling;
//
//    tmp_machine->sibling = machine;
//    machine->sibling = NULL;
//  }
//
//}


//--------------------------------------------------------------------------------------------
//StateMachine * StateMachine::PushMachine(StateMachine * &list, StateMachine * machine)
//{
//  if (NULL==machine) return this;
//
//  //turn off this machine
//  Suspend();
//
//  //set the last machine to be us
//  machine->_parent = this;
//
//  //move us off the list and insert machine
//  Swap(list, this, machine);
//
//  return machine;
//};


//--------------------------------------------------------------------------------------------
//StateMachine * StateMachine::PopMachine(StateMachine * &list)
//{
//  if (NULL!=_parent)
//  {
//    _parent->Resume();
//
//    // move us off the list and insert _parent
//    Swap(list, this, _parent);
//  };
//
//  // delete ourselves. say bye!
//  if (NULL!=this) { Suspend(); delete this; }
//
//  //return the restored machine
//  return _parent;
//}

//---------------------------------------------------------------------------------------------
//void StateMachine::Swap(StateMachine * &list, StateMachine *machine1, StateMachine *machine2)
//{
//  if (NULL==list || NULL==machine1 || NULL==machine2) return;
//
//  //check to see whether machine2 is in the list
//  StateMachine * tmp = list;
//  bool found2 = false;
//  while (NULL!=tmp)
//  {
//    if (tmp->sibling == machine2) {found2 = true; break;}
//    tmp = tmp->sibling;
//  }
//
//  // if they are both in the list, do nothing
//  // Suspend() will make the run() function skip over the machine
//  if (found2) return;
//
//  if (list == machine1)
//  {
//    list     = machine2;
//    machine2->sibling = machine1->sibling;
//    machine1->sibling = NULL;
//  }
//  else
//  {
//    StateMachine * tmp = list;
//
//    while (NULL!=tmp)
//    {
//      if (tmp->sibling == machine1)
//      {
//        tmp->sibling = machine2;
//        machine2->sibling = machine1->sibling;
//        machine1->sibling = NULL;
//        return;
//      }
//      tmp = tmp->sibling;
//    }
//  };
//
//
//}

//
////---------------------------------------------------------------------------------------------
//Task::ID StateMachine::spawn(StateMachine *machine, StateMachine *parent)
//{
//  //error trap
//  if(NULL==machine) return BAD_ID;
//
//  //add it to the task list
//  Add(machine);
//
//  //if the parent exists, pause it
//  if(NULL != parent)
//    parent->Pause();
//
//  return machine->get_ID();
//};
//
////---------------------------------------------------------------------------------------------
//StateMachine * StateMachine::Unregister(StateMachine *machine)
//{
//  if(NULL == machine) return NULL;
//
//  // unlink the machine from the task list
//  Remove(machine);
//
//  //if the parent exists, un-pause it
//  if(NULL != machine->get_parent())
//    machine->get_parent()->Play();
//
//  return machine->get_parent();
//};