#!/usr/bin/env python3
"""
DRIFT SYSTEM - Predictive Holographic Interface
Version: 7.0.0
Author: OBINexus
Date: 4 March 2026

A tripartite vector system in R³ where:
- G = (E, V, W) is a weighted graph
- Drift is determined by angular velocity relative to camera
- Colors indicate directional state (Red/Orange/Yellow/Green)
- Spline interpolation provides predictive smoothing
"""

import asyncio
import json
import math
import numpy as np
from datetime import datetime
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict
from enum import Enum

class DriftState(Enum):
    """Four-state cabin system"""
    RED = "red"         # Shifting away
    ORANGE = "orange"   # Static
    YELLOW = "yellow"   # Drifting orthogonal
    GREEN = "green"     # Approaching

@dataclass
class Vector3D:
    """3D vector in R³"""
    x: float
    y: float
    z: float = 0.0
    
    def __sub__(self, other):
        return Vector3D(self.x - other.x, self.y - other.y, self.z - other.z)
    
    def __add__(self, other):
        return Vector3D(self.x + other.x, self.y + other.y, self.z + other.z)
    
    def __mul__(self, scalar):
        return Vector3D(self.x * scalar, self.y * scalar, self.z * scalar)
    
    def magnitude(self):
        return math.sqrt(self.x**2 + self.y**2 + self.z**2)
    
    def normalize(self):
        mag = self.magnitude()
        if mag == 0:
            return Vector3D(0, 0, 0)
        return Vector3D(self.x/mag, self.y/mag, self.z/mag)
    
    def dot(self, other):
        return self.x * other.x + self.y * other.y + self.z * other.z
    
    def to_tuple(self):
        return (self.x, self.y, self.z)

@dataclass
class DriftNode:
    """A node in the drift graph with weighted properties"""
    id: str
    position: Vector3D
    velocity: Vector3D
    weight: float  # Graph density/computational cost
    state: DriftState
    timestamp: float
    confidence: float  # Prediction confidence (0-1)
    
    def to_dict(self):
        return {
            "id": self.id,
            "position": {"x": self.position.x, "y": self.position.y, "z": self.position.z},
            "velocity": {"x": self.velocity.x, "y": self.velocity.y, "z": self.velocity.z},
            "weight": self.weight,
            "state": self.state.value,
            "timestamp": self.timestamp,
            "confidence": self.confidence
        }

class SplineInterpolator:
    """
    Quadratic spline for smooth prediction
    P(t) = (1-t)²P₀ + 2(1-t)t·P₁ + t²P₂
    Weighted: 2/3 current + 1/3 predicted
    """
    
    def __init__(self, weight_current: float = 2/3, weight_predicted: float = 1/3):
        self.wc = weight_current
        self.wp = weight_predicted
        self.history: List[Tuple[Vector3D, float]] = []  # (position, timestamp)
        self.max_history = 5
    
    def add_point(self, pos: Vector3D, timestamp: float):
        self.history.append((pos, timestamp))
        if len(self.history) > self.max_history:
            self.history.pop(0)
    
    def predict(self, dt: float = 0.1) -> Tuple[Vector3D, float]:
        """
        Predict future position using weighted spline
        Returns: (predicted_position, confidence)
        """
        if len(self.history) < 2:
            return self.history[-1][0] if self.history else Vector3D(0, 0, 0), 0.0
        
        # Get last two points for linear prediction
        p0, t0 = self.history[-2]
        p1, t1 = self.history[-1]
        
        # Calculate velocity
        time_diff = t1 - t0
        if time_diff == 0:
            return p1, 0.5
        
        velocity = Vector3D(
            (p1.x - p0.x) / time_diff,
            (p1.y - p0.y) / time_diff,
            (p1.z - p0.z) / time_diff
        )
        
        # Linear prediction (P₂ = P₁ + v·dt)
        p2 = Vector3D(
            p1.x + velocity.x * dt,
            p1.y + velocity.y * dt,
            p1.z + velocity.z * dt
        )
        
        # Weighted average: 2/3 current + 1/3 predicted
        predicted = Vector3D(
            self.wc * p1.x + self.wp * p2.x,
            self.wc * p1.y + self.wp * p2.y,
            self.wc * p1.z + self.wp * p2.z
        )
        
        # Confidence based on velocity magnitude and history
        speed = velocity.magnitude()
        confidence = min(1.0, 0.5 + speed * 0.1 + len(self.history) * 0.1)
        
        return predicted, confidence

class DriftGraph:
    """
    Weighted graph G = (E, V, W) for drift computation
    - E: edges (connections between nodes)
    - V: vertices (drift nodes)
    - W: weights (computational cost, density)
    """
    
    def __init__(self):
        self.nodes: Dict[str, DriftNode] = {}
        self.edges: List[Tuple[str, str, float]] = []  # (from, to, weight)
        self.camera_position = Vector3D(0, 0, -1)  # Camera at z=-1 looking at z=0
        self.splines: Dict[str, SplineInterpolator] = {}
        
        # Discriminant parameters for state switching
        self.discriminant_a = 1.0
        self.discriminant_b = 0.0
        self.discriminant_c = -1.0
    
    def add_node(self, node_id: str, position: Vector3D, timestamp: float = None):
        """Add or update a node in the graph"""
        if timestamp is None:
            timestamp = datetime.now().timestamp()
        
        # Calculate velocity if node exists
        velocity = Vector3D(0, 0, 0)
        if node_id in self.nodes:
            old_node = self.nodes[node_id]
            dt = timestamp - old_node.timestamp
            if dt > 0:
                velocity = Vector3D(
                    (position.x - old_node.position.x) / dt,
                    (position.y - old_node.position.y) / dt,
                    (position.z - old_node.position.z) / dt
                )
        
        # Determine drift state based on velocity relative to camera
        state = self._compute_drift_state(position, velocity)
        
        # Compute weight (graph density/computational cost)
        weight = self._compute_weight(position, velocity)
        
        # Create/update node
        node = DriftNode(
            id=node_id,
            position=position,
            velocity=velocity,
            weight=weight,
            state=state,
            timestamp=timestamp,
            confidence=1.0
        )
        
        self.nodes[node_id] = node
        
        # Update spline predictor
        if node_id not in self.splines:
            self.splines[node_id] = SplineInterpolator()
        self.splines[node_id].add_point(position, timestamp)
        
        return node
    
    def _compute_drift_state(self, position: Vector3D, velocity: Vector3D) -> DriftState:
        """
        Compute drift state based on movement direction relative to camera
        
        Camera is at (0, 0, -1) looking toward origin
        - Red: Moving away from camera (positive z velocity)
        - Orange: Static (zero velocity)
        - Yellow: Moving orthogonal to camera (x-y plane)
        - Green: Moving toward camera (negative z velocity)
        """
        speed = velocity.magnitude()
        
        # Static threshold
        if speed < 0.1:
            return DriftState.ORANGE
        
        # Direction from camera to position
        to_camera = Vector3D(0, 0, -1) - position
        to_camera = to_camera.normalize()
        
        # Normalize velocity
        vel_norm = velocity.normalize()
        
        # Dot product gives cosine of angle between velocity and camera direction
        dot = vel_norm.dot(to_camera)
        
        # Determine state based on angle
        # dot = 1: moving directly toward camera (green)
        # dot = -1: moving directly away from camera (red)
        # dot = 0: moving orthogonal (yellow)
        
        if dot > 0.5:  # Within ~60° of camera direction
            return DriftState.GREEN  # Approaching
        elif dot < -0.5:  # Within ~60° of away from camera
            return DriftState.RED  # Shifting away
        else:  # Orthogonal movement
            return DriftState.YELLOW  # Drifting
    
    def _compute_weight(self, position: Vector3D, velocity: Vector3D) -> float:
        """
        Compute graph weight (computational cost)
        Based on: lattice traversal cost + node density + velocity magnitude
        """
        # Base weight from position (distance from origin)
        distance_weight = position.magnitude() * 0.1
        
        # Velocity weight (faster = more expensive to track)
        velocity_weight = velocity.magnitude() * 0.5
        
        # Node density weight (more nodes = higher cost)
        density_weight = len(self.nodes) * 0.01
        
        return distance_weight + velocity_weight + density_weight
    
    def predict_node(self, node_id: str, dt: float = 0.1) -> Optional[DriftNode]:
        """Predict future state of a node using spline interpolation"""
        if node_id not in self.nodes or node_id not in self.splines:
            return None
        
        current = self.nodes[node_id]
        spline = self.splines[node_id]
        
        predicted_pos, confidence = spline.predict(dt)
        
        # Predicted velocity
        predicted_vel = Vector3D(
            (predicted_pos.x - current.position.x) / dt,
            (predicted_pos.y - current.position.y) / dt,
            (predicted_pos.z - current.position.z) / dt
        )
        
        # Predicted state
        predicted_state = self._compute_drift_state(predicted_pos, predicted_vel)
        
        return DriftNode(
            id=f"{node_id}_pred",
            position=predicted_pos,
            velocity=predicted_vel,
            weight=current.weight * 0.9,  # Slightly less confident
            state=predicted_state,
            timestamp=current.timestamp + dt,
            confidence=confidence
        )
    
    def get_discriminant(self) -> float:
        """Compute discriminant Δ = b² - 4ac for state switching"""
        return self.discriminant_b**2 - 4 * self.discriminant_a * self.discriminant_c
    
    def to_dict(self) -> dict:
        """Serialize graph to dictionary"""
        return {
            "nodes": {k: v.to_dict() for k, v in self.nodes.items()},
            "edges": self.edges,
            "discriminant": self.get_discriminant(),
            "camera": {"x": self.camera_position.x, "y": self.camera_position.y, "z": self.camera_position.z},
            "timestamp": datetime.now().timestamp()
        }

class DriftSystem:
    """Main drift system coordinating graph, prediction, and state"""
    
    def __init__(self):
        self.graph = DriftGraph()
        self.running = False
        self.update_interval = 0.05  # 20fps
        
        # Trident channel states (from NSIGII)
        self.channel_states = {
            "transmit": {"state": "order", "rwx": 2},
            "receive": {"state": "chaos", "rwx": 4},
            "verify": {"state": "consensus", "rwx": 7}
        }
    
    def update_cursor(self, x: float, y: float, z: float = 0.0):
        """Update cursor position in the drift graph"""
        pos = Vector3D(x, y, z)
        timestamp = datetime.now().timestamp()
        
        node = self.graph.add_node("cursor", pos, timestamp)
        
        # Update discriminant based on cursor velocity
        speed = node.velocity.magnitude()
        self.graph.discriminant_b = min(4.0, speed * 2)
        
        return node
    
    def get_state(self) -> dict:
        """Get current system state"""
        state = self.graph.to_dict()
        state["channels"] = self.channel_states
        state["predicted"] = {}
        
        # Add predictions for all nodes
        for node_id in self.graph.nodes:
            pred = self.graph.predict_node(node_id, dt=0.1)
            if pred:
                state["predicted"][node_id] = pred.to_dict()
        
        return state

# Global drift system instance
drift_system = DriftSystem()

if __name__ == "__main__":
    # Test the drift system
    print("DRIFT SYSTEM v7.0.0")
    print("=" * 50)
    
    # Simulate cursor movement
    test_positions = [
        (0, 0, 0),      # Static - should be orange
        (0.1, 0, 0),    # Moving right - yellow (orthogonal)
        (0.2, 0, 0.1),  # Moving right and toward - green
        (0.2, 0, -0.1), # Moving right and away - red
        (0.2, 0, 0),    # Static again - orange
    ]
    
    for pos in test_positions:
        node = drift_system.update_cursor(*pos)
        print(f"Position: {pos} -> State: {node.state.value.upper()}")
        print(f"  Velocity: ({node.velocity.x:.3f}, {node.velocity.y:.3f}, {node.velocity.z:.3f})")
        print(f"  Weight: {node.weight:.3f}")
        print()
    
    print("Discriminant Δ:", drift_system.graph.get_discriminant())
    print("Full state:", json.dumps(drift_system.get_state(), indent=2))
