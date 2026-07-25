# Vision Gimbal Control Review

## Current control chain

```text
target pixel center
  -> MaixCAM2 angle estimate (40 ms target frame)
  -> UART1 binary frame
  -> TianMengXing visual outer loop
  -> UART3/RS485 speed command
  -> X42S internal encoder closed loop
```

X42S is a closed-loop stepper because its integrated controller uses the motor
encoder. `0x36` returns the encoder-derived real-time shaft angle. It does not
provide an IMU. The TianMengXing application now requests this angle every
100 ms and blocks motion when feedback becomes stale after the startup grace
window. The encoder loop stays inside the motor; it is not replaced by a
software PID on the MCU.

MaixCAM2 does include a six-axis IMU. It is useful for a later chassis-vibration
or camera-rate compensation loop, but it must not be merged blindly into the
current tracker: its physical axes, sign, mounting angle, bias and gyro unit
must first be measured on the real gimbal. The current tracker uses image error
as the pointing reference because that is the only signal that directly says
whether the target is centered.

## 2026-07-25 optimization

The current baseline archive is in the local TianMengXing backup directory.
This revision makes four bounded changes:

1. MaixCAM2 sends at 25 Hz instead of 20 Hz.
2. Pixel displacement uses a pinhole angle conversion with a 90 degree by 51
   degree first-pass lens FOV instead of a 70 degree linear approximation.
3. The TianMengXing processes each new visual frame once. It no longer sends
   multiple commands for the exact same image frame.
4. The yaw controller uses a bounded image-angle-rate feedforward term and a
   55 ms prediction. The existing 6 degree center deadband, 9 degree restart
   deadband and 180 ms settle behavior remain unchanged.

This is a practical visual-servo controller: proportional feedback centers the
target, the bounded rate term reduces lag while the target moves, and the motor
encoder protects the configured motion limits. It cannot be literally zero
delay or zero error: exposure, image processing, UART, motor acceleration and
mechanical compliance always introduce a finite delay. The goal is stable,
small residual error without a static-target oscillation.

## IMU next step

After this revision is tested, run a small MaixCAM2 IMU axis test while moving
the mounted camera left, right, up and down. Record gyro axis, sign, bias and
units. Only then extend the protocol with a gyro-rate frame and add it as a
high-frequency damping/vehicle-vibration compensation input. Do not use
integrated IMU angle alone for target tracking because gyro integration drifts.

Useful official references:

- [MaixCAM2 hardware specifications](https://wiki.sipeed.com/maixcam2) list
  the onboard six-axis IMU and UART4 A21/A22.
- [MaixPy IMU API](https://wiki.sipeed.com/maixpy/api/maix/ext_dev/imu.html)
  defines gyro data, calibration and its degrees-per-second unit.
- [FLO](https://strawlab.org/fast-lock-on/) is an open-source low-latency
  vision tracker. Its architecture reinforces the feedback-plus-bounded-
  feedforward direction used here; its software is not a drop-in driver for
  the MSPM0G3507/X42S protocol.

## Test order

1. Keep the camera wired to the PC while tuning; turn off wireless preview.
2. Test a stationary red target at center, then 10 degrees left/right. Confirm
   there is no continuous hunting.
3. Move the target at a slow constant speed, then stop it. The gimbal should
   brake with at most one small correction.
4. Repeat with the same target farther away. If the angle remains too small,
   calibrate the actual lens horizontal FOV, not the motor gain first.
5. Confirm target loss sends `VISION,LOST` within two capture frames and leaves
   both motors electrically enabled but commanded to zero speed.
