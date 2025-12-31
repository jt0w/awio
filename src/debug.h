#ifndef DEBUG_H
#define DEBUG_H
#ifdef AWIO_DEBUG
#include <signal.h>
#define breakpoint() raise(SIGTRAP)
#else
#define breakpoint()
#endif
#endif // endif DEBUG_H
