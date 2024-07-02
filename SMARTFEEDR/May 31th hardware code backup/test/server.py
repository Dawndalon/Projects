import pigpio
import time

pi = pigpio.pi()

if not pi.connected:
    print("Failed to connect to pigpiod.")
else:
    print("Connected successfully. Version:", pi.get_pigpio_version())
    pi.set_PWM_dutycycle(13, 0)  # Set PWM to 0 to stop the motor
    time.sleep(1)  # Wait for a second to ensure the motor stops
    pi.stop()
