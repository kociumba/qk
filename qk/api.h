#ifndef API_H
#define API_H

#ifdef QK_BUILD_SHARED

#ifdef _WIN32

#ifdef QK_EXPORTS
#define QK_API __declspec(dllexport)
#else
#define QK_API __declspec(dllimport)
#endif

#else
#define QK_API __attribute__((visibility("default")))
#endif

#else
#define QK_API
#endif

#endif  // API_H
