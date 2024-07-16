#include <glad/gl.h>

#include "animation.h"
#include "icosahedron.h"


void getDiceRollQuaternion(int dice_value, versor q_out) {
    int face_idx = getIcosahedronFaceIndex(dice_value);

    size_t face_vertex_idx = face_idx * 3;
    size_t orientation_vertex_idx = face_vertex_idx + getOrientationVertexIndex(face_idx);

    // Rotate face to positive Z direction
    vec3 positive_z_vec = { 0.0f, 0.0f, 1.0f };
    Vertex first_vertex = gIcosahedronMesh[face_vertex_idx];
    vec3 face_normal = { first_vertex.n[0], first_vertex.n[1], first_vertex.n[2] };

    versor q_rot;
    glm_quat_from_vecs(face_normal, positive_z_vec, q_rot);

    // Correct orientation
    vec3 positive_y_vec = { 0.0f, 1.0f, 0.0f };
    Vertex orientation_vertex = gIcosahedronMesh[orientation_vertex_idx];
    vec3 orient_vec = { orientation_vertex.x, orientation_vertex.y, orientation_vertex.z };
    glm_quat_rotatev(q_rot, orient_vec, orient_vec);
    orient_vec[2] = 0.0f;
    GLfloat orientation_angle = glm_vec3_angle(orient_vec, positive_y_vec);
    if (orient_vec[0] < 0.0f) {
        orientation_angle *= -1;
    }

    // We manually find and specify axis-angle to handle case when orient_vec = -positive_y_vec
    versor q_orient;
    glm_quatv(q_orient, orientation_angle, positive_z_vec);

    // Perform transformations in reverse order
    glm_quat_mul(q_orient, q_rot, q_out);
}


void getRandomRollQuaternion(versor q_out) {
    q_out[0] = (float)rand() / (float)rand();
    q_out[1] = (float)rand() / (float)rand();
    q_out[2] = (float)rand() / (float)rand();
    q_out[3] = 0.0f;
    glm_quat_normalize(q_out);
}


void getIdleAnimationQuaternion(float time_delta, float rot_speed_deg, versor q_out) {
    static float rot_angle_deg = 0.0f;
    rot_angle_deg += rot_speed_deg * time_delta;

    versor q1, q2, q3;
    glm_quatv(q1, glm_rad(rot_angle_deg), (vec3) { 0.0f, 1.0f, 0.0f });
    glm_quatv(q2, glm_rad(rot_angle_deg * 1.5), (vec3) { 0.0f, 0.0f, 1.0f });
    glm_quatv(q3, glm_rad(rot_angle_deg * 1.75), (vec3) { 1.0f, 0.0f, 0.0f });

    glm_quat_mul(q2, q3, q2);
    glm_quat_mul(q1, q2, q_out);
}


float getRollAngleDeltaRad(const AnimationSettings* settings_ptr) {
    return settings_ptr->n_rotations * 2 * M_PI / settings_ptr->n_points;
}


void resetRollAnimationState(RollAnimationState* state_ptr) {
    state_ptr->cur_n = 0;
    state_ptr->t = 0.0;
    glm_quatv(state_ptr->q_prev, 0.0f, (vec3) { 0.0f, 1.0f, 0.0f });
    state_ptr->hasFinished = false;
}


RollAnimationState initRollAnimationState(size_t roll_points_num) {
    RollAnimationState state;    
    state.q_arr = malloc(sizeof(versor) * roll_points_num);
    resetRollAnimationState(&state);
    return state;
}


void deleteRollAnimationState(RollAnimationState* state_ptr) {
    free(state_ptr->q_arr);
}


void fillRollAnimationQueue(RollAnimationState* state_ptr, versor initial_rot_quat,
                            const AnimationSettings* anim_settings_ptr, size_t dice_value) {
    resetRollAnimationState(state_ptr);
    glm_quat_copy(initial_rot_quat, state_ptr->q_prev);

    const size_t n_rotations = anim_settings_ptr->n_rotations;
    const size_t n_points = anim_settings_ptr->n_points;
    const float roll_angle_delta_rad = getRollAngleDeltaRad(anim_settings_ptr);

    getDiceRollQuaternion(dice_value, state_ptr->q_arr[n_points - 1]);

    versor q;
    vec3 axis;
    float angle, added_angle = 0.0f;

    for (size_t n = 0; n < n_points - 1; ++n) {
        float t = (float)n / (n_points - 1);
        glm_quat_slerp(state_ptr->q_prev, state_ptr->q_arr[n_points - 1], t, q);

        added_angle += roll_angle_delta_rad;

        angle = glm_quat_angle(q) + added_angle;
        glm_quat_axis(q, axis);

        glm_quatv(state_ptr->q_arr[n], angle, axis);
    }
}


void getRollAnimationQuaternion(float time_delta, const AnimationSettings* settings_ptr,
    RollAnimationState* state_ptr, versor q_out) {
    const size_t n_points = settings_ptr->n_points;
    const float roll_angle_delta_rad = getRollAngleDeltaRad(settings_ptr);

    // Determine speed
    if (state_ptr->cur_n <= n_points / 2) {
        state_ptr->cur_speed_rad_per_sec = glm_rad(settings_ptr->max_rot_speed);
    } else {
        state_ptr->cur_speed_rad_per_sec = glm_max(
            glm_rad(settings_ptr->min_rot_speed),
            state_ptr->cur_speed_rad_per_sec - glm_rad(settings_ptr->deaceleration) * time_delta
        );
    }

    // Perform n rotations before moving to final position
    if (state_ptr->cur_n < n_points) {
        // ROLL_ANGLE_DELTA_RAD - rotaion angle per one position
        state_ptr->t += time_delta * state_ptr->cur_speed_rad_per_sec / roll_angle_delta_rad;

        // Interpolate frame rotation from previous position to desired position
        glm_quat_slerp(state_ptr->q_prev, state_ptr->q_arr[state_ptr->cur_n],
                       glm_min(state_ptr->t, 1.0), q_out);

        if (state_ptr->t >= 1.0) {
            state_ptr->cur_n += 1;
            state_ptr->t = 0.0f;
            glm_quat_copy(q_out, state_ptr->q_prev);
        }
        state_ptr->hasFinished = false;
    } else {
        glm_quat_copy(state_ptr->q_arr[state_ptr->cur_n - 1], q_out);
        state_ptr->hasFinished = true;
    }
}