#pragma once
#include <cglm/cglm.h>


typedef struct {
    float idle_rot_speed;  // deg/sec

    // Roll animation settings
    // Number of points should be way more than number of rotations
    // in order to avoid phase wrapping which leads to movement in wrong direction
    size_t n_rotations;
    size_t n_points;
    float max_rot_speed;  // deg/sec
    float min_rot_speed;  // deg/sec
    float deaceleration;  // deg/sec
} AnimationSettings;


typedef struct {
    size_t cur_n;
    versor* q_arr;
    double t;
    versor q_prev;
    float cur_speed_rad_per_sec;
    bool hasFinished;
} RollAnimationState;

// Initialize animation state.
//     roll_points_num - number of steps approximating animation
RollAnimationState initRollAnimationState(size_t roll_points_num);
void deleteRollAnimationState(RollAnimationState* state);

// Get current rotation quaternion for idle animation
void getIdleAnimationQuaternion(float time_delta, float rot_speed_deg, versor q_out);

// Fill roll animation queue in current animation state using target dice value
void fillRollAnimationQueue(RollAnimationState* state, versor initial_rot_quat,
                            const AnimationSettings* settings, size_t dice_value);

// Get current rotation quaternion for idle animation
void getRollAnimationQuaternion(float time_delta, const AnimationSettings* settings,
                                RollAnimationState* state, versor q_out);