#ifndef GFSIM_MODEL_API_V1_H
#define GFSIM_MODEL_API_V1_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PYCIRCUIT_MODEL_ABI_V1 1u

typedef struct PycircuitModelV1 PycircuitModelV1;
typedef int32_t PycircuitModelStatusV1;
typedef int32_t PycircuitModelStepStateV1;

enum {
  PYCIRCUIT_MODEL_STATUS_V1_OK = 0,
  PYCIRCUIT_MODEL_STATUS_V1_INVALID_ARGUMENT = 1,
  PYCIRCUIT_MODEL_STATUS_V1_ABI_MISMATCH = 2,
  PYCIRCUIT_MODEL_STATUS_V1_INVALID_STATE = 3,
  PYCIRCUIT_MODEL_STATUS_V1_RUNTIME_FAILURE = 4,
};

enum {
  PYCIRCUIT_MODEL_STEP_V1_RUNNING = 0,
  PYCIRCUIT_MODEL_STEP_V1_QUIESCENT = 1,
  PYCIRCUIT_MODEL_STEP_V1_TERMINATED = 2,
  PYCIRCUIT_MODEL_STEP_V1_FAILED = 3,
};

typedef struct PycircuitModelBufferV1 {
  const uint8_t *data;
  uint64_t size;
} PycircuitModelBufferV1;

typedef struct PycircuitModelStepResultV1 {
  uint32_t struct_size;
  PycircuitModelStepStateV1 state;
  uint64_t epoch_time;
  uint32_t epoch_delta;
  uint32_t reserved;
} PycircuitModelStepResultV1;

typedef struct PycircuitModelApiV1 {
  uint32_t struct_size;
  uint32_t abi_version;

  PycircuitModelStatusV1 (*create)(PycircuitModelV1 **model);
  void (*destroy)(PycircuitModelV1 *model);
  /* Created -> Configured. Accepts canonical {} or pycircuit-model-config v1. */
  PycircuitModelStatusV1 (*configure_json)(PycircuitModelV1 *model,
                                         const uint8_t *data, uint64_t size);
  /* Configured/Ready/Completed/Failed -> Ready and restores deterministic
   * state. */
  PycircuitModelStatusV1 (*reset)(PycircuitModelV1 *model);
  PycircuitModelStatusV1 (*step)(PycircuitModelV1 *model,
                               PycircuitModelStepResultV1 *result);
  PycircuitModelStatusV1 (*statistics_json)(PycircuitModelV1 *model,
                                          PycircuitModelBufferV1 *result);
  PycircuitModelStatusV1 (*last_error)(PycircuitModelV1 *model,
                                     PycircuitModelBufferV1 *result);
} PycircuitModelApiV1;

#if UINTPTR_MAX != UINT64_MAX
#error "PycircuitModelApiV1 requires a 64-bit host ABI"
#endif

#if defined(__cplusplus)
static_assert(sizeof(PycircuitModelApiV1) == 64,
              "PycircuitModelApiV1 layout changed");
static_assert(sizeof(PycircuitModelBufferV1) == 16,
              "PycircuitModelBufferV1 layout changed");
static_assert(sizeof(PycircuitModelStepResultV1) == 24,
              "PycircuitModelStepResultV1 layout changed");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(PycircuitModelApiV1) == 64,
               "PycircuitModelApiV1 layout changed");
_Static_assert(sizeof(PycircuitModelBufferV1) == 16,
               "PycircuitModelBufferV1 layout changed");
_Static_assert(sizeof(PycircuitModelStepResultV1) == 24,
               "PycircuitModelStepResultV1 layout changed");
#endif

typedef const PycircuitModelApiV1 *(*PycircuitModelQueryV1)(void);

/* A model bundle defines PYCIRCUIT_MODEL_BUILD before including this header so the
   entry point declaration carries the export attribute the shared library needs.
   A declaration and its definition have to agree on that attribute: MSVC reports
   C2375 "redefinition; different linkage" for a dllexport definition whose
   earlier declaration lacks it. Consumers that only call the entry point leave
   the macro undefined and see a plain declaration. */
#if !defined(PYCIRCUIT_MODEL_EXPORT)
#if defined(PYCIRCUIT_MODEL_BUILD) && defined(_WIN32)
#define PYCIRCUIT_MODEL_EXPORT __declspec(dllexport)
#elif defined(PYCIRCUIT_MODEL_BUILD) && (defined(__GNUC__) || defined(__clang__))
#define PYCIRCUIT_MODEL_EXPORT __attribute__((visibility("default")))
#else
#define PYCIRCUIT_MODEL_EXPORT
#endif
#endif

PYCIRCUIT_MODEL_EXPORT const PycircuitModelApiV1 *pycircuit_model_query_v1(void);

#ifdef __cplusplus
}
#endif

#endif
