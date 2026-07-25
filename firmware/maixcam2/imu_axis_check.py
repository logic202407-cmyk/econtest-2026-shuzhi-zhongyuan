"""MaixCAM2 onboard-IMU axis check for the camera gimbal.

Keep the camera still during startup, then move the mounted camera one axis at
a time. The console reports the calibrated gyro vector in degrees per second.
Record which component becomes positive for: yaw right, yaw left, pitch up and
pitch down. Do not add gyro compensation to main.py until that mapping is known.
"""

from maix import time
from maix.ext_dev import imu


CALIBRATION_ID = "econtest_gimbal"
PRINT_INTERVAL_MS = 50


def main():
    device = imu.IMU("default", mode=imu.Mode.GYRO_ONLY)

    if not device.calib_gyro_exists(CALIBRATION_ID):
        print("IMU,CALIBRATE,KEEP_STILL,3000MS")
        device.calib_gyro(3000, 10, CALIBRATION_ID)
    else:
        device.load_calib_gyro(CALIBRATION_ID)

    print("IMU,READY,GYRO_UNIT,DEG_PER_S")
    while True:
        data = device.read_all(calib_gryo=True, radian=False)
        gyro = data.gyro
        print("IMU,GYRO,X,{:.2f},Y,{:.2f},Z,{:.2f}".format(
            gyro.x, gyro.y, gyro.z
        ))
        time.sleep_ms(PRINT_INTERVAL_MS)


if __name__ == "__main__":
    main()
