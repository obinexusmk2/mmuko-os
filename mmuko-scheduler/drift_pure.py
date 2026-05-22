"""
MMUKO Fluid Camera - Pure Python Version
No DLL required, works immediately
Date: 2026-03-05 | OBINexus Constitutional Computing
"""

import numpy as np
import math

try:
    import cv2
except ImportError:
    cv2 = None

# PURE PYTHON CLASSIFICATION (no C DLL needed)
def classify_drift(v_toward, v_ortho, threshold=0.5):
    """Python implementation of your C logic"""
    speed_away = -v_toward
    
    # Check orthogonal first (blue priority)
    if abs(v_ortho) > threshold and abs(v_ortho) > abs(v_toward):
        return 1  # BLUE_ORTHOGONAL
    
    # Check away (red)
    if speed_away > threshold:
        return 0  # RED_AWAY
    
    # Check toward (green)
    if v_toward > threshold:
        return 2  # GREEN_APPROACH
    
    # Static
    if abs(v_toward) < threshold and abs(v_ortho) < threshold:
        return 3  # ORANGE_STATIC
    
    return 4  # YELLOW_TRANSITION

def get_color(state, intensity):
    """Get RGB color for state"""
    if state == 0:  # RED_AWAY
        return (255, int(69 * intensity), 0)
    elif state == 1:  # BLUE_ORTHOGONAL
        return (0, int(128 * intensity), 255)
    elif state == 2:  # GREEN_APPROACH
        return (int(100 * intensity), 255, int(50 * intensity))
    elif state == 3:  # ORANGE_STATIC
        return (255, 165, 0)
    else:  # YELLOW_TRANSITION
        return (255, 255, int(100 * intensity))

STATE_NAMES = {
    0: "RED (AWAY)",
    1: "BLUE (ORTHOGONAL)",
    2: "GREEN (APPROACH)",
    3: "ORANGE (STATIC)",
    4: "YELLOW (TRANSITION)"
}

STATE_COLORS_BGR = {
    0: (0, 55, 255),      # Red-Orange
    1: (255, 102, 0),     # Blue-Yellow
    2: (40, 255, 80),     # Green-Yellow-Orange
    3: (0, 165, 255),     # Orange
    4: (0, 255, 255),     # Yellow
}

def _as_vector2(value, name):
    arr = np.asarray(value, dtype=float)
    if arr.shape != (2,):
        raise ValueError(f"{name} must be a 2D vector")
    return arr

def drift_theorem_observation(camera_pos, object_pos, previous_object_pos,
                              dt=1.0, alpha=2.0/3.0,
                              radial_threshold=0.5,
                              angular_threshold=0.01):
    """
    Compute the Drift Theorem observation tuple for a 2D frame.

    C(t) is the observer/camera position, P(t) is the observed centroid,
    V(t) = P(t) - C(t). Radial drift is d||V||/dt and angular drift is
    acos((V(t).V(t-dt))/(|V(t)||V(t-dt)|)) / dt.
    """
    if dt <= 0:
        raise ValueError("dt must be positive")
    if not 0.0 < alpha < 1.0:
        raise ValueError("alpha must be between 0 and 1")

    camera = _as_vector2(camera_pos, "camera_pos")
    current = _as_vector2(object_pos, "object_pos")
    previous = _as_vector2(previous_object_pos, "previous_object_pos")

    relative = current - camera
    previous_relative = previous - camera
    drift_vector = (relative - previous_relative) / dt

    distance = float(np.linalg.norm(relative))
    previous_distance = float(np.linalg.norm(previous_relative))
    radial_drift = (distance - previous_distance) / dt

    theta = 0.0
    angular_drift = 0.0
    if distance > 1e-9 and previous_distance > 1e-9:
        cos_theta = float(
            np.dot(relative, previous_relative) / (distance * previous_distance)
        )
        theta = math.acos(max(-1.0, min(1.0, cos_theta)))
        angular_drift = theta / dt

    weighted = alpha * current + (1.0 - alpha) * previous
    component_states = []
    if radial_drift > radial_threshold:
        component_states.append("separation")
    elif radial_drift < -radial_threshold:
        component_states.append("approach")

    if angular_drift > angular_threshold:
        component_states.append("angular_drift")

    if not component_states:
        component_states.append("stable")

    dominant_state = classify_drift(-radial_drift, angular_drift,
                                    threshold=radial_threshold)

    return {
        "camera": camera,
        "position": current,
        "previous_position": previous,
        "relative_vector": relative,
        "previous_relative_vector": previous_relative,
        "drift_vector": drift_vector,
        "distance": distance,
        "previous_distance": previous_distance,
        "radial_drift": radial_drift,
        "theta": theta,
        "angular_drift": angular_drift,
        "weighted_position": weighted,
        "component_states": component_states,
        "dominant_state": dominant_state,
        "dominant_state_name": STATE_NAMES[dominant_state],
    }

class MMUKOCamera:
    def __init__(self, camera_id=0):
        if cv2 is None:
            raise RuntimeError("OpenCV (cv2) is required for camera capture")
        self.cap = cv2.VideoCapture(camera_id, cv2.CAP_DSHOW)
        if not self.cap.isOpened():
            raise RuntimeError(f"Cannot open camera {camera_id}")
        
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)
        
        self.zoom_level = 1.0
        self.max_zoom = 5.0
        self.prev_gray = None
        
        self.feature_params = dict(
            maxCorners=100, qualityLevel=0.3, minDistance=7, blockSize=7
        )
        self.lk_params = dict(
            winSize=(15, 15), maxLevel=2,
            criteria=(cv2.TERM_CRITERIA_EPS | cv2.TERM_CRITERIA_COUNT, 10, 0.03)
        )
        
        print("MMUKO Camera (Pure Python) initialized")
        print("Controls: [Q]uit | [+/-] Zoom | [R]eset | [S]ave")
    
    def apply_digital_zoom(self, frame, zoom):
        if zoom <= 1.0:
            return frame
        h, w = frame.shape[:2]
        new_w, new_h = int(w / zoom), int(h / zoom)
        x1, y1 = (w - new_w) // 2, (h - new_h) // 2
        cropped = frame[y1:y1+new_h, x1:x1+new_w]
        return cv2.resize(cropped, (w, h), interpolation=cv2.INTER_LINEAR)
    
    def compute_motion(self, frame):
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        
        if self.prev_gray is None:
            self.prev_gray = gray
            return 0.0, 0.0, 0
        
        p0 = cv2.goodFeaturesToTrack(self.prev_gray, mask=None, **self.feature_params)
        if p0 is None:
            self.prev_gray = gray
            return 0.0, 0.0, 0
        
        p1, st, err = cv2.calcOpticalFlowPyrLK(self.prev_gray, gray, p0, None, **self.lk_params)
        if p1 is None or st is None:
            self.prev_gray = gray
            return 0.0, 0.0, 0
        
        good_new = p1[st == 1]
        good_old = p0[st == 1]
        
        if len(good_new) == 0:
            self.prev_gray = gray
            return 0.0, 0.0, 0
        
        motions = good_new - good_old
        avg_motion = np.mean(motions, axis=0)
        dx, dy = avg_motion[0], avg_motion[1]
        
        # Observer at center
        h, w = frame.shape[:2]
        cx, cy = w // 2, h // 2
        
        entity_x = np.mean(good_new[:, 0])
        entity_y = np.mean(good_new[:, 1])
        
        to_obs_x, to_obs_y = cx - entity_x, cy - entity_y
        dist = math.sqrt(to_obs_x**2 + to_obs_y**2)
        
        if dist > 0:
            to_obs_x, to_obs_y = to_obs_x/dist, to_obs_y/dist
        
        v_toward = -(dx * to_obs_x + dy * to_obs_y) * self.zoom_level
        v_ortho = abs(dx * (-to_obs_y) + dy * to_obs_x) * self.zoom_level
        
        self.prev_gray = gray
        return v_toward, v_ortho, len(good_new)
    
    def draw_hud(self, frame, state, v_toward, v_ortho, count):
        h, w = frame.shape[:2]
        color_bgr = STATE_COLORS_BGR.get(state, (128, 128, 128))
        
        # Border
        cv2.rectangle(frame, (0, 0), (w, h), color_bgr, 20)
        
        # Text
        cv2.putText(frame, f"STATE: {STATE_NAMES[state]}", (30, 50),
                   cv2.FONT_HERSHEY_SIMPLEX, 1, color_bgr, 3)
        cv2.putText(frame, f"Toward: {v_toward:+.2f}", (30, 100),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        cv2.putText(frame, f"Ortho:  {v_ortho:+.2f}", (30, 140),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        cv2.putText(frame, f"Features: {count}", (30, 180),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        cv2.putText(frame, f"Zoom: {self.zoom_level:.1f}x", (30, 220),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.7, (255, 255, 255), 2)
        
        # Crosshair
        cv2.drawMarker(frame, (w//2, h//2), (0, 255, 0), cv2.MARKER_CROSS, 40, 2)
        
        # Color swatch
        cv2.rectangle(frame, (w-150, 30), (w-30, 90), color_bgr, -1)
        cv2.rectangle(frame, (w-150, 30), (w-30, 90), (255, 255, 255), 2)
        
        return frame
    
    def run(self):
        print("\nStarting MMUKO Fluid Camera...")
        
        while True:
            ret, frame = self.cap.read()
            if not ret:
                break
            
            display = self.apply_digital_zoom(frame, self.zoom_level)
            v_toward, v_ortho, count = self.compute_motion(display)
            state = classify_drift(v_toward, v_ortho)
            display = self.draw_hud(display, state, v_toward, v_ortho, count)
            
            cv2.imshow('MMUKO Fluid - OBINexus', display)
            
            key = cv2.waitKey(1) & 0xFF
            if key == ord('q') or key == 27:
                break
            elif key == ord('+') or key == ord('='):
                self.zoom_level = min(self.max_zoom, self.zoom_level + 0.5)
                print(f"Zoom: {self.zoom_level:.1f}x")
            elif key == ord('-'):
                self.zoom_level = max(1.0, self.zoom_level - 0.5)
                print(f"Zoom: {self.zoom_level:.1f}x")
            elif key == ord('r'):
                self.zoom_level = 1.0
                print("Zoom reset")
            elif key == ord('s'):
                fn = f"mmuko_{cv2.getTickCount()}.png"
                cv2.imwrite(fn, display)
                print(f"Saved: {fn}")
        
        self.cap.release()
        cv2.destroyAllWindows()
        print("Shutdown")

if __name__ == "__main__":
    try:
        MMUKOCamera(0).run()
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
