/* Independent C11 consumer for generated/dut.h and agentic_model_query_v1. */
#include "dut.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition, code)                                               \
  do {                                                                         \
    if (!(condition))                                                          \
      return (code);                                                           \
  } while (0)

static int contains(const char *text, const char *fragment) {
  return text != NULL && strstr(text, fragment) != NULL;
}

static int copy_buffer(const AgenticModelBufferV1 *buffer, char *destination,
                       size_t capacity) {
  if (buffer->size >= capacity || (buffer->size != 0 && buffer->data == NULL))
    return 0;
  if (buffer->size != 0)
    memcpy(destination, buffer->data, (size_t)buffer->size);
  destination[buffer->size] = '\0';
  return 1;
}

static int read_error(const AgenticModelApiV1 *api, AgenticModelV1 *model,
                      char *destination, size_t capacity) {
  AgenticModelBufferV1 buffer = {0};
  return api->last_error(model, &buffer) == AGENTIC_MODEL_STATUS_V1_OK &&
         copy_buffer(&buffer, destination, capacity);
}

static int read_statistics(const AgenticModelApiV1 *api, AgenticModelV1 *model,
                           char *destination, size_t capacity) {
  AgenticModelBufferV1 buffer = {0};
  return api->statistics_json(model, &buffer) == AGENTIC_MODEL_STATUS_V1_OK &&
         copy_buffer(&buffer, destination, capacity);
}

static int has_stat(const char *json, const char *name, const char *path,
                    uint64_t value) {
  char row[256];
  char value_fragment[48];
  const char *start;
  const char *end;
  const char *value_position;
  size_t remaining;
  snprintf(row, sizeof(row), "\"name\":\"%s\",\"object_path\":\"%s\"", name,
           path);
  start = strstr(json, row);
  if (start == NULL)
    return 0;
  end = strchr(start, '}');
  if (end == NULL)
    return 0;
  remaining = (size_t)(end - start);
  snprintf(value_fragment, sizeof(value_fragment), "\"value\":%llu",
           (unsigned long long)value);
  value_position = strstr(start, value_fragment);
  if (value_position == NULL || value_position >= end ||
      strlen(value_fragment) > remaining)
    return 0;
  value_position += strlen(value_fragment);
  return value_position == end || *value_position == ',' ||
         *value_position == ' ' || *value_position == '\n';
}

static AgenticModelStatusV1 configure(const AgenticModelApiV1 *api,
                                      AgenticModelV1 *model,
                                      const char *config) {
  return api->configure_json(model, (const uint8_t *)config,
                             (uint64_t)strlen(config));
}

static int check_common_table(const AgenticModelApiV1 *api) {
  return api != NULL && api->struct_size == sizeof(*api) &&
         api->abi_version == AGENTIC_MODEL_ABI_V1 && api->create != NULL &&
         api->destroy != NULL && api->configure_json != NULL &&
         api->reset != NULL && api->step != NULL &&
         api->statistics_json != NULL && api->last_error != NULL &&
         sizeof(AgenticModelApiV1) == 64 &&
         sizeof(AgenticModelBufferV1) == 16 &&
         sizeof(AgenticModelStepResultV1) == 24;
}

static int check_retryable_creation_and_config(const AgenticModelApiV1 *api,
                                               AgenticModelV1 **out_model) {
  AgenticModelV1 *model = (AgenticModelV1 *)(uintptr_t)1;
  char error[2048];
  AgenticModelStepResultV1 result = {sizeof(AgenticModelStepResultV1),
                                     AGENTIC_MODEL_STEP_V1_RUNNING, 77, 88, 99};
  const AgenticModelStepResultV1 untouched = result;
  REQUIRE(api->create(NULL) == AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT, 10);
  api->destroy(NULL);
  REQUIRE(api->create(&model) == AGENTIC_MODEL_STATUS_V1_OK && model != NULL,
          11);
  REQUIRE(api->step(model, &result) == AGENTIC_MODEL_STATUS_V1_INVALID_STATE,
          12);
  REQUIRE(result.state == untouched.state &&
              result.epoch_time == untouched.epoch_time &&
              result.epoch_delta == untouched.epoch_delta &&
              result.reserved == untouched.reserved,
          13);
  REQUIRE(read_error(api, model, error, sizeof(error)) &&
              contains(error, "\"code\":\"invalid_state\"") &&
              contains(error, "\"phase\":\"api\""),
          14);

  REQUIRE(api->configure_json(model, NULL, 1) ==
              AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT,
          15);
  REQUIRE(configure(api, model, "{ }") ==
              AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT,
          16);
  REQUIRE(configure(api, model,
                    "{\"deadlock_window\":0,\"max_domain_cycles\":{},"
                    "\"max_ticks\":null,\"schema\":\"agentic-model-config\","
                    "\"version\":\"1\"}") ==
              AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT,
          17);
  REQUIRE(read_error(api, model, error, sizeof(error)) &&
              contains(error, "\"code\":\"invalid_argument\"") &&
              contains(error, "\"phase\":\"configure\""),
          18);

  REQUIRE(configure(api, model,
                    "{\"deadlock_window\":null,\"max_domain_cycles\":{},"
                    "\"max_ticks\":3,\"schema\":\"agentic-model-config\","
                    "\"version\":\"1\"}") == AGENTIC_MODEL_STATUS_V1_OK,
          19);
  REQUIRE(read_error(api, model, error, sizeof(error)) && error[0] == '\0', 20);
  REQUIRE(configure(api, model, "{}") == AGENTIC_MODEL_STATUS_V1_INVALID_STATE,
          21);
  REQUIRE(api->reset(model) == AGENTIC_MODEL_STATUS_V1_OK, 22);

  /* The C ABI accepts its documented canonical unlimited configuration. */
  AgenticModelV1 *unlimited = NULL;
  REQUIRE(api->create(&unlimited) == AGENTIC_MODEL_STATUS_V1_OK &&
              unlimited != NULL,
          23);
  REQUIRE(configure(api, unlimited, "{}") == AGENTIC_MODEL_STATUS_V1_OK, 24);
  api->destroy(unlimited);
  *out_model = model;
  return 0;
}

static int run_active_model(const AgenticModelApiV1 *api,
                            AgenticModelV1 *model) {
  AgenticModelStepResultV1 result = {sizeof(AgenticModelStepResultV1),
                                     AGENTIC_MODEL_STEP_V1_RUNNING, 77, 88, 99};
  char statistics[8192];
  char replay_statistics[8192];
  char error[2048];
  unsigned index;

  REQUIRE(api->step(model, NULL) == AGENTIC_MODEL_STATUS_V1_INVALID_ARGUMENT,
          30);
  result.struct_size = sizeof(result) - 1;
  result.state = AGENTIC_MODEL_STEP_V1_RUNNING;
  result.epoch_time = 77;
  result.epoch_delta = 88;
  result.reserved = 99;
  REQUIRE(api->step(model, &result) == AGENTIC_MODEL_STATUS_V1_ABI_MISMATCH,
          31);
  REQUIRE(result.struct_size == sizeof(result) - 1 &&
              result.state == AGENTIC_MODEL_STEP_V1_RUNNING &&
              result.epoch_time == 77 && result.epoch_delta == 88 &&
              result.reserved == 99,
          32);
  REQUIRE(read_error(api, model, error, sizeof(error)) &&
              contains(error, "\"code\":\"abi_mismatch\"") &&
              contains(error, "\"phase\":\"api\""),
          33);

  for (index = 1; index <= 3; ++index) {
    result.struct_size = sizeof(result);
    REQUIRE(api->step(model, &result) == AGENTIC_MODEL_STATUS_V1_OK, 34);
    REQUIRE(result.epoch_time == index, 35);
    REQUIRE(result.state == (index == 3 ? AGENTIC_MODEL_STEP_V1_TERMINATED
                                        : AGENTIC_MODEL_STEP_V1_RUNNING),
            36);
    REQUIRE(result.epoch_delta == 0 && result.reserved == 0, 37);
  }
  REQUIRE(read_statistics(api, model, statistics, sizeof(statistics)), 38);
  REQUIRE(contains(statistics, "\"name\":\"cycles\"") &&
              has_stat(statistics, "cycles", "@runtime", 3) &&
              has_stat(statistics, "stop_reason", "@runtime", 1),
          39);
  REQUIRE(has_stat(statistics, "count", "root.left", 3) &&
              has_stat(statistics, "count", "root.right", 10),
          40);
  REQUIRE(read_statistics(api, model, replay_statistics,
                          sizeof(replay_statistics)) &&
              strcmp(statistics, replay_statistics) == 0,
          41);

  REQUIRE(api->reset(model) == AGENTIC_MODEL_STATUS_V1_OK, 42);
  REQUIRE(read_statistics(api, model, replay_statistics,
                          sizeof(replay_statistics)) &&
              has_stat(replay_statistics, "cycles", "@runtime", 0) &&
              has_stat(replay_statistics, "stop_reason", "@runtime", 0),
          43);
  REQUIRE(read_error(api, model, error, sizeof(error)) && error[0] == '\0', 44);

  for (index = 1; index <= 3; ++index) {
    result.struct_size = sizeof(result);
    REQUIRE(api->step(model, &result) == AGENTIC_MODEL_STATUS_V1_OK, 45);
    REQUIRE(result.state == (index == 3 ? AGENTIC_MODEL_STEP_V1_TERMINATED
                                        : AGENTIC_MODEL_STEP_V1_RUNNING) &&
                result.epoch_time == index,
            46);
  }
  REQUIRE(read_statistics(api, model, replay_statistics,
                          sizeof(replay_statistics)) &&
              strcmp(statistics, replay_statistics) == 0,
          47);

  /* A second handle has its own limit, epoch, and statistics snapshot. */
  AgenticModelV1 *other = NULL;
  REQUIRE(api->create(&other) == AGENTIC_MODEL_STATUS_V1_OK && other != NULL,
          48);
  REQUIRE(configure(api, other,
                    "{\"deadlock_window\":null,\"max_domain_cycles\":{},"
                    "\"max_ticks\":1,\"schema\":\"agentic-model-config\","
                    "\"version\":\"1\"}") == AGENTIC_MODEL_STATUS_V1_OK,
          49);
  REQUIRE(api->reset(other) == AGENTIC_MODEL_STATUS_V1_OK, 50);
  result.struct_size = sizeof(result);
  REQUIRE(api->step(other, &result) == AGENTIC_MODEL_STATUS_V1_OK &&
              result.state == AGENTIC_MODEL_STEP_V1_TERMINATED &&
              result.epoch_time == 1,
          51);
  REQUIRE(read_statistics(api, model, statistics, sizeof(statistics)) &&
              has_stat(statistics, "cycles", "@runtime", 3),
          52);
  api->destroy(other);

  /* A domain-only limit terminates; an equal tick limit wins the tie. */
  REQUIRE(api->create(&other) == AGENTIC_MODEL_STATUS_V1_OK && other != NULL,
          53);
  REQUIRE(configure(api, other,
                    "{\"deadlock_window\":null,\"max_domain_cycles\":{"
                    "\"default\":1},\"max_ticks\":null,"
                    "\"schema\":\"agentic-model-config\","
                    "\"version\":\"1\"}") == AGENTIC_MODEL_STATUS_V1_OK,
          54);
  REQUIRE(api->reset(other) == AGENTIC_MODEL_STATUS_V1_OK, 55);
  result.struct_size = sizeof(result);
  REQUIRE(api->step(other, &result) == AGENTIC_MODEL_STATUS_V1_OK &&
              result.state == AGENTIC_MODEL_STEP_V1_TERMINATED &&
              result.epoch_time == 1,
          56);
  REQUIRE(read_statistics(api, other, statistics, sizeof(statistics)) &&
              has_stat(statistics, "stop_reason", "@runtime", 2),
          57);
  api->destroy(other);

  REQUIRE(api->create(&other) == AGENTIC_MODEL_STATUS_V1_OK && other != NULL,
          58);
  REQUIRE(configure(api, other,
                    "{\"deadlock_window\":null,\"max_domain_cycles\":{"
                    "\"default\":1},\"max_ticks\":1,"
                    "\"schema\":\"agentic-model-config\","
                    "\"version\":\"1\"}") == AGENTIC_MODEL_STATUS_V1_OK,
          59);
  REQUIRE(api->reset(other) == AGENTIC_MODEL_STATUS_V1_OK, 60);
  result.struct_size = sizeof(result);
  REQUIRE(api->step(other, &result) == AGENTIC_MODEL_STATUS_V1_OK &&
              result.state == AGENTIC_MODEL_STEP_V1_TERMINATED &&
              result.epoch_time == 1,
          61);
  REQUIRE(read_statistics(api, other, statistics, sizeof(statistics)) &&
              has_stat(statistics, "stop_reason", "@runtime", 1),
          62);
  api->destroy(other);
  return 0;
}

static int run_hold_model(const AgenticModelApiV1 *api, AgenticModelV1 *model) {
  AgenticModelStepResultV1 result = {sizeof(AgenticModelStepResultV1), 0, 0, 0,
                                     0};
  char statistics[4096];
  unsigned index;
  for (index = 1; index <= 3; ++index) {
    REQUIRE(api->step(model, &result) == AGENTIC_MODEL_STATUS_V1_OK, 60);
    REQUIRE(result.state == (index == 3 ? AGENTIC_MODEL_STEP_V1_TERMINATED
                                        : AGENTIC_MODEL_STEP_V1_RUNNING) &&
                result.epoch_time == index,
            61);
  }
  REQUIRE(read_statistics(api, model, statistics, sizeof(statistics)) &&
              has_stat(statistics, "cycles", "@runtime", 3) &&
              has_stat(statistics, "stop_reason", "@runtime", 1),
          62);
  return 0;
}

static int run_zero_rule_model(const AgenticModelApiV1 *api,
                               AgenticModelV1 *model) {
  AgenticModelStepResultV1 result = {sizeof(AgenticModelStepResultV1), 0, 0, 0,
                                     0};
  char statistics[4096];
  unsigned index;
  for (index = 0; index != 2; ++index) {
    REQUIRE(api->step(model, &result) == AGENTIC_MODEL_STATUS_V1_OK, 70);
    REQUIRE(result.state == AGENTIC_MODEL_STEP_V1_QUIESCENT &&
                result.epoch_time == 0,
            71);
  }
  REQUIRE(read_statistics(api, model, statistics, sizeof(statistics)) &&
              has_stat(statistics, "cycles", "@runtime", 0) &&
              has_stat(statistics, "stop_reason", "@runtime", 0),
          72);
  return 0;
}

static int run_failed_model(const AgenticModelApiV1 *api,
                            AgenticModelV1 *model) {
  AgenticModelStepResultV1 result = {sizeof(AgenticModelStepResultV1), 0, 0, 0,
                                     0};
  AgenticModelStepResultV1 refused = {sizeof(AgenticModelStepResultV1),
                                      AGENTIC_MODEL_STEP_V1_RUNNING, 77, 88,
                                      99};
  char first_error[4096];
  char second_error[4096];
  char statistics[4096];
  unsigned attempt;
  for (attempt = 0; attempt != 2; ++attempt) {
    result.struct_size = sizeof(result);
    REQUIRE(api->step(model, &result) == AGENTIC_MODEL_STATUS_V1_OK &&
                result.state == AGENTIC_MODEL_STEP_V1_RUNNING &&
                result.epoch_time == 1,
            80);
    result.struct_size = sizeof(result);
    REQUIRE(api->step(model, &result) ==
                    AGENTIC_MODEL_STATUS_V1_RUNTIME_FAILURE &&
                result.state == AGENTIC_MODEL_STEP_V1_FAILED &&
                result.epoch_time == 1,
            81);
    REQUIRE(read_error(api, model, attempt == 0 ? first_error : second_error,
                       sizeof(first_error)) &&
                contains(attempt == 0 ? first_error : second_error,
                         "\"code\":\"source_check_failed\"") &&
                contains(attempt == 0 ? first_error : second_error,
                         "\"phase\":\"check\""),
            82);
    REQUIRE(read_statistics(api, model, statistics, sizeof(statistics)) &&
                has_stat(statistics, "cycles", "@runtime", 1) &&
                has_stat(statistics, "stop_reason", "@runtime", 0) &&
                has_stat(statistics, "last_successful_count", "root", 0),
            83);
    REQUIRE(api->step(model, &refused) ==
                    AGENTIC_MODEL_STATUS_V1_INVALID_STATE &&
                refused.state == AGENTIC_MODEL_STEP_V1_RUNNING &&
                refused.epoch_time == 77 && refused.epoch_delta == 88 &&
                refused.reserved == 99,
            84);
    REQUIRE(read_error(api, model, statistics, sizeof(statistics)) &&
                strcmp(statistics, attempt == 0 ? first_error : second_error) ==
                    0,
            85);
    if (attempt == 0) {
      REQUIRE(api->reset(model) == AGENTIC_MODEL_STATUS_V1_OK, 86);
      REQUIRE(read_error(api, model, statistics, sizeof(statistics)) &&
                  statistics[0] == '\0',
              87);
    }
  }
  REQUIRE(strcmp(first_error, second_error) == 0, 88);
  return 0;
}

int main(int argc, char **argv) {
  const AgenticModelApiV1 *api = agentic_model_query_v1();
  AgenticModelV1 *model = NULL;
  int status;
  if (!check_common_table(api))
    return 1;
  status = check_retryable_creation_and_config(api, &model);
  if (status != 0)
    return status;
  if (argc == 1 || strcmp(argv[1], "design") == 0)
    status = run_active_model(api, model);
  else if (strcmp(argv[1], "hold") == 0)
    status = run_hold_model(api, model);
  else if (strcmp(argv[1], "zero") == 0)
    status = run_zero_rule_model(api, model);
  else if (strcmp(argv[1], "failure") == 0)
    status = run_failed_model(api, model);
  else
    status = 90;
  api->destroy(model);
  return status;
}
