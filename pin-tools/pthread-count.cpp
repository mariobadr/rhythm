#include "pin.H"

#include <iostream>
#include <fstream>

#include "pthread-events.hpp"

/**
 * The total number of created threads.
 */
static UINT32 totalThreads = 0;

/**
 * A lock to increment totalThreads.
 */
PIN_LOCK totalThreadLock;

/**
 * The pin tool accepts the output file name as an argument.
 */
KNOB<std::string> KnobOutput(KNOB_MODE_WRITEONCE, "pintool", "o", "pthread-event-count.out", "output file name");

/**
 * Statistics to track for a thread.
 */
struct threadData {
  /**
   * Constructor.
   */
  threadData()
  {
    for(UINT32 i = 0; i < PTHREAD_CALLS_SIZE; i++) {
      eventCounts[i] = 0;
    }
  }

  UINT64 eventCounts[PTHREAD_CALLS_SIZE];
};

/**
 * The key used for thread-level storage.
 */
static TLS_KEY tlsKey;

/**
 * Get the ThreadData for a given thread.
 *
 * @param threadId The thread identifier to get the data for.
 * @return A pointer to the thread's ThreadData.
 */
threadData *GetThreadData(THREADID const threadId)
{
  return static_cast<threadData *>(PIN_GetThreadData(tlsKey, threadId));
}

VOID CountEvent(THREADID const threadId, UINT32 const index)
{
  threadData *data = GetThreadData(threadId);

  data->eventCounts[index]++;
}

/**
 * Add instrumentation calls to the pthread synchronization functions.
 *
 * @param image The image to check for function calls.
 */
VOID ImageLoad(IMG const image, VOID *)
{
  for(UINT32 i = 0; i < PTHREAD_CALLS_SIZE; ++i) {
    RTN routine = RTN_FindByName(image, events[i]);

    if(RTN_Valid(routine)) {
      RTN_Open(routine);

      RTN_InsertCall(routine, IPOINT_BEFORE, AFUNPTR(CountEvent), IARG_THREAD_ID, IARG_UINT32, i,
          IARG_END);

      RTN_Close(routine);
    }
  }
}

/**
 * Setup when a thread has begun execution (i.e., after it has been created).
 *
 * @param threadId The thread identifier.
 */
VOID ThreadStart(THREADID const threadId, CONTEXT *, INT32, VOID *)
{
  // setup thread local storage
  threadData *data = new threadData;
  PIN_SetThreadData(tlsKey, data, threadId);

  PIN_GetLock(&totalThreadLock, threadId + 1);
  totalThreads++;
  PIN_ReleaseLock(&totalThreadLock);
}

/**
 * Dump stats to file.
 */
VOID Fini(INT32, VOID *)
{
  FILE *output = fopen(KnobOutput.Value().c_str(), "w");
  fprintf(output, "tid,event,count\n");

  for(UINT32 i = 0; i < totalThreads; i++) {
    threadData *data = GetThreadData(i);

    for(UINT32 j = 0; j < PTHREAD_CALLS_SIZE; j++) {
      UINT64 count = data->eventCounts[j];

      if(count > 0) {
        fprintf(output, "%u,%s,%lu\n", i, events[j], count);
      }
    }
  }
}

/**
 * Print some help on how to use the pin tool.
 */
VOID PrintUsage()
{
  std::cerr << KNOB_BASE::StringKnobSummary() << "\n";
}

int main(int argc, char *argv[])
{
  PIN_InitSymbols();
  if(PIN_Init(argc, argv)) {
    PrintUsage();

    return EXIT_FAILURE;
  }

  tlsKey = PIN_CreateThreadDataKey(0);
  PIN_InitLock(&totalThreadLock);

  PIN_AddThreadStartFunction(ThreadStart, NULL);

  IMG_AddInstrumentFunction(ImageLoad, NULL);
  PIN_AddFiniFunction(Fini, NULL);

  // never returns
  PIN_StartProgram();

  return EXIT_SUCCESS;
}