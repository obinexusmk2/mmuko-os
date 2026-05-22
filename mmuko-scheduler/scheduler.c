/*
 * scheduler.c
 * Cross-platform task scheduler with polar space/time telemetry.
 *
 * Usage:
 *   scheduler.exe --program hello.exe --interval 5s --repeat 10
 *
 * OBINexus Constitutional Computing Framework
 * Time allocation and scheduling for executable programs.
 */

#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#define PROGRAM_PATH_MAX 512
#define NSIGII_TAG_MAX 64
#define NSIGII_REQUIRED_TAG "NSIGII-HR-VERIFIED"
#define SCHEDULER_E 2.71828182845904523536
#define CONSTRAINT_K_MAX 16.0

/* ============================================================================
 * TIME UTILITIES
 * ============================================================================ */

static uint64_t scheduler_now_ms(void) {
#ifdef _WIN32
    static LARGE_INTEGER frequency = {0};
    static LARGE_INTEGER start_time = {0};

    if (frequency.QuadPart == 0) {
        if (!QueryPerformanceFrequency(&frequency)) {
            return (uint64_t)GetTickCount();
        }
        QueryPerformanceCounter(&start_time);
    }

    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);
    return (uint64_t)(((current_time.QuadPart - start_time.QuadPart) * 1000) /
                      frequency.QuadPart);
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
#endif
}

static void scheduler_sleep_ms(uint64_t ms) {
#ifdef _WIN32
    while (ms > 0) {
        DWORD chunk = (ms > UINT32_MAX) ? UINT32_MAX : (DWORD)ms;
        Sleep(chunk);
        ms -= chunk;
    }
#else
    struct timespec req;
    req.tv_sec = (time_t)(ms / 1000);
    req.tv_nsec = (long)((ms % 1000) * 1000000);

    while (nanosleep(&req, &req) == -1 && errno == EINTR) {
        /* Keep sleeping for the remaining time after interruption. */
    }
#endif
}

static int64_t elapsed_delta_ms(uint64_t actual, uint64_t scheduled) {
    if (actual >= scheduled) {
        return (int64_t)(actual - scheduled);
    }
    return -(int64_t)(scheduled - actual);
}

static uint64_t executable_size_bytes(const char *program_path) {
    struct stat st;
    if (stat(program_path, &st) != 0 || st.st_size < 0) {
        return UINT64_MAX;
    }
    return (uint64_t)st.st_size;
}

/* ============================================================================
 * ARGUMENT PARSING
 * ============================================================================ */

static const char *skip_spaces(const char *s) {
    while (*s != '\0' && isspace((unsigned char)*s)) {
        s++;
    }
    return s;
}

static int text_contains_ci(const char *text, const char *needle) {
    size_t needle_len = strlen(needle);
    if (needle_len == 0) {
        return 1;
    }

    for (const char *p = text; *p != '\0'; p++) {
        size_t i = 0;
        while (i < needle_len && p[i] != '\0' &&
               tolower((unsigned char)p[i]) ==
                   tolower((unsigned char)needle[i])) {
            i++;
        }
        if (i == needle_len) {
            return 1;
        }
    }
    return 0;
}

static int string_eq_ci(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static int parse_symbolic_seconds(const char *expr, double *out_value,
                                  const char **out_unit) {
    const char *p = skip_spaces(expr);
    char *end = NULL;
    double exponent = 1.0;

    if (strncmp(p, "exp(", 4) == 0) {
        p += 4;
        errno = 0;
        exponent = strtod(p, &end);
        if (errno == ERANGE || end == p) {
            return 0;
        }
        p = skip_spaces(end);
        if (*p != ')') {
            return 0;
        }
        *out_value = exp(exponent);
        *out_unit = p + 1;
        return 1;
    }

    if (*p != 'e' && *p != 'E') {
        return 0;
    }

    p++;
    if (*p == '\0' || isalpha((unsigned char)*p)) {
        *out_value = SCHEDULER_E;
        *out_unit = p;
        return 1;
    }

    if (*p == '^') {
        p++;
    } else if (p[0] == '*' && p[1] == '*') {
        p += 2;
    } else {
        return 0;
    }

    errno = 0;
    exponent = strtod(p, &end);
    if (errno == ERANGE || end == p) {
        return 0;
    }

    *out_value = pow(SCHEDULER_E, exponent);
    *out_unit = end;
    return 1;
}

static uint64_t seconds_to_ms(double seconds) {
    double ms;

    if (!isfinite(seconds) || seconds <= 0.0) {
        return 0;
    }

    ms = seconds * 1000.0;
    if (!isfinite(ms) || ms > (double)UINT64_MAX) {
        return 0;
    }

    if (ms < 1.0) {
        return 1;
    }

    return (uint64_t)ceil(ms);
}

static uint64_t parse_interval(const char *interval_str) {
    const char *unit = NULL;
    char *numeric_unit = NULL;
    double value = 0.0;
    int symbolic = 0;

    if (text_contains_ci(interval_str, "inf") ||
        text_contains_ci(interval_str, "nan")) {
        fprintf(stderr,
                "ERROR: NSIGII consensus rejects non-finite interval values.\n");
        return 0;
    }

    errno = 0;

    symbolic = parse_symbolic_seconds(interval_str, &value, &unit);
    if (!symbolic) {
        value = strtod(interval_str, &numeric_unit);
        unit = numeric_unit;
        if (errno == ERANGE || numeric_unit == interval_str) {
            fprintf(stderr,
                    "ERROR: Invalid interval format. Use: 5s, 500ms, 2m, e^-1\n");
            return 0;
        }
    }

    unit = skip_spaces(unit);

    if (strcmp(unit, "ms") == 0) {
        if (!isfinite(value) || value <= 0.0 || value > (double)UINT64_MAX) {
            return 0;
        }
        return (value < 1.0) ? 1 : (uint64_t)ceil(value);
    }
    if (strcmp(unit, "s") == 0) {
        return seconds_to_ms(value);
    }
    if (strcmp(unit, "m") == 0) {
        if (!isfinite(value) || value <= 0.0 ||
            value > (double)UINT64_MAX / 60000.0) {
            return 0;
        }
        return (uint64_t)ceil(value * 60000.0);
    }
    if (*unit == '\0' && symbolic) {
        return seconds_to_ms(value);
    }

    if (*unit == '\0') {
        fprintf(stderr,
                "ERROR: Numeric intervals require a unit. Use: 5s, 500ms, 2m\n");
    } else {
        fprintf(stderr, "ERROR: Unknown time unit: %s\n", unit);
    }
    return 0;
}

static uint32_t parse_repeat_count(const char *repeat_str) {
    char *end = NULL;
    unsigned long value;

    if (text_contains_ci(repeat_str, "inf") ||
        text_contains_ci(repeat_str, "nan") ||
        text_contains_ci(repeat_str, "python") ||
        strchr(repeat_str, '-') != NULL) {
        fprintf(stderr,
                "ERROR: NSIGII consensus rejects non-finite, negative, or command-based repeat values.\n");
        return 0;
    }

    errno = 0;
    value = strtoul(repeat_str, &end, 10);
    if (errno == ERANGE || end == repeat_str || *end != '\0' || value == 0 ||
        value > UINT32_MAX) {
        fprintf(stderr,
                "ERROR: --repeat must be a finite positive integer.\n");
        return 0;
    }
    return (uint32_t)value;
}

static int parse_finite_double(const char *text, const char *name, double *out) {
    char *end = NULL;
    double value;

    if (text_contains_ci(text, "inf") || text_contains_ci(text, "nan")) {
        fprintf(stderr, "ERROR: %s must be finite.\n", name);
        return 0;
    }

    errno = 0;
    value = strtod(text, &end);
    if (errno == ERANGE || end == text || *skip_spaces(end) != '\0' ||
        !isfinite(value)) {
        fprintf(stderr, "ERROR: %s must be a finite numeric value.\n", name);
        return 0;
    }

    *out = value;
    return 1;
}

/* ============================================================================
 * PROCESS EXECUTION
 * ============================================================================ */

#ifdef _WIN32
static int run_child_process(const char *program, int *exit_code) {
    STARTUPINFOA startup_info;
    PROCESS_INFORMATION process_info;
    char command_line[PROGRAM_PATH_MAX + 4];
    int written;
    DWORD win_exit_code = 0;

    written = snprintf(command_line, sizeof(command_line), "\"%s\"", program);
    if (written < 0 || (size_t)written >= sizeof(command_line)) {
        fprintf(stderr, "[ERROR] Program path is too long for CreateProcessA\n");
        return -1;
    }

    memset(&startup_info, 0, sizeof(startup_info));
    memset(&process_info, 0, sizeof(process_info));
    startup_info.cb = sizeof(startup_info);

    if (!CreateProcessA(NULL, command_line, NULL, NULL, FALSE, 0, NULL, NULL,
                        &startup_info, &process_info)) {
        fprintf(stderr, "[ERROR] CreateProcessA failed for %s (winerr=%lu)\n",
                program, (unsigned long)GetLastError());
        return -1;
    }

    WaitForSingleObject(process_info.hProcess, INFINITE);
    if (!GetExitCodeProcess(process_info.hProcess, &win_exit_code)) {
        fprintf(stderr, "[ERROR] GetExitCodeProcess failed (winerr=%lu)\n",
                (unsigned long)GetLastError());
        win_exit_code = (DWORD)-1;
    }

    CloseHandle(process_info.hThread);
    CloseHandle(process_info.hProcess);

    *exit_code = (int)win_exit_code;
    return 0;
}
#else
static int run_child_process(const char *program, int *exit_code) {
    pid_t pid = fork();
    int status = 0;

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        execlp(program, program, (char *)NULL);
        perror("execlp");
        _exit(EXIT_FAILURE);
    }

    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return -1;
    }

    if (WIFEXITED(status)) {
        *exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        *exit_code = 128 + WTERMSIG(status);
    } else {
        *exit_code = -1;
    }
    return 0;
}
#endif

/* ============================================================================
 * SCHEDULER DATA
 * ============================================================================ */

typedef enum {
    ORIGIN_USER = 0,
    ORIGIN_LEGACY = 1
} packet_origin_t;

typedef enum {
    SEQUENCE_FLASH = 0,
    SEQUENCE_FILTER = 1
} sequence_mode_t;

typedef enum {
    PACKET_EXECUTABLE = 0,
    PACKET_TEXT = 1,
    PACKET_BINARY = 2
} packet_type_t;

typedef struct {
    char program_path[PROGRAM_PATH_MAX];
    uint64_t interval_ms;
    uint64_t maybe_ttl_ms;
    uint32_t repeat_count;
    uint32_t executions_completed;
    uint32_t repair_count;
    uint64_t start_time;
    uint64_t space_payload_bytes;
    uint64_t time_payload_ms;
    double constraint_k;
    double effective_k;
    packet_origin_t origin;
    sequence_mode_t sequence;
    packet_type_t packet_type;
    char nsigii_tag[NSIGII_TAG_MAX];
} scheduler_task_t;

typedef struct {
    uint64_t scheduled_time;
    uint64_t actual_start_time;
    uint64_t actual_end_time;
    int64_t drift_ms;
    int64_t delta_ms;
    double maybe_decay;
    double suffering;
    int repair_routed;
    int exit_code;
} execution_record_t;

static const char *origin_name(packet_origin_t origin) {
    return origin == ORIGIN_LEGACY ? "legacy" : "user";
}

static const char *sequence_name(sequence_mode_t sequence) {
    return sequence == SEQUENCE_FILTER ? "filter" : "flash";
}

static const char *packet_type_name(packet_type_t packet_type) {
    switch (packet_type) {
        case PACKET_TEXT:
            return "text";
        case PACKET_BINARY:
            return "binary";
        case PACKET_EXECUTABLE:
        default:
            return "executable";
    }
}

static int parse_origin(const char *text, packet_origin_t *out) {
    if (string_eq_ci(text, "user")) {
        *out = ORIGIN_USER;
        return 1;
    }
    if (string_eq_ci(text, "legacy")) {
        *out = ORIGIN_LEGACY;
        return 1;
    }
    fprintf(stderr, "ERROR: --origin must be user or legacy.\n");
    return 0;
}

static int parse_sequence(const char *text, sequence_mode_t *out) {
    if (string_eq_ci(text, "flash")) {
        *out = SEQUENCE_FLASH;
        return 1;
    }
    if (string_eq_ci(text, "filter")) {
        *out = SEQUENCE_FILTER;
        return 1;
    }
    fprintf(stderr, "ERROR: --sequence must be flash or filter.\n");
    return 0;
}

static int parse_packet_type(const char *text, packet_type_t *out) {
    if (string_eq_ci(text, "executable")) {
        *out = PACKET_EXECUTABLE;
        return 1;
    }
    if (string_eq_ci(text, "text")) {
        *out = PACKET_TEXT;
        return 1;
    }
    if (string_eq_ci(text, "binary")) {
        *out = PACKET_BINARY;
        return 1;
    }
    fprintf(stderr,
            "ERROR: --packet-type must be executable, text, or binary.\n");
    return 0;
}

static double clamp_constraint_k(double k) {
    if (k < 0.0) {
        return 0.0;
    }
    if (k > CONSTRAINT_K_MAX) {
        return CONSTRAINT_K_MAX;
    }
    return k;
}

static int validate_nsigii_policy(const scheduler_task_t *task) {
    if (!string_eq_ci(task->nsigii_tag, NSIGII_REQUIRED_TAG)) {
        fprintf(stderr,
                "ERROR: NSIGII schema rejected packet: required tag is %s.\n",
                NSIGII_REQUIRED_TAG);
        return 0;
    }

    if (task->packet_type != PACKET_EXECUTABLE) {
        fprintf(stderr,
                "ERROR: Scheduler expects homogeneous executable packets; route %s packets through the rational wheel first.\n",
                packet_type_name(task->packet_type));
        return 0;
    }

    if (task->origin == ORIGIN_LEGACY && task->sequence == SEQUENCE_FLASH) {
        fprintf(stderr,
                "ERROR: Legacy packets are filter-only; flash mutation is forbidden for static banking data.\n");
        return 0;
    }

    return 1;
}

/* ============================================================================
 * SCHEDULER CORE
 * ============================================================================ */

static int execute_program(const scheduler_task_t *task, execution_record_t *record) {
    int exit_code = -1;
    int run_status;
    uint64_t drift_debt = 0;

    record->actual_start_time = scheduler_now_ms();
    record->drift_ms = elapsed_delta_ms(record->actual_start_time,
                                        record->scheduled_time);
    drift_debt = (record->drift_ms > 0) ? (uint64_t)record->drift_ms : 0;
    record->maybe_decay = exp(-((double)drift_debt /
                                (double)task->maybe_ttl_ms));
    record->delta_ms = (int64_t)task->maybe_ttl_ms - (int64_t)drift_debt;
    record->suffering = ((double)drift_debt - (double)task->maybe_ttl_ms) *
                        task->effective_k;

    printf("[NSIGII] MAYBE ttl=%" PRIu64
           " ms, decay=%.6f, delta=%" PRId64
           " ms, suffering=%.2f\n",
           task->maybe_ttl_ms, record->maybe_decay, record->delta_ms,
           record->suffering);

    if (drift_debt > task->maybe_ttl_ms) {
        record->actual_end_time = record->actual_start_time;
        record->exit_code = -2;
        record->repair_routed = 1;
        printf("[REPAIR] Auto-resolve chaos: drift debt=%" PRIu64
               " ms exceeded maybe TTL=%" PRIu64
               " ms; execution routed to enzyme repair queue.\n",
               drift_debt, task->maybe_ttl_ms);
        return record->exit_code;
    }

    printf("[EXEC] Starting: %s at t=%" PRIu64 " ms (drift=%" PRId64 " ms)\n",
           task->program_path, record->actual_start_time, record->drift_ms);
    fflush(stdout);

    run_status = run_child_process(task->program_path, &exit_code);
    record->actual_end_time = scheduler_now_ms();
    record->exit_code = (run_status == 0) ? exit_code : -1;

    printf("[EXEC] Completed: %s (exit code: %d, duration: %" PRIu64 " ms)\n",
           task->program_path, record->exit_code,
           record->actual_end_time - record->actual_start_time);

    return record->exit_code;
}

static void print_polar_header(const scheduler_task_t *task) {
    printf("[POLAR] Space payload: ");
    if (task->space_payload_bytes == UINT64_MAX) {
        printf("unavailable");
    } else {
        printf("%" PRIu64 " bytes", task->space_payload_bytes);
    }
    printf("\n");
    printf("[POLAR] Time payload reservation: %" PRIu64 " ms\n",
           task->time_payload_ms);
    printf("[POLAR] Allocation model: executable space + scheduled time slots\n");
    printf("[SCHEMA] tag=%s origin=%s sequence=%s packet=%s access=%s\n",
           task->nsigii_tag, origin_name(task->origin),
           sequence_name(task->sequence), packet_type_name(task->packet_type),
           task->origin == ORIGIN_LEGACY ? "READ_ONLY" : "RWX");
    printf("[SUFFERING] K input=%.3f, K effective=%.3f, clamp_max=%.3f\n",
           task->constraint_k, task->effective_k, CONSTRAINT_K_MAX);
    printf("[NSIGII] MAYBE TTL: %" PRIu64
           " ms (finite hold; overdue work decays by exp(-drift/ttl))\n",
           task->maybe_ttl_ms);
    printf("[NSIGII] Consensus: HELD (finite interval=%" PRIu64
           " ms, finite repeat=%" PRIu32 ")\n",
           task->interval_ms, task->repeat_count);
}

static void run_scheduler(scheduler_task_t *task) {
    execution_record_t *records;
    uint64_t total_duration = 0;
    uint64_t max_duration = 0;
    uint64_t min_duration = UINT64_MAX;
    int64_t total_drift = 0;
    int64_t min_drift = INT64_MAX;
    int64_t max_drift = INT64_MIN;

    records = (execution_record_t *)calloc(task->repeat_count,
                                           sizeof(execution_record_t));
    if (records == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        return;
    }

    task->start_time = scheduler_now_ms();

    printf("\n");
    printf("========================================\n");
    printf("  TASK SCHEDULER\n");
    printf("  OBINexus Constitutional Computing\n");
    printf("========================================\n");
    printf("Program:      %s\n", task->program_path);
    printf("Interval:     %" PRIu64 " ms\n", task->interval_ms);
    printf("Repeat count: %" PRIu32 "\n", task->repeat_count);
    printf("Start time:   t=%" PRIu64 " ms\n", task->start_time);
    print_polar_header(task);
    printf("========================================\n\n");
    fflush(stdout);

    for (uint32_t i = 0; i < task->repeat_count; i++) {
        uint64_t scheduled_time = task->start_time + (uint64_t)i * task->interval_ms;
        uint64_t current_time = scheduler_now_ms();
        uint64_t wait_ms = (current_time >= scheduled_time)
                               ? 0
                               : (scheduled_time - current_time);

        if (i > 0 && wait_ms > 0) {
            printf("[SCHEDULER] Waiting %" PRIu64
                   " ms until next execution...\n",
                   wait_ms);
            scheduler_sleep_ms(wait_ms);
        }

        records[i].scheduled_time = scheduled_time;
        execute_program(task, &records[i]);
        if (records[i].repair_routed) {
            task->repair_count++;
        } else {
            task->executions_completed++;
        }

        if (i < task->repeat_count - 1) {
            printf("\n");
        }
    }

    printf("\n");
    printf("========================================\n");
    printf("  SCHEDULER SUMMARY\n");
    printf("========================================\n");
    printf("Total executions: %" PRIu32 "/%" PRIu32 "\n",
           task->executions_completed, task->repeat_count);
    printf("Repair routed:    %" PRIu32 "\n", task->repair_count);
    printf("Total time:       %" PRIu64 " ms\n",
           scheduler_now_ms() - task->start_time);
    printf("Time payload:     %" PRIu64 " ms\n", task->time_payload_ms);
    if (task->space_payload_bytes == UINT64_MAX) {
        printf("Space payload:    unavailable\n\n");
    } else {
        printf("Space payload:    %" PRIu64 " bytes\n\n",
               task->space_payload_bytes);
    }

    for (uint32_t i = 0; i < task->repeat_count; i++) {
        uint64_t duration = records[i].actual_end_time - records[i].actual_start_time;

        total_duration += duration;
        total_drift += records[i].drift_ms;
        if (duration > max_duration) max_duration = duration;
        if (duration < min_duration) min_duration = duration;
        if (records[i].drift_ms > max_drift) max_drift = records[i].drift_ms;
        if (records[i].drift_ms < min_drift) min_drift = records[i].drift_ms;

        printf("Execution #%" PRIu32 ":\n", i + 1);
        printf("  Scheduled: t=%" PRIu64 " ms\n", records[i].scheduled_time);
        printf("  Started:   t=%" PRIu64 " ms\n", records[i].actual_start_time);
        printf("  Completed: t=%" PRIu64 " ms\n", records[i].actual_end_time);
        printf("  Duration:  %" PRIu64 " ms\n", duration);
        printf("  Drift:     %" PRId64 " ms\n", records[i].drift_ms);
        printf("  Delta:     %" PRId64 " ms\n", records[i].delta_ms);
        printf("  Decay:     %.6f\n", records[i].maybe_decay);
        printf("  Suffering: %.2f\n", records[i].suffering);
        printf("  Route:     %s\n",
               records[i].repair_routed ? "REPAIR" : "EXECUTE");
        printf("  Exit code: %d\n\n", records[i].exit_code);
    }

    printf("Execution Statistics:\n");
    printf("  Average duration: %" PRIu64 " ms\n",
           total_duration / task->repeat_count);
    printf("  Min duration:     %" PRIu64 " ms\n", min_duration);
    printf("  Max duration:     %" PRIu64 " ms\n", max_duration);
    printf("  Total work time:  %" PRIu64 " ms\n", total_duration);
    printf("  Average drift:    %" PRId64 " ms\n",
           total_drift / (int64_t)task->repeat_count);
    printf("  Min drift:        %" PRId64 " ms\n", min_drift);
    printf("  Max drift:        %" PRId64 " ms\n", max_drift);
    printf("  Time utilization: %.2f%%\n",
           task->time_payload_ms == 0
               ? 0.0
               : ((double)total_duration * 100.0) /
                     (double)task->time_payload_ms);
    printf("========================================\n\n");

    free(records);
}

/* ============================================================================
 * COMMAND-LINE PARSING
 * ============================================================================ */

static void print_usage(const char *program_name) {
    printf("\n");
    printf("USAGE:\n");
    printf("  %s --program <executable> --interval <time> --repeat <count>\n\n",
           program_name);
    printf("ARGUMENTS:\n");
    printf("  --program <executable>   Path to program to execute\n");
    printf("  --interval <time>        Time between executions (e.g., 5s, 500ms, 2m, e^-1)\n");
    printf("                           Symbolic e expressions are seconds by default.\n");
    printf("  --maybe-ttl <time>       Max MAYBE hold before repair (default: interval)\n");
    printf("  --repeat <count>         Finite positive integer repeat count\n");
    printf("                           inf, -inf, nan, and command execution are rejected.\n\n");
    printf("NSIGII GUARDS:\n");
    printf("  --constraint-k <number>  Suffering coefficient, clamped to %.1f\n",
           CONSTRAINT_K_MAX);
    printf("  --origin <user|legacy>   Packet source; legacy is read-only/filter-only\n");
    printf("  --sequence <flash|filter> Flash mutates, filter only reads/sorts\n");
    printf("  --packet-type <type>     executable, text, or binary; scheduler accepts executable\n");
    printf("  --nsigii-tag <tag>       Required tag: %s\n\n", NSIGII_REQUIRED_TAG);
    printf("EXAMPLES:\n");
    printf("  %s --program hello.exe --interval 5s --repeat 10\n", program_name);
    printf("  %s --program hello.exe --interval e^-1 --repeat 2\n", program_name);
    printf("  %s --program hello.exe --interval e^-1 --repeat 2 --maybe-ttl e^-1 --constraint-k 4\n",
           program_name);
    printf("  %s --program ./test --interval 1s --repeat 5\n", program_name);
    printf("  %s --program /bin/ls --interval 2s --repeat 3\n\n", program_name);
}

int main(int argc, char *argv[]) {
    scheduler_task_t task;
    memset(&task, 0, sizeof(task));

    task.interval_ms = 5000;
    task.maybe_ttl_ms = 0;
    task.repeat_count = 10;
    task.constraint_k = 1.0;
    task.effective_k = 1.0;
    task.origin = ORIGIN_USER;
    task.sequence = SEQUENCE_FLASH;
    task.packet_type = PACKET_EXECUTABLE;
    snprintf(task.nsigii_tag, sizeof(task.nsigii_tag), "%s",
             NSIGII_REQUIRED_TAG);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--program") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: --program requires an argument\n");
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
            snprintf(task.program_path, sizeof(task.program_path), "%s", argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--interval") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: --interval requires an argument\n");
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
            task.interval_ms = parse_interval(argv[i + 1]);
            if (task.interval_ms == 0) {
                return EXIT_FAILURE;
            }
            i++;
        } else if (strcmp(argv[i], "--maybe-ttl") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: --maybe-ttl requires an argument\n");
                return EXIT_FAILURE;
            }
            task.maybe_ttl_ms = parse_interval(argv[i + 1]);
            if (task.maybe_ttl_ms == 0) {
                return EXIT_FAILURE;
            }
            i++;
        } else if (strcmp(argv[i], "--repeat") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: --repeat requires an argument\n");
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
            task.repeat_count = parse_repeat_count(argv[i + 1]);
            if (task.repeat_count == 0) {
                return EXIT_FAILURE;
            }
            i++;
        } else if (strcmp(argv[i], "--constraint-k") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: --constraint-k requires an argument\n");
                return EXIT_FAILURE;
            }
            if (!parse_finite_double(argv[i + 1], "--constraint-k",
                                     &task.constraint_k)) {
                return EXIT_FAILURE;
            }
            i++;
        } else if (strcmp(argv[i], "--origin") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: --origin requires an argument\n");
                return EXIT_FAILURE;
            }
            if (!parse_origin(argv[i + 1], &task.origin)) {
                return EXIT_FAILURE;
            }
            i++;
        } else if (strcmp(argv[i], "--sequence") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: --sequence requires an argument\n");
                return EXIT_FAILURE;
            }
            if (!parse_sequence(argv[i + 1], &task.sequence)) {
                return EXIT_FAILURE;
            }
            i++;
        } else if (strcmp(argv[i], "--packet-type") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: --packet-type requires an argument\n");
                return EXIT_FAILURE;
            }
            if (!parse_packet_type(argv[i + 1], &task.packet_type)) {
                return EXIT_FAILURE;
            }
            i++;
        } else if (strcmp(argv[i], "--nsigii-tag") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "ERROR: --nsigii-tag requires an argument\n");
                return EXIT_FAILURE;
            }
            snprintf(task.nsigii_tag, sizeof(task.nsigii_tag), "%s",
                     argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else {
            fprintf(stderr, "ERROR: Unknown argument: %s\n", argv[i]);
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (task.program_path[0] == '\0') {
        fprintf(stderr, "ERROR: --program is required\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (task.maybe_ttl_ms == 0) {
        task.maybe_ttl_ms = task.interval_ms;
    }
    task.effective_k = clamp_constraint_k(task.constraint_k);

    if (!validate_nsigii_policy(&task)) {
        return EXIT_FAILURE;
    }

    if (task.interval_ms > UINT64_MAX / task.repeat_count) {
        fprintf(stderr, "ERROR: time payload would overflow uint64_t\n");
        return EXIT_FAILURE;
    }

    task.space_payload_bytes = executable_size_bytes(task.program_path);
    task.time_payload_ms = task.interval_ms * task.repeat_count;

    run_scheduler(&task);
    return EXIT_SUCCESS;
}
