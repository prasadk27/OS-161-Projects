/* 
 * stoplight.c
 *
 * 31-1-2003 : GWA : Stub functions created for CS161 Asst1.
 *
 * NB: You can use any synchronization primitives available to solve
 * the stoplight problem in this file.
 */


/*
 * 
 * Includes
 *
 */

#include <types.h>
#include <lib.h>
#include <test.h>
#include <thread.h>
#include <synch.h>
#include <queue.h>
#include <curthread.h>
#include <machine/spl.h>

// Declaring lock

struct lock *lock0;	// lock for NW
struct lock *lock1;	// lock for NE
struct lock *lock2;	// lock for SE
struct lock *lock3;	// lock for SW
struct lock *temp;	// lock for enforcing limit

// Declaring queues

struct queue *queue0;	// queue for lock0
struct queue *queue1;	// queue for lock1
struct queue *queue2;	// queue for lock2
struct queue *queue3;	// queue for lock3
struct queue *queue4;	// queue to enforce limit

/*
 *
 * Constants
 *
 */
// indicators
int NW = 0;
int NE = 0;
int SE = 0;
int SW = 0;
int limit = 0;
int NWcount=0, SWcount=0, NEcount=0, SEcount=0,count=0;

/*
 * Number of cars created.
 */

#define NCARS 20


/*
 *
 * Function Definitions
 *
 */


/*
 * gostraight()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement passing straight through the
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
gostraight(unsigned long cardirection,
           unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */
		int spl;

		if (cardirection == 0)
		{
			//Print message
			kprintf("Car %lu: Approaching NORTH, heading towards SOUTH\n",carnumber);
			lock_acquire(lock0);	// Try to acquire lock0
			// If lock0 is arleady acquired add thread to the queue
			while (NW == 1)									
			{
				assert(lock0 != NULL);
				assert(lock_do_i_hold(lock0));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock0);
				NWcount++;
				q_preallocate(queue0, NWcount);
				q_addtail(queue0, curthread);
				thread_sleep(curthread);
				lock_acquire(lock0);
				splx(spl);

			}
			NW = 1;
			// Print message
			kprintf("Car %lu: Entered NW from NORTH, heading towards SW\n",carnumber);
			NW = 0;

			lock_acquire(lock3);
			while (SW == 1) 
			{ 
				assert(lock3 != NULL);
				assert(lock_do_i_hold(lock3));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock3);
				SWcount++;
				q_preallocate(queue3, SWcount);
				q_addtail(queue3, curthread);
				thread_sleep(curthread);
				lock_acquire(lock3);
				splx(spl);
			}
			
			SW = 1;
			// Print message
			kprintf("Car %lu: Entered SW from NW, heading towards SOUTH\n",carnumber);
			SW = 0;

			//call signals
			if (NWcount != 0)
			{
				assert(lock0 != NULL);
				assert(lock_do_i_hold(lock0));
				spl = splhigh();
				NWcount--;
				struct thread *next_thread = q_remhead(queue0);
				thread_wakeup(next_thread);
				splx(spl);
			}
			lock_release(lock0);
			if (SWcount != 0)
			{
				assert(lock3 != NULL);
				assert(lock_do_i_hold(lock3));
				spl = splhigh();
				SWcount--;
				struct thread *next_thread = q_remhead(queue3);
				thread_wakeup(next_thread);
				splx(spl);
			}

			// Print message
			kprintf("Car %lu: Reached SOUTH\n",carnumber);
			//unlock locks

			lock_release(lock3);
		}

		else if (cardirection == 1)
		{
			//Print message
			kprintf("Car %lu: Approaching EAST, heading towards WEST\n", carnumber);
			lock_acquire(lock1);
			while (NE == 1)
			{
				assert(lock1 != NULL);
				assert(lock_do_i_hold(lock1));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock1);
				NEcount++;
				q_preallocate(queue1, NEcount);
				q_addtail(queue1, curthread);
				thread_sleep(curthread);
				lock_acquire(lock1);
				splx(spl);

			}
			NE = 1;
			// Print message
			kprintf("Car %lu: Entered NE from EAST, heading towards NW\n",carnumber);
			NE = 0;
			

			lock_acquire(lock0);
			while (NW == 1)
			{
				assert(lock0 != NULL);
				assert(lock_do_i_hold(lock0));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock0);
				NWcount++;
				q_preallocate(queue0, NWcount);
				q_addtail(queue0, curthread);
				thread_sleep(curthread);
				lock_acquire(lock0);
				splx(spl);
			}

			NW = 1;
			// Print message
			kprintf("Car %lu: Entering NW from NE, heading towards WEST\n", carnumber);
			NW = 0;

		
			if (NEcount != 0)
			{
				assert(lock1 != NULL);
				assert(lock_do_i_hold(lock1));
				spl = splhigh();
				NEcount--;
				struct thread *next_thread = q_remhead(queue1);
				thread_wakeup(next_thread);
				splx(spl);
			}
			lock_release(lock1);
			if (NWcount != 0)
			{
				assert(lock0 != NULL);
				assert(lock_do_i_hold(lock0));
				spl = splhigh();
				NWcount--;
				struct thread *next_thread = q_remhead(queue0);
				thread_wakeup(next_thread);
				splx(spl);
			}

			// Print message
			kprintf("Car %lu: Reached WEST from EAST\n", carnumber);
			//unlock locks

			lock_release(lock0);
		}
		else if (cardirection == 2)
		{
			//Print message
			kprintf("Car %lu: Approaching SOUTH, heading towards NORTH\n", carnumber);
			lock_acquire(lock2);
			while (SE == 1)
			{
				assert(lock2 != NULL);
				assert(lock_do_i_hold(lock2));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock2);
				SEcount++;
				q_preallocate(queue2, SEcount);
				q_addtail(queue2, curthread);
				thread_sleep(curthread);
				lock_acquire(lock2);
				splx(spl);

			}
			SE = 1;
			// Print message
			kprintf("Car %lu: Entered SE from EAST, heading towards NE\n", carnumber);
			SE = 0;
		

			lock_acquire(lock1);
			while (NE == 1)
			{
				assert(lock1 != NULL);
				assert(lock_do_i_hold(lock1));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock1);
				NEcount++;
				q_preallocate(queue1, NEcount);
				q_addtail(queue1, curthread);
				thread_sleep(curthread);
				lock_acquire(lock1);
				splx(spl);
			}

			NE = 1;
			// Print message
			kprintf("Car %lu: Entering NE from SE, heading towards NORTH\n", carnumber);
			NE = 0;

			//call signals
			if (SEcount != 0)
			{
				assert(lock2 != NULL);
				assert(lock_do_i_hold(lock2));
				spl = splhigh();
				SEcount--;
				struct thread *next_thread = q_remhead(queue2);
				thread_wakeup(next_thread);
				splx(spl);
			}
			lock_release(lock2);

			if (NEcount != 0)
			{
				assert(lock1 != NULL);
				assert(lock_do_i_hold(lock1));
				spl = splhigh();
				NEcount--;
				struct thread *next_thread = q_remhead(queue1);
				thread_wakeup(next_thread);
				splx(spl);
			}

			// Print message
			kprintf("Car %lu: Reached NORTH from SOUTH\n", carnumber);
			//unlock locks

			lock_release(lock1);
		}

		else if (cardirection == 3)
		{
			//Print message
			kprintf("Car %lu: Approaching WEST, heading towards EAST\n", carnumber);
			lock_acquire(lock3);
			while (SW == 1)
			{
				assert(lock3 != NULL);
				assert(lock_do_i_hold(lock3));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock3);
				SWcount++;
				q_preallocate(queue3, SWcount);
				q_addtail(queue3, curthread);
				thread_sleep(curthread);
				lock_acquire(lock3);
				splx(spl);

			}
			SW = 1;
			// Print message
			kprintf("Car %lu: Reached SW from WEST, heading towards SE\n", carnumber);
			SW = 0;

			lock_acquire(lock2);
			while (SE == 1)
			{
				assert(lock2 != NULL);
				assert(lock_do_i_hold(lock2));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock2);
				SEcount++;
				q_preallocate(queue2, SEcount);
				q_addtail(queue2, curthread);
				thread_sleep(curthread);
				lock_acquire(lock2);
				splx(spl);
			}

			SE = 1;
			// Print message
			kprintf("Car %lu: Reached SE from SW, heading towards EAST\n", carnumber);
			SE = 0;


			if (SWcount != 0)
			{
				assert(lock3 != NULL);
				assert(lock_do_i_hold(lock3));
				spl = splhigh();
				SWcount--;
				struct thread *next_thread = q_remhead(queue3);
				thread_wakeup(next_thread);
				splx(spl);
			}
			lock_release(lock3);
			if (SEcount != 0)
			{
				assert(lock2 != NULL);
				assert(lock_do_i_hold(lock2));
				spl = splhigh();
				SEcount--;
				struct thread *next_thread = q_remhead(queue2);
				thread_wakeup(next_thread);
				splx(spl);
			}

			// Print message
			kprintf("Car %lu: Reached EAST from WEST\n", carnumber);
			//unlock locks

			lock_release(lock2);
		}


}


/*
 * turnleft()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a left turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnleft(unsigned long cardirection,
         unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */

      
		int spl;
		if (cardirection == 0)
		{
			// Print message
			kprintf("Car %lu: Approaching NORTH, heading towards EAST\n", carnumber);
			lock_acquire(lock0);
			while (NW == 1)
			{
				assert(lock0 != NULL);
				assert(lock_do_i_hold(lock0));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock0);
				NWcount++;
				q_preallocate(queue0, NWcount);
				q_addtail(queue0, curthread);
				thread_sleep(curthread);
				lock_acquire(lock0);
				splx(spl);
			}
			NW = 1;
			// Print message
			kprintf("Car %lu: Reached NW from NORTH, heading towards SW\n", carnumber);
			NW = 0;

			lock_acquire(lock3);
			while (SW == 1) 
			{
				assert(lock3 != NULL);
				assert(lock_do_i_hold(lock3));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock3);
				SWcount++;
				q_preallocate(queue3, SWcount);
				q_addtail(queue3, curthread);
				thread_sleep(curthread);
				lock_acquire(lock3);
				splx(spl);
			}
			SW = 1;
			// Print message
			kprintf("Car %lu: Reached SW from NW, heading towards SE\n", carnumber);
			SW = 0;
			if (NWcount != 0)
			{
				assert(lock0 != NULL);
				assert(lock_do_i_hold(lock0));
				spl = splhigh();
				NWcount--;
				struct thread *next_thread = q_remhead(queue0);
				thread_wakeup(next_thread);
				splx(spl);
			}
			lock_release(lock0);


			
			lock_acquire(lock2);
			while (SE == 1) 
			{
				assert(lock2 != NULL);
				assert(lock_do_i_hold(lock2));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock2);
				SEcount++;
				q_preallocate(queue2, SEcount);
				q_addtail(queue2, curthread);
				thread_sleep(curthread);
				lock_acquire(lock2);
				splx(spl);
			}

			SE = 1;
			// Print message
			kprintf("Car %lu: Reached SE from SW, heading towards EAST\n", carnumber);
			SE = 0;
			if (SWcount != 0)
			{
				assert(lock3 != NULL);
				assert(lock_do_i_hold(lock3));
				spl = splhigh();
				SWcount--;
				struct thread *next_thread = q_remhead(queue3);
				thread_wakeup(next_thread);
				splx(spl);
			}
			lock_release(lock3);
			if (SEcount != 0) 
			{
				assert(lock2 != NULL);
				assert(lock_do_i_hold(lock2));
				spl = splhigh();
				SEcount--;
				struct thread *next_thread = q_remhead(queue2);
				thread_wakeup(next_thread);
				splx(spl);
			}
			kprintf("Car %lu: Reached EAST from NORTH\n", carnumber);
			
			lock_release(lock2);

		}

		else if (cardirection == 1)
		{
			// Print message
			kprintf("Car %lu: Approaching EAST, heading towards SOUTH\n", carnumber);
			lock_acquire(lock1);
			while (NE == 1)
			{
				assert(lock1 != NULL);
				assert(lock_do_i_hold(lock1));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock1);
				NEcount++;
				q_preallocate(queue1, NEcount);
				q_addtail(queue1, curthread);
				thread_sleep(curthread);
				lock_acquire(lock1);
				splx(spl);
			}
			NE = 1;
			// Print message
			kprintf("Car %lu: Reached NE from EAST, heading towards NW\n", carnumber);
			NE = 0;

			lock_acquire(lock0);
			while (NW == 1)
			{
				assert(lock0 != NULL);
				assert(lock_do_i_hold(lock0));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock0);
				NWcount++;
				q_preallocate(queue0, NWcount);
				q_addtail(queue0, curthread);
				thread_sleep(curthread);
				lock_acquire(lock0);
				splx(spl);
			}
			NW = 1;
			// Print message
			kprintf("Car %lu: Reached NW from NE, heading towards SW\n", carnumber);
			NW = 0;
			if (NEcount != 0)
			{
				assert(lock1 != NULL);
				assert(lock_do_i_hold(lock1));
				spl = splhigh();
				NEcount--;
				struct thread *next_thread = q_remhead(queue1);
				thread_wakeup(next_thread);
				splx(spl);
			}
			lock_release(lock1);



			lock_acquire(lock3);
			while (SW == 1)
			{
				assert(lock3 != NULL);
				assert(lock_do_i_hold(lock3));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock3);
				SWcount++;
				q_preallocate(queue3, SWcount);
				q_addtail(queue3, curthread);
				thread_sleep(curthread);
				lock_acquire(lock3);
				splx(spl);
			}

			SW = 1;
			// Print message
			kprintf("Car %lu: Reached SW from NW, heading towards SOUTH\n", carnumber);
			SW = 0;
			if (NWcount != 0)
			{
				assert(lock0 != NULL);
				assert(lock_do_i_hold(lock0));
				spl = splhigh();
				NWcount--;
				struct thread *next_thread = q_remhead(queue0);
				thread_wakeup(next_thread);
				splx(spl);
			}
			lock_release(lock0);
			if (SWcount != 0)
			{
				assert(lock3 != NULL);
				assert(lock_do_i_hold(lock3));
				spl = splhigh();
				SWcount--;
				struct thread *next_thread = q_remhead(queue3);
				thread_wakeup(next_thread);
				splx(spl);
			}
			kprintf("Car %lu: Reached SOUTH from EAST\n", carnumber);
			lock_release(lock3);

		}

		else if (cardirection == 2)
		{
			// Print message
			kprintf("Car %lu: Approaching SOUTH, heading towards WEST\n", carnumber);
			lock_acquire(lock2);
			while (SE == 1)
			{
				assert(lock2 != NULL);
				assert(lock_do_i_hold(lock2));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock2);
				SEcount++;
				q_preallocate(queue2, SEcount);
				q_addtail(queue2, curthread);
				thread_sleep(curthread);
				lock_acquire(lock2);
				splx(spl);
			}
			SE = 1;
			// Print message
			kprintf("Car %lu: Reached SE from SOUTH, heading towards NE\n", carnumber);
			SE = 0;

			lock_acquire(lock1);
			while (NE == 1)
			{
				assert(lock1 != NULL);
				assert(lock_do_i_hold(lock1));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock1);
				NEcount++;
				q_preallocate(queue1, NEcount);
				q_addtail(queue1, curthread);
				thread_sleep(curthread);
				lock_acquire(lock1);
				splx(spl);
			}
			NE = 1;
			// Print message
			kprintf("Car %lu: Reached NE from SE, heading towards NW\n", carnumber);
			NE = 0;

			if (SEcount != 0)
			{
				assert(lock2 != NULL);
				assert(lock_do_i_hold(lock2));
				spl = splhigh();
				SEcount--;
				struct thread *next_thread = q_remhead(queue2);
				thread_wakeup(next_thread);
				splx(spl);
			}
			lock_release(lock2);



			lock_acquire(lock0);
			while (NW == 1)
			{
				assert(lock0 != NULL);
				assert(lock_do_i_hold(lock0));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock0);
				NWcount++;
				q_preallocate(queue0, NWcount);
				q_addtail(queue0, curthread);
				thread_sleep(curthread);
				lock_acquire(lock0);
				splx(spl);
			}

			NW = 1;
			// Print message
			kprintf("Car %lu: Reached NW from NE, heading towards WEST\n", carnumber);
			NW = 0;
			if (NEcount != 0)
			{
				assert(lock1 != NULL);
				assert(lock_do_i_hold(lock1));
				spl = splhigh();
				NEcount--;
				struct thread *next_thread = q_remhead(queue1);
				thread_wakeup(next_thread);
				splx(spl);
			}
			lock_release(lock1);
			if (NWcount != 0)
			{
				assert(lock0 != NULL);
				assert(lock_do_i_hold(lock0));
				spl = splhigh();
				NWcount--;
				struct thread *next_thread = q_remhead(queue0);
				thread_wakeup(next_thread);
				splx(spl);
			}
			kprintf("Car %lu: Reached WEST from SOUTH\n", carnumber);

			lock_release(lock0);

		}

		else if (cardirection == 3)
		{
			// Print message
			kprintf("Car %lu: Approaching WEST, heading towards NORTH\n", carnumber);
			lock_acquire(lock3);
			while (SW == 1)
			{
				assert(lock3 != NULL);
				assert(lock_do_i_hold(lock3));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock3);
				SWcount++;
				q_preallocate(queue3, SWcount);
				q_addtail(queue3, curthread);
				thread_sleep(curthread);
				lock_acquire(lock3);
				splx(spl);
			}
			SW = 1;
			// Print message
			kprintf("Car %lu: Reached SW from WEST, heading towards SE\n", carnumber);
			SW = 0;

			lock_acquire(lock2);
			while (SE == 1)
			{
				assert(lock2 != NULL);
				assert(lock_do_i_hold(lock2));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock2);
				SEcount++;
				q_preallocate(queue2, SEcount);
				q_addtail(queue2, curthread);
				thread_sleep(curthread);
				lock_acquire(lock2);
				splx(spl);
			}
			SE = 1;
			// Print message
			kprintf("Car %lu: Reached SE from SW, heading towards NE\n", carnumber);
			SE = 0;

			if (SWcount != 0)
			{
				assert(lock3 != NULL);
				assert(lock_do_i_hold(lock3));
				spl = splhigh();
				SWcount--;
				struct thread *next_thread = q_remhead(queue3);
				thread_wakeup(next_thread);
				splx(spl);
			}
			lock_release(lock3);



			lock_acquire(lock1);
			while (NE == 1)
			{
				assert(lock1 != NULL);
				assert(lock_do_i_hold(lock1));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock1);
				NEcount++;
				q_preallocate(queue1, NEcount);
				q_addtail(queue1, curthread);
				thread_sleep(curthread);
				lock_acquire(lock1);
				splx(spl);
			}

			NE = 1;
			// Print message
			kprintf("Car %lu: Reached NE from SE, heading towards NORTH\n", carnumber);
			NE = 0;
			if (SEcount != 0)
			{
				assert(lock2 != NULL);
				assert(lock_do_i_hold(lock2));
				spl = splhigh();
				SEcount--;
				struct thread *next_thread = q_remhead(queue2);
				thread_wakeup(next_thread);
				splx(spl);
			}
			lock_release(lock2);
			if (NEcount != 0)
			{
				assert(lock1 != NULL);
				assert(lock_do_i_hold(lock1));
				spl = splhigh();
				NEcount--;
				struct thread *next_thread = q_remhead(queue1);
				thread_wakeup(next_thread);
				splx(spl);
			}
			kprintf("Car %lu: Reached NORTH from WEST\n", carnumber);
			lock_release(lock1);

		}
}


/*
 * turnright()
 *
 * Arguments:
 *      unsigned long cardirection: the direction from which the car
 *              approaches the intersection.
 *      unsigned long carnumber: the car id number for printing purposes.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      This function should implement making a right turn through the 
 *      intersection from any direction.
 *      Write and comment this function.
 */

static
void
turnright(unsigned long cardirection,
          unsigned long carnumber)
{
        /*
         * Avoid unused variable warnings.
         */
     
		int spl;
		if (cardirection == 0)
		{
			kprintf("Car %lu: Approaching NORTH, heading towards WEST\n", carnumber);
			lock_acquire(lock0); //Acquire the lock if its available.

			while (NW == 1)
			{
				assert(lock0 != NULL);
				assert(lock_do_i_hold(lock0));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock0);
				NWcount++;
				q_preallocate(queue0, NWcount);
				q_addtail(queue0, curthread);
				thread_sleep(curthread);
				lock_acquire(lock0);
				splx(spl);
			}
			NW = 1;
			// Print message
			kprintf("Car %lu: Reached NW from NORTH, heading towards WEST\n", carnumber);
			NW = 0;
			if (NWcount != 0)
			{
				assert(lock0 != NULL);
				assert(lock_do_i_hold(lock0));
				spl = splhigh();
				NWcount--;
				struct thread *next_thread = q_remhead(queue0);
				thread_wakeup(next_thread);
				splx(spl);
			}
			kprintf("Car %lu: Reached WEST\n", carnumber);
			lock_release(lock0); //unlock code
		}
		else if (cardirection == 1)
		{
			kprintf("Car %lu: Approaching EAST, heading towards NORTH\n", carnumber);
			lock_acquire(lock1); 

			while (NE == 1)
			{
				assert(lock1 != NULL);
				assert(lock_do_i_hold(lock1));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock1);
				NEcount++;
				q_preallocate(queue1, NEcount);
				q_addtail(queue1, curthread);
				thread_sleep(curthread);
				lock_acquire(lock1);
				splx(spl);
			}
			NE = 1;
			// Print message
			kprintf("Car %lu: Reached NE from EAST, heading towards NORTH\n", carnumber);
			NE = 0;
			if (NEcount != 0)
			{
				assert(lock1 != NULL);
				assert(lock_do_i_hold(lock1));
				spl = splhigh();
				NEcount--;
				struct thread *next_thread = q_remhead(queue1);
				thread_wakeup(next_thread);
				splx(spl);
			}
			kprintf("Car %lu: Reached NORTH from EAST\n", carnumber);
			lock_release(lock1); //unlock code
		}
		else if (cardirection == 2)
		{
			kprintf("Car %lu: Approaching SOUTH, heading towards EAST\n", carnumber);
			lock_acquire(lock2); //Acquire the lock if its available.

			while (SE == 1)
			{
				assert(lock2 != NULL);
				assert(lock_do_i_hold(lock2));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock2);
				SEcount++;
				q_preallocate(queue2, SEcount);
				q_addtail(queue2, curthread);
				thread_sleep(curthread);
				lock_acquire(lock2);
				splx(spl);
			}
			SE = 1;
			// Print message
			kprintf("Car %lu: Reached SE from SOUTH, heading towards EAST\n", carnumber);
			SE = 0;
			if (SEcount != 0)
			{
				assert(lock2 != NULL);
				assert(lock_do_i_hold(lock2));
				spl = splhigh();
				SEcount--;
				struct thread *next_thread = q_remhead(queue2);
				thread_wakeup(next_thread);
				splx(spl);
			}
			kprintf("Car %lu: Reached EAST from SOUTH\n", carnumber);
			lock_release(lock2); 
		}
		else if (cardirection == 3)
		{
			kprintf("Car %lu: Approaching WEST, heading towards SOUTH\n", carnumber);
			lock_acquire(lock3); //Acquire the lock if its available.

			while (SW == 1)
			{
				assert(lock3 != NULL);
				assert(lock_do_i_hold(lock3));
				assert(in_interrupt == 0);
				spl = splhigh();
				lock_release(lock3);
				SWcount++;
				q_preallocate(queue3, SWcount);
				q_addtail(queue3, curthread);
				thread_sleep(curthread);
				lock_acquire(lock3);
				splx(spl);
			}
			SW = 1;
			// Print message
			kprintf("Car %lu: Reached SW from WEST, heading towards SOUTH\n", carnumber);
			SW = 0;
			if (SWcount != 0)
			{
				assert(lock3 != NULL);
				assert(lock_do_i_hold(lock3));
				spl = splhigh();
				SWcount--;
				struct thread *next_thread = q_remhead(queue3);
				thread_wakeup(next_thread);
				splx(spl);
			}
			kprintf("Car %lu: Reached SOUTH from WEST\n", carnumber);
			lock_release(lock3);
		}
}


/*
 * approachintersection()
 *
 * Arguments: 
 *      void * unusedpointer: currently unused.
 *      unsigned long carnumber: holds car id number.
 *
 * Returns:
 *      nothing.
 *
 * Notes:
 *      Change this function as necessary to implement your solution. These
 *      threads are created by createcars().  Each one must choose a direction
 *      randomly, approach the intersection, choose a turn randomly, and then
 *      complete that turn.  The code to choose a direction randomly is
 *      provided, the rest is left to you to implement.  Making a turn
 *      or going straight should be done by calling one of the functions
 *      above.
 */
 
static
void
approachintersection(void * unusedpointer,
                     unsigned long carnumber)
{
    int cardirection;
    int turn;		// Turn taken
    int spl;
        /*
         * Avoid unused variable and function warnings.
         */

        (void) unusedpointer;
        (void) carnumber;
	(void) gostraight;
	(void) turnleft;
	(void) turnright;

	lock_acquire(temp);
	while (limit > 2)
	{
		assert(temp != NULL);
		assert(lock_do_i_hold(temp));
		assert(in_interrupt == 0);
		spl = splhigh();
		lock_release(temp);
		count++;
		q_preallocate(queue4, count);
		q_addtail(queue4, curthread);
		thread_sleep(curthread);
		lock_acquire(temp);
		splx(spl);
	}
	limit++;
        /*
         * cardirection is set randomly.(From where the car approaches the intersection
         * 0 -> North
         * 1 -> East
         * 2 -> South
         * 3 -> Wesr
         */

        cardirection = random() % 4;
	
	/* Assigining a random direction
 	 * 0 -> Right
 	 * 1 -> Left
 	 * 2 -> Straight
 	 */

	turn = random() % 3;
	lock_release(temp);

	if (turn == 0)
		turnright(cardirection,carnumber);
	else if (turn == 1)
		turnleft(cardirection,carnumber);
	else if (turn == 2)
		gostraight(cardirection,carnumber);

	lock_acquire(temp);
	limit=limit-1;
	if (count != 0)
	{
		assert(temp != NULL);
		assert(lock_do_i_hold(temp));
		spl = splhigh();
		count--;
		struct thread *next_thread = q_remhead(queue4);
		thread_wakeup(next_thread);
		splx(spl);
	}

	lock_release(temp);
}


/*
 * createcars()
 *
 * Arguments:
 *      int nargs: unused.
 *      char ** args: unused.
 *
 * Returns:
 *      0 on success.
 *
 * Notes:
 *      Driver code to start up the approachintersection() threads.  You are
 *      free to modiy this code as necessary for your solution.
 */

int
createcars(int nargs,
           char ** args)
{
        int index, error;
	//
        /*
         * Avoid unused variable warnings.
         */

        (void) nargs;
        (void) args;
	
	// Creating locks
	
	lock0 = lock_create("lock0");
	lock1 = lock_create("lock1");
	lock2 = lock_create("lock2");
	lock3 = lock_create("lock3");
	temp = lock_create("temp");

	// Creating queues
	
	queue0 = q_create(1);
	queue1 = q_create(1);
	queue2 = q_create(1);
	queue3 = q_create(1);
	queue4 = q_create(1);
        /*
         * Start NCARS approachintersection() threads.
         */

        for (index = 0; index < NCARS; index++) {

                error = thread_fork("approachintersection thread",
                                    NULL,
                                    index,
                                    approachintersection,
                                    NULL
                                    );

                /*
                 * panic() on error.
                 */

                if (error) {
                        
                        panic("approachintersection: thread_fork failed: %s\n",
                              strerror(error)
                              );
                }
        }
        return 0;
}
