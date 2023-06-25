#ifndef FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_CONTROL_H_
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_CONTROL_H_

#if defined _WIN32 || defined __CYGWIN__
#ifdef __GNUC__
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_EXPORT __attribute__((dllexport))
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_IMPORT __attribute__((dllimport))
#else
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_EXPORT __declspec(dllexport)
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_IMPORT __declspec(dllimport)
#endif
#ifdef FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_BUILDING_DLL
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_PUBLIC \
  FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_EXPORT
#else
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_PUBLIC \
  FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_IMPORT
#endif
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_PUBLIC_TYPE \
  FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_PUBLIC
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_LOCAL
#else
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_EXPORT __attribute__((visibility("default")))
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_IMPORT
#if __GNUC__ >= 4
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_PUBLIC __attribute__((visibility("default")))
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_LOCAL __attribute__((visibility("hidden")))
#else
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_PUBLIC
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_LOCAL
#endif
#define FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_PUBLIC_TYPE
#endif

#endif  // FOUR_WHEEL_STEERING_CONTROLLER__VISIBILITY_CONTROL_H_
