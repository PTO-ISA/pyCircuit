#ifndef GFSIM_MODEL_API_V1_H
#define GFSIM_MODEL_API_V1_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AGENTIC_MODEL_ABI_V1 1u

typedef struct AgenticModelV1 AgenticModelV1;
typedef int32_t AgenticModelStatusV1;
typedef int32_t AgenticModelStepStateV1;

enum {
  AGENTIC_MODEL_STATUS_V1_OK = 0,
  AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT = 1,
  AGENTIC_MODEL_STATUS_V1_ABI_MISMATCH = 2,
  AGENTIC_MODEL_STATUS_V1_INVALID_STATE = 3,
  AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE = 4,
};

enum {
  AGENTIC_MODEL_STEP_V1_RUNNING = 0,
  AGENTIC_MODEL_STEP_V1_QUIESCENT = 1,
  AGENTIC_MODEL_STEP_V1_TERMINATED = 2,
  AGENTIC_MODEL_STEP_V1_FAILED = 3,
};

typedef struct AgenticModelBufferV1 {
  const uint8_t *data;
  uint64_t size;
} AgenticModelBufferV1;

typedef struct AgenticModelStepResultV1 {
  uint32_t struct_size;
  AgenticModelStepStateV1 state;
  uint64_t epoch_time;
  uint32_t epoch_delta;
  uint32_t reserved;
} AgenticModelStepResultV1;

typedef struct AgenticModelApiV1 {
  uint32_t struct_size;
  uint32_t abi_version;
  const char *sdk_product_version;
  const char *sdk_source_revision;

  AgenticModelStatusV1 (*create)(AgenticModelV1 **model);
  void (*destroy)(AgenticModelV1 *model);
  /* Created -> Configured. Accepts canonical {} or agentic-model-config v1. */
  AgenticModelStatusV1 (*configure_json)(AgenticModelV1 *model,
                                         const uint8_t *data, uint64_t size);
  /* Configured/Ready/Completed -> Ready and restores deterministic state. */
  AgenticModelStatusV1 (*reset)(AgenticModelV1 *model);
  AgenticModelStatusV1 (*step)(AgenticModelV1 *model,
                               AgenticModelStepResultV1 *result);
  AgenticModelStatusV1 (*statistics_json)(AgenticModelV1 *model,
                                          AgenticModelBufferV1 *result);
  AgenticModelStatusV1 (*last_error)(AgenticModelV1 *model,
                                     AgenticModelBufferV1 *result);
} AgenticModelApiV1;

#if UINTPTR_MAX != UINT64_MAX
#error "AgenticModelApiV1 requires a 64-bit host ABI"
#endif

#if defined(__cplusplus)
static_assert(sizeof(AgenticModelApiV1) == 80,
              "AgenticModelApiV1 layout changed");
static_assert(sizeof(AgenticModelBufferV1) == 16,
              "AgenticModelBufferV1 layout changed");
static_assert(sizeof(AgenticModelStepResultV1) == 24,
              "AgenticModelStepResultV1 layout changed");
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(AgenticModelApiV1) == 80,
               "AgenticModelApiV1 layout changed");
_Static_assert(sizeof(AgenticModelBufferV1) == 16,
               "AgenticModelBufferV1 layout changed");
_Static_assert(sizeof(AgenticModelStepResultV1) == 24,
               "AgenticModelStepResultV1 layout changed");
#endif

typedef const AgenticModelApiV1 *(*AgenticModelQueryV1)(void);

const AgenticModelApiV1 *agentic_model_query_v1(void);

#ifdef __cplusplus
}
#endif

#endif
