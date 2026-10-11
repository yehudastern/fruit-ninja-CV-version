"""
vision_server.py - camera hand tracker for CV Fruit Ninja.

Reads frames from the webcam, finds the hand with Google MediaPipe's Hand
Landmarker model, and sends the position of the index fingertip to the C++
game over UDP.

Message format (one UDP packet per camera frame with a detected hand):
    "<x> <y>"   e.g. "0.5123 0.4410"
x and y are normalized to [0, 1] relative to the camera image
((0, 0) = top-left, (1, 1) = bottom-right). The game converts them to pixels.

Run from inside the vision/ folder (the model path below is relative).
Press 'q' in the video window to quit.
"""

import socket
import time
import cv2
import mediapipe as mp
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

# set up UDP socket for sending fingertip coordinates
# 127.0.0.1 = this computer. Must match the port in NetworkReceiver (main.cpp).
UDP_IP = "127.0.0.1"
UDP_PORT = 5005
# SOCK_DGRAM = UDP: fire-and-forget packets, no connection needed. If the game
# is not running yet, the packets are simply dropped.
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# set up MediaPipe Hand Landmarker
# hand_landmarker.task is the pre-trained model file (in this folder).
base_options = python.BaseOptions(model_asset_path="hand_landmarker.task")
options = vision.HandLandmarkerOptions(
    base_options=base_options,
    num_hands=1,                       # we only need one hand (the "blade")
    min_hand_detection_confidence=0.6, # how sure the model must be to find a NEW hand
    min_tracking_confidence=0.6,       # how sure it must be to keep tracking it
    # VIDEO mode: frames are related, so MediaPipe can track the hand between
    # frames instead of searching the whole image every time (faster, smoother).
    # It requires increasing timestamps, passed to detect_for_video below.
    running_mode=vision.RunningMode.VIDEO
)

# Open the default webcam (device 0) at a low resolution: 640x480 is enough
# for hand tracking and keeps processing fast (= low latency in the game).
cap = cv2.VideoCapture(0)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

print(f"Broadcasting fingertip coordinates to {UDP_IP}:{UDP_PORT}...")

# `with` closes the landmarker automatically when the block ends.
with vision.HandLandmarker.create_from_options(options) as landmarker:
    while cap.isOpened():
        success, frame = cap.read()
        if not success:
            continue  # the camera sometimes drops a frame; just try again

        # Mirror the image horizontally so it behaves like a mirror:
        # moving your hand right moves the blade right.
        frame = cv2.flip(frame, 1)
        h, w, _ = frame.shape  # height, width, color channels

        # OpenCV stores images as BGR, but MediaPipe expects RGB.
        rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)

        # VIDEO mode needs a timestamp in milliseconds for every frame.
        frame_timestamp_ms = int(time.time() * 1000)
        detection_result = landmarker.detect_for_video(mp_image, frame_timestamp_ms)

        if detection_result.hand_landmarks:
            # MediaPipe returns 21 landmarks per hand. Index 8 is the tip of
            # the index finger (0 = wrist, 4 = thumb tip, 12 = middle tip...).
            landmarks = detection_result.hand_landmarks[0]
            index_tip = landmarks[8]

            # send normalized coordinates of the index fingertip over UDP
            # 4 decimal places is far more precision than the screen needs.
            message = f"{index_tip.x:.4f} {index_tip.y:.4f}"
            sock.sendto(message.encode("utf-8"), (UDP_IP, UDP_PORT))

            # draw a circle on the index fingertip in the video frame
            # (converted back from normalized to pixel coordinates; this is
            # only for the preview window, it is not sent to the game)
            cx, cy = int(index_tip.x * w), int(index_tip.y * h)
            cv2.circle(frame, (cx, cy), 10, (0, 255, 0), -1)  # -1 = filled

        # Show the camera preview. waitKey(1) also lets OpenCV process window
        # events; it returns the pressed key code (or -1 if none).
        cv2.imshow("Python Vision Server", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

# Release the camera, close the preview window and the socket.
cap.release()
cv2.destroyAllWindows()
sock.close()
