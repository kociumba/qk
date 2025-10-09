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

#ifdef QK_TRAITS
#include "qk_traits.h"
#endif

#ifdef QK_EMBEDDING
#include "qk_binary.h"
#endif

#ifdef QK_UTILS
#include "qk_utils.h"
#endif

#endif  // COMMON_H
