#ifndef RUNTIME_H
#define RUNTIME_H

#include "memory.h"

#if defined(QK_WINDOWS)

#include "windows/process.h"

#elif defined(QK_UNIX)

#include "unix/process.h"

#endif

#endif  // RUNTIME_H
