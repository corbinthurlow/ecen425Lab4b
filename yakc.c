#include "clib.h"
#include "yaku.h"
#include "yakk.h"


int IdleTaskStk[IDLE_TASK_STACK_SIZE];	// Setting the size of the stack for the idle task
TCBptr YKCurrTask;						// TCB pointer to the current task

// Function Prototypes
void YKIdleTask(void); 			// Idle task, always ready
void insertReady(TCBptr tmp);	// Insert a TCB to the ready list
void removeReady(void);			// Remove the current ready TCB
void YKDispatcherASM();			// Dispatch function written in assembly called from here
void* ssTemp;					// Temp global to store ss to be able to push and pop it
// Needed global variables
unsigned int YKCtxSwCount;    	// Variable for tracking context switches
unsigned int YKTickNum;       	// Variable incremented by tick handler
unsigned int YKNestingLevel;  	// Variable tracking nesting level
unsigned int YKKernelStarted; 	// Variable indicating kernel has started
unsigned int YKIdleCount;		// Idle count variable
// TCB and linkedlist declarations
TCBptr YKRdyList;  		// a list of TCBs of all ready tasks in order of decreasing priority
TCBptr YKBlockList; 		//tasks delayed or suspended
TCBptr YKFreeTCBList;	//a list of available TCBs
TCB YKTCBArray[MAX_TASKS+1]; //array to allocate all needed TCBs including idle task
int saveContext = 0;		//value used to determine whether or not context needs saving
TCBptr oldTask;				//pointer used for keeping track of the old task

//initializes the global variables needed to run the RTOS
//and adds IdleTask to queue
void YKInitialize(void){

	int i;

	YKEnterMutex();		//disable interrupts 
	
	YKFreeTCBList = &(YKTCBArray[0]);	//set the free tcb list to first address in allocated space
	for (i = 0; i < MAX_TASKS; i++)		
		YKTCBArray[i].next = &(YKTCBArray[i+1]);
	YKTCBArray[MAX_TASKS].next = NULL; 		//last task is idle task
	

	YKCtxSwCount = 0;	//init switch count to zero
	YKIdleCount = 0;	//init idlecount to zero
	YKTickNum = 0;		//init tick number to zero
	YKNestingLevel = 0;	//init nesting level to 0
	YKKernelStarted = 0;	//variable used to tell of kernel has started

	
	YKRdyList = NULL; //ready list inited to null
	YKBlockList = NULL;	//block list inited to null
	YKCurrTask = NULL;	//currtask inited to null

	YKNewTask(YKIdleTask, (void*)&IdleTaskStk[IDLE_TASK_STACK_SIZE], LOWEST_PRIORITY);	//created the idle task

	YKExitMutex();	//re-enables interrupts


}

//creates a new task. 
//init task data
void YKNewTask(void (*task)(void), void *taskStack, unsigned int priority){
	TCBptr newTaskPtr = YKFreeTCBList;
	
	
	YKEnterMutex(); //disable interrupts when creating a new task

	//set initial register values
	*((int*)taskStack-1) = 0x0200; //set flags
	*((int*)taskStack-2) = 0;		 		// set space for CS
	*((int*)taskStack-3) = (int*)task;		//set space for IP
	*((int*)taskStack-4) = &taskStack;		//set space for bp
	*((int*)taskStack-5) = 0;				//set space for DX
	*((int*)taskStack-6) = 0;				//set space for CX
	*((int*)taskStack-7) = 0;				//set space for BX
	*((int*)taskStack-8) = 0;				//set space for AX
	*((int*)taskStack-9) = 0;				//set space for ES
	*((int*)taskStack-10) = 0;				//set space for DI
	*((int*)taskStack-11) = 0;				//set space for SI
	*((int*)taskStack-12) = 0;				//set space for DS
	*((int*)taskStack-13) = 0;				//set space for SS

	//allocate a spot in the task array

	YKFreeTCBList->stackptr = ((int*)taskStack-13);
	YKFreeTCBList->state = NEW;
	YKFreeTCBList->priority = priority;
	YKFreeTCBList->delay = 0;

	//remove this TCB form AvailTCBList
	newTaskPtr = YKFreeTCBList;
    YKFreeTCBList = newTaskPtr->next;

	//
	insertReady(newTaskPtr);

	if (YKKernelStarted == 1) {
		YKScheduler();
	}

	YKExitMutex();

}


//delay task
void YKDelayTask(unsigned int count){
	//int i;
	//for(i = 0; i < 1000*count; i++){
	//	printString("Delaying task...\n");	
	//} 
	if(count == 0){
		return;	
	}
	removeReady();


}




//YAK's idle task. this task does not do much, but spins in a loop and counts.
// The while loop is exactly 4 lines of of instructions. The YKIdleCount = YKIdleCount
// takes two instructions. This is fine because YKIdleTask is the only function to 
// modify YKIdleCount (unless CPU resets it to 0, then there will be a problem

void YKRun(void) {
	YKKernelStarted = 1;
	YKScheduler();
}

void YKScheduler(void) {	
	if (YKCurrTask != NULL && YKRdyList->priority == YKCurrTask->priority) {
		//do nothing, because we're executing highest priority already
		return;
	}

	//check interrupt nesting level, if greater than 0, don't save context
	if(YKNestingLevel == 0 && YKCurrTask != NULL){
		saveContext = 1;
	} else {
		saveContext = 0;
	}

	YKCtxSwCount++;
	//assign the first task in the list to YKCurrTask
	oldTask = YKCurrTask;
	YKCurrTask = YKRdyList;
	
	//assign new status to task
	if(oldTask != NULL) 
		oldTask->state = READY;
	
	YKCurrTask->state = RUNNING;
	
	YKDispatcherASM();
}


/*
 * code to insert an entry in doubly linked ready list sorted by
 * priority numbers (lowest number first).  tmp points to TCB
 * to be inserted 
 */ 
void insertReady(TCBptr tmp) {
	TCBptr tmp2;

	tmp->state = READY;

    if (YKRdyList == NULL) { /* is this first insertion? */
		YKRdyList = tmp;
		YKRdyList->next = NULL;
		YKRdyList->prev = NULL;
    } else { /* not first insertion */
		tmp2 = YKRdyList;	/* insert in sorted ready list */
		while (tmp2->priority < tmp->priority)
			tmp2 = tmp2->next;	/* assumes idle task is at end */
		if (tmp2->prev == NULL)	/* insert in list before tmp2 */
			YKRdyList = tmp;
		else
			tmp2->prev->next = tmp;
		tmp->prev = tmp2->prev;
		tmp->next = tmp2;
		tmp2->prev = tmp;
    }
}

/*
 * code to remove an entry from the ready list and put in
 * suspended list, which is not sorted.  (This only works for the
 * current task, so the TCB of the task to be suspended is assumed
 * to be the first entry in the ready list.)
 */
void removeReady(void){
	TCBptr tmp2,tmp; //set up tmp TCBptrs
	tmp = YKRdyList;	//set tmp tp the ready TCB
	tmp->state = BLOCKED; //set the tmp state to Blocked

	YKRdyList = tmp->next;	//then set tmp next to be ready 
	tmp->next->prev = NULL;	//set tmp's next prev to null
	tmp->next = YKBlockList;
	YKBlockList = tmp;
	tmp->prev = NULL;
	if(tmp->next != NULL){
		tmp->next->prev = tmp;
	}


}
