/* drift_lib.c - DLL for Python ctypes */
/* Compile: gcc -shared -o drift_lib.dll drift_lib.c -lm */

#include <math.h>
#include <stdint.h>

#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

typedef struct {
    uint8_t r, g, b;
} DriftColor;

typedef enum {
    DRIFT_RED_AWAY = 0,
    DRIFT_BLUE_ORTHOGONAL = 1,
    DRIFT_GREEN_APPROACH = 2,
    DRIFT_ORANGE_STATIC = 3,
    DRIFT_YELLOW_TRANSITION = 4
} DriftState;

DLLEXPORT DriftState classify_drift(float velocity_toward, float velocity_ortho, float threshold) {
    float speed_away = -velocity_toward;
    
    if (fabsf(velocity_ortho) > threshold && fabsf(velocity_ortho) > fabsf(velocity_toward)) {
        return DRIFT_BLUE_ORTHOGONAL;
    }
    if (speed_away > threshold) {
        return DRIFT_RED_AWAY;
    }
    if (velocity_toward > threshold) {
        return DRIFT_GREEN_APPROACH;
    }
    if (fabsf(velocity_toward) < threshold && fabsf(velocity_ortho) < threshold) {
        return DRIFT_ORANGE_STATIC;
    }
    return DRIFT_YELLOW_TRANSITION;
}

DLLEXPORT DriftState classify_drift_components(float radial_drift,
                                               float angular_drift,
                                               float radial_threshold,
                                               float angular_threshold) {
    if (fabsf(radial_drift) <= radial_threshold &&
        fabsf(angular_drift) <= angular_threshold) {
        return DRIFT_ORANGE_STATIC;
    }
    if (fabsf(angular_drift) > angular_threshold) {
        return DRIFT_BLUE_ORTHOGONAL;
    }
    if (radial_drift > radial_threshold) {
        return DRIFT_RED_AWAY;
    }
    if (radial_drift < -radial_threshold) {
        return DRIFT_GREEN_APPROACH;
    }
    return DRIFT_YELLOW_TRANSITION;
}

DLLEXPORT DriftState compute_drift_observation_2d(
    float camera_x, float camera_y,
    float object_x, float object_y,
    float previous_object_x, float previous_object_y,
    float dt,
    float alpha,
    float radial_threshold,
    float angular_threshold,
    float* relative_x,
    float* relative_y,
    float* drift_x,
    float* drift_y,
    float* distance,
    float* radial_drift,
    float* theta,
    float* angular_drift,
    float* weighted_x,
    float* weighted_y) {
    float vx, vy, pvx, pvy;
    float prev_distance;
    float dot;
    float denom;
    float cos_theta;

    if (dt <= 0.0f) {
        dt = 1.0f;
    }
    if (alpha <= 0.0f || alpha >= 1.0f) {
        alpha = 2.0f / 3.0f;
    }

    vx = object_x - camera_x;
    vy = object_y - camera_y;
    pvx = previous_object_x - camera_x;
    pvy = previous_object_y - camera_y;

    *relative_x = vx;
    *relative_y = vy;
    *drift_x = (vx - pvx) / dt;
    *drift_y = (vy - pvy) / dt;
    *distance = sqrtf(vx * vx + vy * vy);
    prev_distance = sqrtf(pvx * pvx + pvy * pvy);
    *radial_drift = (*distance - prev_distance) / dt;

    *theta = 0.0f;
    *angular_drift = 0.0f;
    if (*distance > 1e-6f && prev_distance > 1e-6f) {
        dot = vx * pvx + vy * pvy;
        denom = (*distance) * prev_distance;
        cos_theta = dot / denom;
        if (cos_theta < -1.0f) cos_theta = -1.0f;
        if (cos_theta > 1.0f) cos_theta = 1.0f;
        *theta = acosf(cos_theta);
        *angular_drift = *theta / dt;
    }

    *weighted_x = alpha * object_x + (1.0f - alpha) * previous_object_x;
    *weighted_y = alpha * object_y + (1.0f - alpha) * previous_object_y;

    return classify_drift_components(*radial_drift, *angular_drift,
                                     radial_threshold, angular_threshold);
}

DLLEXPORT void get_color(DriftState state, float intensity, uint8_t* r, uint8_t* g, uint8_t* b) {
    switch(state) {
        case DRIFT_RED_AWAY:
            *r = 255; *g = (uint8_t)(69 * intensity); *b = 0;
            break;
        case DRIFT_BLUE_ORTHOGONAL:
            *r = 0; *g = (uint8_t)(128 * intensity); *b = 255;
            break;
        case DRIFT_GREEN_APPROACH:
            *r = (uint8_t)(100 * intensity); *g = 255; *b = (uint8_t)(50 * intensity);
            break;
        case DRIFT_ORANGE_STATIC:
            *r = 255; *g = 165; *b = 0;
            break;
        case DRIFT_YELLOW_TRANSITION:
            *r = 255; *g = 255; *b = (uint8_t)(100 * intensity);
            break;
        default:
            *r = *g = *b = 128;
    }
}

DLLEXPORT const char* get_state_name(DriftState state) {
    const char* names[] = {"RED_AWAY", "BLUE_ORTHOGONAL", "GREEN_APPROACH", 
                          "ORANGE_STATIC", "YELLOW_TRANSITION"};
    return names[state];
}
