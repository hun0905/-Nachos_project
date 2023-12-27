// alarm.cc
//	Routines to use a hardware timer device to provide a
//	software alarm clock.  For now, we just provide time-slicing.
//
//	Not completely implemented.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "alarm.h"
#include "main.h"

//----------------------------------------------------------------------
// Alarm::Alarm
//      Initialize a software alarm clock.  Start up a timer device
//
//      "doRandom" -- if true, arrange for the hardware interrupts to 
//		occur at random, instead of fixed, intervals.
//----------------------------------------------------------------------

Alarm::Alarm(bool doRandom)
{
    timer = new Timer(doRandom, this);
    
    SleepThreads = new SortedList<SleepThread *>(Compare); //added
}

//----------------------------------------------------------------------
// Alarm::CallBack
//	Software interrupt handler for the timer device. The timer device is
//	set up to interrupt the CPU periodically (once every TimerTicks).
//	This routine is called each time there is a timer interrupt,
//	with interrupts disabled.
//
//	Note that instead of calling Yield() directly (which would
//	suspend the interrupt handler, not the interrupted thread
//	which is what we wanted to context switch), we set a flag
//	so that once the interrupt handler is done, it will appear as 
//	if the interrupted thread called Yield at the point it is 
//	was interrupted.
//
//	For now, just provide time-slicing.  Only need to time slice 
//      if we're currently running something (in other words, not idle).
//	Also, to keep from looping forever, we check if there's
//	nothing on the ready list, and there are no other pending
//	interrupts.  In this case, we can safely halt.
//----------------------------------------------------------------------
void 
Alarm::PushSleep(Thread *T, int x){
    
    SleepThread *sthread = new SleepThread(T, x);
    SleepThreads->Insert(sthread);
    T->Sleep(false); 
}
void
Alarm::WaitUntil(int x){
    //close the interrupt
    IntStatus oldL = kernel->interrupt->SetLevel(IntOff);
    Thread* T = kernel->currentThread;
    int wake = kernel->stats->totalTicks + x;
    PushSleep(T,wake);
    kernel->interrupt->SetLevel(oldL);
}
void 
Alarm::CallBack() 
{

    Interrupt *interrupt = kernel->interrupt;
    MachineStatus status = interrupt->getStatus();
    bool wake = PushReady();
    if (status == IdleMode && wake==false && SleepThreads->IsEmpty()) {	
        if (!interrupt->AnyFutureInterrupts()) {
	    timer->Disable();	
	}
    } else {			
	interrupt->YieldOnReturn();
    }
}

bool
Alarm::PushReady(){
    bool wake = false;
    ListIterator<SleepThread *> iter(SleepThreads);
    while(!iter.IsDone())
    {
        SleepThread * thread = iter.Item();
        if (thread->when <= kernel->stats->totalTicks)
        {
           wake = true;
           cout << "wake up." << endl;
           kernel->scheduler->ReadyToRun(thread->sleep_thread);
           iter.Next();
           SleepThreads->Remove(thread);
        }
        else { break; } 
    }
    return wake;
}

int 
Alarm::Compare(SleepThread *a, SleepThread *b){
    if (a->when < b->when) { return -1; }
    else if (a->when > b->when) { return 1; }
    else { return 0; }
}


Alarm::SleepThread::SleepThread(Thread *t, int x)
{
   sleep_thread= t;
   when = x;
}
Alarm::~Alarm()
{
   delete timer;
   while (!SleepThreads->IsEmpty()) {
        delete SleepThreads->RemoveFront();
    }
    delete SleepThreads;
}