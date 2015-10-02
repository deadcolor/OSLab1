#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/switch.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

//User added header file
#include "threads/fixed_point.h"

#ifdef USERPROG
#include "userprog/process.h"
#endif

/* Random value for struct thread's `magic' member.
   Used to detect stack overflow.  See the big comment at the top
   of thread.h for details. */
#define THREAD_MAGIC 0xcd6abf4b

/* List of processes in THREAD_READY state, that is, processes
   that are ready to run but not actually running. */
static struct list ready_list;

/* List of all processes.  Processes are added to this list
   when they are first scheduled and removed when they exit. */
static struct list all_list;

/* Idle thread. */
static struct thread *idle_thread;

/* Initial thread, the thread running init.c:main(). */
static struct thread *initial_thread;

/* Lock used by allocate_tid(). */
static struct lock tid_lock;

/* Stack frame for kernel_thread(). */
struct kernel_thread_frame 
  {
    void *eip;                  /* Return address. */
    thread_func *function;      /* Function to call. */
    void *aux;                  /* Auxiliary data for function. */
  };

/* Statistics. */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

/* Scheduling. */
#define TIME_SLICE 4            /* # of timer ticks to give each thread. */
static unsigned thread_ticks;   /* # of timer ticks since last yield. */

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
bool thread_mlfqs;
int load_avg;           //Used in advanced scheduler, load average

static void kernel_thread (thread_func *, void *aux);

static void idle (void *aux UNUSED);
static struct thread *running_thread (void);
static struct thread *next_thread_to_run (void);
static void init_thread (struct thread *, const char *name, int priority);
static bool is_thread (struct thread *) UNUSED;
static void *alloc_frame (struct thread *, size_t size);
static void schedule (void);
void thread_schedule_tail (struct thread *prev);
static tid_t allocate_tid (void);

/* Initializes the threading system by transforming the code
   that's currently running into a thread.  This can't work in
   general and it is possible in this case only because loader.S
   was careful to put the bottom of the stack at a page boundary.

   Also initializes the run queue and the tid lock.

   After calling this function, be sure to initialize the page
   allocator before trying to create any threads with
   thread_create().

   It is not safe to call thread_current() until this function
   finishes. */
void
thread_init (void) 
{
  ASSERT (intr_get_level () == INTR_OFF);

  lock_init (&tid_lock);
  list_init (&ready_list);
  list_init (&all_list);

  /* Set up a thread structure for the running thread. */
  initial_thread = running_thread ();
  init_thread (initial_thread, "main", PRI_DEFAULT);
  initial_thread->status = THREAD_RUNNING;
  initial_thread->tid = allocate_tid ();
}

/* Starts preemptive thread scheduling by enabling interrupts.
   Also creates the idle thread. */
void
thread_start (void) 
{
  /* Create the idle thread. */
  struct semaphore idle_started;
  sema_init (&idle_started, 0);
  thread_create ("idle", PRI_MIN, idle, &idle_started);

  load_avg=INTtoFP(0);      //initialize load_average to 0 in fixed point

  /* Start preemptive thread scheduling. */
  intr_enable ();

  /* Wait for the idle thread to initialize idle_thread. */
  sema_down (&idle_started);
}

/* Called by the timer interrupt handler at each timer tick.
   Thus, this function runs in an external interrupt context. */
void
thread_tick (void) 
{
  struct thread *t = thread_current ();

  /* Update statistics. */
  if (t == idle_thread)
    idle_ticks++;
#ifdef USERPROG
  else if (t->pagedir != NULL)
    user_ticks++;
#endif
  else
    kernel_ticks++;

  /* Enforce preemption. */
  if (++thread_ticks >= TIME_SLICE)
    intr_yield_on_return ();
}

/* Prints thread statistics. */
void
thread_print_stats (void) 
{
  printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
          idle_ticks, kernel_ticks, user_ticks);
}

/* Creates a new kernel thread named NAME with the given initial
   PRIORITY, which executes FUNCTION passing AUX as the argument,
   and adds it to the ready queue.  Returns the thread identifier
   for the new thread, or TID_ERROR if creation fails.

   If thread_start() has been called, then the new thread may be
   scheduled before thread_create() returns.  It could even exit
   before thread_create() returns.  Contrariwise, the original
   thread may run for any amount of time before the new thread is
   scheduled.  Use a semaphore or some other form of
   synchronization if you need to ensure ordering.

   The code provided sets the new thread's `priority' member to
   PRIORITY, but no actual priority scheduling is implemented.
   Priority scheduling is the goal of Problem 1-3. */
tid_t
thread_create (const char *name, int priority,
               thread_func *function, void *aux) 
{
  struct thread *t;
  struct kernel_thread_frame *kf;
  struct switch_entry_frame *ef;
  struct switch_threads_frame *sf;
  tid_t tid;
  enum intr_level old_level;

  ASSERT (function != NULL);

  /* Allocate thread. */
  t = palloc_get_page (PAL_ZERO);
  if (t == NULL)
    return TID_ERROR;

  /* Initialize thread. */
  init_thread (t, name, priority);
  tid = t->tid = allocate_tid ();

  /* Prepare thread for first run by initializing its stack.
     Do this atomically so intermediate values for the 'stack' 
     member cannot be observed. */
  old_level = intr_disable ();

  /* Stack frame for kernel_thread(). */
  kf = alloc_frame (t, sizeof *kf);
  kf->eip = NULL;
  kf->function = function;
  kf->aux = aux;

  /* Stack frame for switch_entry(). */
  ef = alloc_frame (t, sizeof *ef);
  ef->eip = (void (*) (void)) kernel_thread;

  /* Stack frame for switch_threads(). */
  sf = alloc_frame (t, sizeof *sf);
  sf->eip = switch_entry;
  sf->ebp = 0;

  intr_set_level (old_level);

  /* Add to run queue. */
  thread_unblock (t);
	
	thread_preempt();
  /*
  thread_update_ready_list();
  thread_yield();
  thread_update_ready_list();
  */
  return tid;
}

/* Puts the current thread to sleep.  It will not be scheduled
   again until awoken by thread_unblock().

   This function must be called with interrupts turned off.  It
   is usually a better idea to use one of the synchronization
   primitives in synch.h. */
void
thread_block (void) 
{
  ASSERT (!intr_context ());
  ASSERT (intr_get_level () == INTR_OFF);

  thread_current ()->status = THREAD_BLOCKED;
  schedule ();
}

/* Transitions a blocked thread T to the ready-to-run state.
   This is an error if T is not blocked.  (Use thread_yield() to
   make the running thread ready.)

   This function does not preempt the running thread.  This can
   be important: if the caller had disabled interrupts itself,
   it may expect that it can atomically unblock a thread and
   update other data. */
void
thread_unblock (struct thread *t) 
{
  enum intr_level old_level;

  ASSERT (is_thread (t));

  old_level = intr_disable ();
  ASSERT (t->status == THREAD_BLOCKED);
  list_insert_ordered (&ready_list, &t->elem, thread_priority_cmp, NULL);
  t->status = THREAD_READY;
  intr_set_level (old_level);
}

/* Returns the name of the running thread. */
const char *
thread_name (void) 
{
  return thread_current ()->name;
}

/* Returns the running thread.
   This is running_thread() plus a couple of sanity checks.
   See the big comment at the top of thread.h for details. */
struct thread *
thread_current (void) 
{
  struct thread *t = running_thread ();
  
  /* Make sure T is really a thread.
     If either of these assertions fire, then your thread may
     have overflowed its stack.  Each thread has less than 4 kB
     of stack, so a few big automatic arrays or moderate
     recursion can cause stack overflow. */
  ASSERT (is_thread (t));
  ASSERT (t->status == THREAD_RUNNING);

  return t;
}

/* Returns the running thread's tid. */
tid_t
thread_tid (void) 
{
  return thread_current ()->tid;
}

/* Deschedules the current thread and destroys it.  Never
   returns to the caller. */
void
thread_exit (void) 
{
  ASSERT (!intr_context ());

#ifdef USERPROG
  process_exit ();
#endif

  /* Remove thread from all threads list, set our status to dying,
     and schedule another process.  That process will destroy us
     when it calls thread_schedule_tail(). */
  intr_disable ();
  list_remove (&thread_current()->allelem);
  thread_current ()->status = THREAD_DYING;
  schedule ();
  NOT_REACHED ();
}

/* Yields the CPU.  The current thread is not put to sleep and
   may be scheduled again immediately at the scheduler's whim. */
void
thread_yield (void) 
{
  struct thread *cur = thread_current ();
  enum intr_level old_level;
  
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  if (cur != idle_thread) 
     list_insert_ordered (&ready_list, &cur->elem, thread_priority_cmp, NULL);
  cur->status = THREAD_READY;
  schedule ();
  intr_set_level (old_level);
}

/* Invoke function 'func' on all threads, passing along 'aux'.
   This function must be called with interrupts off. */
void
thread_foreach (thread_action_func *func, void *aux)
{
  struct list_elem *e;

  ASSERT (intr_get_level () == INTR_OFF);

  for (e = list_begin (&all_list); e != list_end (&all_list);
       e = list_next (e))
    {
      struct thread *t = list_entry (e, struct thread, allelem);
      func (t, aux);
    }
}

/* Sets the current thread's priority to NEW_PRIORITY. */
void
thread_set_priority (int new_priority) 
{
    //if mlfqs is on, should not modify priority
    if(thread_mlfqs)
        return;

    enum intr_level old_level = intr_disable();
    struct thread *cur = thread_current();
    int old = cur -> priority;
    int donation = cur -> donated_priority;

    cur->priority = new_priority;
                        
    //if(old > new_priority && donation < new_priority)
    if(old > new_priority && !donation)
    {
        thread_update_ready_list();
       	thread_preempt();
    }

    intr_set_level(old_level);
}

/* Returns the current thread's priority. */
int
thread_get_priority (void) 
{
    struct thread* cur = thread_current();
    //if (cur->donated_priority != 0)
    if (cur->donated_priority != 0)
        return cur -> donated_priority;
    else
        return cur -> priority;
}

/* Returns the argumen thread's priority */
int
thread_get_priority_arg (struct thread *t) 
{
    if (t->donated_priority != 0)
        return t -> donated_priority;
    else
        return t -> priority;
}

/* Donate thread's priority */
void
thread_donate_priority(struct thread *donor, struct thread *donee)
{
    if(donor -> donated_priority == 0)
        donee -> donated_priority = donor -> priority;
    else
        donee -> donated_priority = donor -> donated_priority;
    //
    //thread_unblock(donee);
    thread_update_ready_list();
    //thread_unblock(donee);
    //thread_yield();
}

/* return priority */
void
thread_recall_priority(struct thread *donee)
{
	int i;
	int new, max;
	struct list_elem *walk;
	struct list_elem *next = NULL;
	struct thread *t;
	struct lock *tmp;
	
	if(list_empty(&donee->locks))
	{
		donee->donated_priority = 0;
		thread_update_ready_list();
		return ;
	}

	/*
	if(list_empty(&tmp->semaphore.waiters))
	t = list_entry(list_front(&tmp->semaphore.waiters), struct thread, elem);
	max = t->priority;
	*/
	max = 0;
	walk = list_begin(&donee->locks);
	
	while(walk != list_end(&donee->locks))
	{
		tmp = list_entry(walk, struct lock, elem);

		if(list_empty(&tmp->semaphore.waiters))
		{
			walk = list_next(walk);
			continue;
		}
		t = list_entry(list_front(&tmp->semaphore.waiters), struct thread, elem);
		new = t->priority;
		//new one is higher than current one
		if(max < new)
			max = new;
		
		walk = list_next(walk);
	}

  donee->donated_priority = max;
  thread_update_ready_list();
}

/* Sorting ready_list */
void
thread_update_ready_list(void)
{
    list_sort(&ready_list, thread_priority_cmp, NULL);
}

void
thread_preempt(void)
{
    struct thread *topThread;
    struct thread *cur = thread_current();
    if(list_empty(&ready_list))
        return;

    topThread=list_entry(list_front(&ready_list),struct thread, elem);
  
    if(thread_get_priority_arg(cur) < thread_get_priority_arg(topThread))
    {
        if(intr_context())  //if in timer interrupt
            intr_yield_on_return();
        else
            thread_yield();
    }

}

/* Compare thread's priority. Used in sorting list */
bool
thread_priority_cmp(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
    struct thread *ta = list_entry (a, struct thread, elem);
    struct thread *tb = list_entry (b, struct thread, elem);
    return thread_get_priority_arg(ta) > thread_get_priority_arg(tb);
}

/* Sets the current thread's nice value to NICE. */
void
thread_set_nice (int nice) 
{
    enum intr_level old_level = intr_disable();
    thread_current()->nice = nice;
    mlfqs_update_priority(thread_current());
    intr_set_level(old_level);
}

/* Returns the current thread's nice value. */
int
thread_get_nice (void) 
{
    return thread_current()->nice;
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void) 
{
  return FP_ROUND(MUL_FP_INT(load_avg,100));
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void) 
{
    return FP_ROUND(MUL_FP_INT(thread_current()->recent_cpu,100));
}

//Update load_avg every 1 second
void mlfqs_update_load_avg(void)
{
    int ready_size = list_size (&ready_list)+1; //size of ready_list+running thread
    int temp1;
    int temp2;
    
    //if current thread is not running
    if(thread_current()==idle_thread)
        ready_size--;

    temp1=MUL_FP(DIV_FP(INTtoFP(59),INTtoFP(60)),load_avg);
    temp2=MUL_FP_INT(DIV_FP(INTtoFP(1),INTtoFP(60)),ready_size);

    load_avg=ADD_FP(temp1,temp2);
}

//Update recent_cpu determined by load_avg and nice
void mlfqs_update_recent_cpu(struct thread *t)
{
    int temp1;
    int temp2;
    int temp3;

    if(t==idle_thread)
        return;

    temp1=MUL_FP_INT(load_avg,2);
    temp2=ADD_FP_INT(temp1,1);
    temp3=MUL_FP(DIV_FP(temp1,temp2),t->recent_cpu);

    t->recent_cpu=ADD_FP_INT(temp3,t->nice);
}

//Update thread's priority determined by nice and recent_cpu
void mlfqs_update_priority(struct thread *t)
{
    int temp1;
    int temp2;
    if(t==idle_thread)
        return;

    temp1=FPtoINT(DIV_FP_INT(t->recent_cpu,4));
    temp2=(t->nice)*2;
    
    t->priority=PRI_MAX-temp1-temp2;

    if(t->priority<PRI_MIN)
        t->priority=PRI_MIN;
    else if(t->priority>PRI_MAX)
        t->priority=PRI_MAX;

    if(t==thread_current())
        chkCurrentPriorityHigh();

}

//Increase recent_cpu of current thread on every tick 
void mlfqs_incr_recent_cpu(void)
{
    int tempCpu=thread_current()->recent_cpu;

    if(thread_current()==idle_thread)
        return;

    thread_current()->recent_cpu=ADD_FP_INT(tempCpu,1);
}
 
//Recalculate recent_cpu, priority, load_avg 
void mlfqs_updates_all(void)
{
    if(!thread_mlfqs)
        return;

    mlfqs_update_load_avg();

    struct list_elem *e;
    struct thread *temp;
    for(e=list_begin(&all_list) ; e != list_end (&all_list) ; e = list_next(e))
    {
        temp = list_entry(e, struct thread, allelem);
        mlfqs_update_recent_cpu(temp);
        mlfqs_update_priority(temp);
    }
}

//Check current thread's priority has still highest priority in ready queue 
void chkCurrentPriorityHigh(void)
{
    struct thread *topThread;
    if(list_empty(&ready_list))
        return;

    topThread=list_entry(list_front(&ready_list),struct thread, elem);
  
    if(thread_current()->priority < topThread->priority)
    {
        if(intr_context())  //if in timer interrupt
            intr_yield_on_return();
        else
            thread_yield();
    }

}


/* Idle thread.  Executes when no other thread is ready to run.

   The idle thread is initially put on the ready list by
   thread_start().  It will be scheduled once initially, at which
   point it initializes idle_thread, "up"s the semaphore passed
   to it to enable thread_start() to continue, and immediately
   blocks.  After that, the idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */
static void
idle (void *idle_started_ UNUSED) 
{
  struct semaphore *idle_started = idle_started_;
  idle_thread = thread_current ();
  sema_up (idle_started);

  for (;;) 
    {
      /* Let someone else run. */
      intr_disable ();
      thread_block ();

      /* Re-enable interrupts and wait for the next one.

         The `sti' instruction disables interrupts until the
         completion of the next instruction, so these two
         instructions are executed atomically.  This atomicity is
         important; otherwise, an interrupt could be handled
         between re-enabling interrupts and waiting for the next
         one to occur, wasting as much as one clock tick worth of
         time.

         See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
         7.11.1 "HLT Instruction". */
      asm volatile ("sti; hlt" : : : "memory");
    }
}

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (thread_func *function, void *aux) 
{
  ASSERT (function != NULL);

  intr_enable ();       /* The scheduler runs with interrupts off. */
  function (aux);       /* Execute the thread function. */
  thread_exit ();       /* If function() returns, kill the thread. */
}

/* Returns the running thread. */
struct thread *
running_thread (void) 
{
  uint32_t *esp;

  /* Copy the CPU's stack pointer into `esp', and then round that
     down to the start of a page.  Because `struct thread' is
     always at the beginning of a page and the stack pointer is
     somewhere in the middle, this locates the curent thread. */
  asm ("mov %%esp, %0" : "=g" (esp));
  return pg_round_down (esp);
}

/* Returns true if T appears to point to a valid thread. */
static bool
is_thread (struct thread *t)
{
  return t != NULL && t->magic == THREAD_MAGIC;
}

/* Does basic initialization of T as a blocked thread named
   NAME. */
static void
init_thread (struct thread *t, const char *name, int priority)
{
  ASSERT (t != NULL);
  ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
  ASSERT (name != NULL);

  memset (t, 0, sizeof *t);
  t->status = THREAD_BLOCKED;
  strlcpy (t->name, name, sizeof t->name);
  t->stack = (uint8_t *) t + PGSIZE;

  t->nice = 0;
  t->priority = priority;
  t->donated_priority = 0;
  t->recent_cpu = INTtoFP(0);

  list_init(&t->locks);
  t->wait = NULL;
  t->magic = THREAD_MAGIC;
  list_push_back (&all_list, &t->allelem);
}

/* Allocates a SIZE-byte frame at the top of thread T's stack and
   returns a pointer to the frame's base. */
static void *
alloc_frame (struct thread *t, size_t size) 
{
  /* Stack data is always allocated in word-size units. */
  ASSERT (is_thread (t));
  ASSERT (size % sizeof (uint32_t) == 0);

  t->stack -= size;
  return t->stack;
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
static struct thread *
next_thread_to_run (void) 
{
  if (list_empty (&ready_list))
    return idle_thread;
  else
    return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/* Used for ordering sleeping thread list in device/timer.c*/
bool thread_ticks_less(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
    struct thread *temp_a = list_entry(a, struct thread, elem);
    struct thread *temp_b = list_entry(b, struct thread, elem);
    if(temp_a->wakeup_time < temp_b->wakeup_time)
        return true;
    else
        return false;

}
/* Completes a thread switch by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.

   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.  This function is normally invoked by
   thread_schedule() as its final action before returning, but
   the first time a thread is scheduled it is called by
   switch_entry() (see switch.S).

   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function.

   After this function and its caller returns, the thread switch
   is complete. */
void
thread_schedule_tail (struct thread *prev)
{
  struct thread *cur = running_thread ();
  
  ASSERT (intr_get_level () == INTR_OFF);

  /* Mark us as running. */
  cur->status = THREAD_RUNNING;

  /* Start new time slice. */
  thread_ticks = 0;

#ifdef USERPROG
  /* Activate the new address space. */
  process_activate ();
#endif

  /* If the thread we switched from is dying, destroy its struct
     thread.  This must happen late so that thread_exit() doesn't
     pull out the rug under itself.  (We don't free
     initial_thread because its memory was not obtained via
     palloc().) */
  if (prev != NULL && prev->status == THREAD_DYING && prev != initial_thread) 
    {
      ASSERT (prev != cur);
      palloc_free_page (prev);
    }
}

/* Schedules a new process.  At entry, interrupts must be off and
   the running process's state must have been changed from
   running to some other state.  This function finds another
   thread to run and switches to it.

   It's not safe to call printf() until thread_schedule_tail()
   has completed. */
static void
schedule (void) 
{
  struct thread *cur = running_thread ();
  struct thread *next = next_thread_to_run ();
  struct thread *prev = NULL;

  ASSERT (intr_get_level () == INTR_OFF);
  ASSERT (cur->status != THREAD_RUNNING);
  ASSERT (is_thread (next));

  if (cur != next)
    prev = switch_threads (cur, next);
  thread_schedule_tail (prev);
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid (void) 
{
  static tid_t next_tid = 1;
  tid_t tid;

  lock_acquire (&tid_lock);
  tid = next_tid++;
  lock_release (&tid_lock);

  return tid;
}

/* Offset of `stack' member within `struct thread'.
   Used by switch.S, which can't figure it out on its own. */
uint32_t thread_stack_ofs = offsetof (struct thread, stack);
