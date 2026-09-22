#ifndef H_BALANCE_CONFIG_H
#define H_BALANCE_CONFIG_H

#include "app_config.h"

/* H-problem one-dimensional ball-balance controller.
 *
 * Sign convention (must stay consistent from vision to mechanics):
 *   - ball position is 0 at the lift/motor end and increases toward the
 *     fixed/pivot end;
 *   - positive X42S angle is CW and raises the lift end;
 *   - raising the lift end accelerates the ball in the positive direction.
 */
#ifndef H_BALANCE_FEATURE_ENABLED
#define H_BALANCE_FEATURE_ENABLED                1U
#endif
#ifndef H_BALANCE_MOTION_ENABLED
#define H_BALANCE_MOTION_ENABLED                 1U
#endif
#define H_BALANCE_MOTOR_ID                       X42S_PITCH_MOTOR_ID

/* K1 (PA26) is the explicit arm/disarm key. The rail must be manually level
 * before arming; the first fresh encoder reply is latched as the level zero. */
#define H_BALANCE_ARM_DEBOUNCE_MS                35U
#define H_BALANCE_ARM_RELEASE_MS                 200U
#define H_BALANCE_ARM_TIMEOUT_MS                 1000U

/* Geometry and target, in 0.01 cm. */
#define H_BALANCE_PIPE_MIN_0P01CM                0
#define H_BALANCE_PIPE_MAX_0P01CM                2500
#define H_BALANCE_DEFAULT_TARGET_0P01CM          1250
#define H_BALANCE_TARGET_MIN_0P01CM              100
#define H_BALANCE_TARGET_MAX_0P01CM              2400
#define H_BALANCE_EDGE_GUARD_0P01CM              100

/* Vision admission and estimator timing. */
#define H_BALANCE_CONFIDENCE_MIN_0P01PCT         4500U
#define H_BALANCE_VALID_FRAMES_TO_RUN            3U
#define H_BALANCE_VISION_GRACE_MS                90U
#define H_BALANCE_VISION_TIMEOUT_MS              180U
#define H_BALANCE_CAPTURE_DT_MIN_MS              12U
#define H_BALANCE_CAPTURE_DT_MAX_MS              160U
#define H_BALANCE_ESTIMATOR_ALPHA_NUM            3
#define H_BALANCE_ESTIMATOR_ALPHA_DEN            4
#define H_BALANCE_ESTIMATOR_BETA_NUM             1
#define H_BALANCE_ESTIMATOR_BETA_DEN             4
#define H_BALANCE_VISION_VELOCITY_FUSE_NUM       2
#define H_BALANCE_VISION_VELOCITY_FUSE_DEN       5
#define H_BALANCE_MAX_BALL_SPEED_0P01CM_S        6000

/* Continuous time-optimal velocity profile. Reference speed is the minimum
 * of the speed cap, braking curve sqrt(2*a*distance), and a linear near-target
 * slope. This avoids a discontinuity between a fast and a capture mode. */
#define H_BALANCE_INTEGRAL_BAND_0P01CM           300
#define H_BALANCE_PROFILE_MAX_VEL_0P01CM_S       1600
#define H_BALANCE_PROFILE_BRAKE_ACC_0P01CM_S2    1800
#define H_BALANCE_PROFILE_POS_TO_VEL_NUM         13
#define H_BALANCE_PROFILE_POS_TO_VEL_DEN         10
#define H_BALANCE_PROFILE_VEL_KP                 5
#define H_BALANCE_INTEGRAL_KI_NUM                8
#define H_BALANCE_INTEGRAL_KI_DEN                10000
#define H_BALANCE_INTEGRAL_MAX_0P01CM_MS         800000
#define H_BALANCE_INTEGRAL_ACC_MAX_0P01CM_S2     500
#define H_BALANCE_MAX_ACC_0P01CM_S2              6100

/* A freely rolling solid ball produces about 1.22 cm/s^2 per 0.1 degree near
 * horizontal. This is a feed-forward starting model, not a hidden gain. */
#define H_BALANCE_ACC_PER_0P1DEG_0P01CM_S2       122
#define H_BALANCE_MAX_TILT_0P1DEG                50
#define H_BALANCE_EDGE_TILT_0P1DEG               50
#define H_BALANCE_TILT_SLEW_0P1DEG_PER_UPDATE    5

/* Settling judgment. */
#define H_BALANCE_SETTLE_POS_0P01CM              50
#define H_BALANCE_SETTLE_VEL_0P01CM_S            100
#define H_BALANCE_SETTLE_HOLD_MS                 500U

/* RS485 scheduling and X42S position-mode profile. At most one request is
 * emitted per service pass so an automatic-direction transceiver can turn
 * around cleanly. */
#define H_BALANCE_CONTROL_PERIOD_MS              25U
#define H_BALANCE_MOTOR_READ_PERIOD_MS           50U
#define H_BALANCE_MOTOR_FEEDBACK_TIMEOUT_MS      250U
#define H_BALANCE_BUS_GUARD_MS                   8U
#define H_BALANCE_COMMAND_REFRESH_MS             120U
#define H_BALANCE_COMMAND_HYST_0P1DEG            1
#define H_BALANCE_MOTOR_RANGE_MARGIN_0P1DEG      8
#define H_BALANCE_MOTOR_SPEED_MIN_0P1RPM         20U
#define H_BALANCE_MOTOR_SPEED_MAX_0P1RPM         150U
#define H_BALANCE_MOTOR_SPEED_PER_ERROR          5U
#define H_BALANCE_MOTOR_ACC_RPM_S                200U

#if (H_BALANCE_MOTION_ENABLED != 0U) && (H_BALANCE_FEATURE_ENABLED == 0U)
#error "H_BALANCE_MOTION_ENABLED requires H_BALANCE_FEATURE_ENABLED."
#endif

#if H_BALANCE_TARGET_MIN_0P01CM <= H_BALANCE_PIPE_MIN_0P01CM || \
    H_BALANCE_TARGET_MAX_0P01CM >= H_BALANCE_PIPE_MAX_0P01CM
#error "H-balance target range must stay inside the physical pipe."
#endif

#if H_BALANCE_MAX_TILT_0P1DEG <= 0 || \
    H_BALANCE_TILT_SLEW_0P1DEG_PER_UPDATE <= 0
#error "H-balance tilt limits must be positive."
#endif

#endif
