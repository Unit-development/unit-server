#ifndef UNITSERVER_CONFIG_H
#define UNITSERVER_CONFIG_H

#ifdef _WIN32
#ifdef BUILDING_UNITSERVER
#define UNITSERVER_API __declspec(dllexport)
#else
#define UNITSERVER_API __declspec(dllimport)
#endif
#else
#define UNITSERVER_API __attribute__((visibility("default")))
#endif

#endif // UNITSERVER_CONFIG_H
