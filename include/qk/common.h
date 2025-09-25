#ifndef COMMON_H
#define COMMON_H

#include "../../qk/api.h"

#ifdef QK_IPC
#include "qk_ipc.h"
#endif

#ifdef QK_FILEPATH
#include "qk_filepath.h"
#endif

#ifdef QK_EVENTS
#include "qk_events.h"
#endif

#ifdef QK_THREADING
#include "qk_threading.h"
#endif

#ifdef QK_RUNTIME_UTILS
#include "qk_runtime.h"
#endif

#endif  // COMMON_H
