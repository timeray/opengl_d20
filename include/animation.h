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


void getDiceRollQuaternion(int, versor);
void getRandomRollQuaternion(versor);
void getIdleAnimationQuaternion(float, float, versor);
float getRollAngleDeltaRad(AnimationSettings*);


RollAnimationState initRollAnimationState(size_t);
void deleteRollAnimationState(RollAnimationState*);
void resetRollAnimationState(RollAnimationState*);
void fillRollAnimationQueue(RollAnimationState*, AnimationSettings*, size_t);
void getRollAnimationQuaternion(float, AnimationSettings*, RollAnimationState*, versor);