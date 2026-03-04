# DRIFT SYSTEM v7.0.0

**Holographic Interface with Four-State Cabin**

## Overview

Drift is a tripartite vector system in R³ where G = (E, V, W) is a weighted graph:
- **E**: Edges (connections between nodes)
- **V**: Vertices (drift nodes/positions)
- **W**: Weights (computational cost, graph density)

The system provides a predictive UI interface that colors elements based on movement direction relative to the camera/viewer.

## Four-State Cabin

| Color | State | Movement | Angle |
|-------|-------|----------|-------|
| **RED** | Shifting | Moving away from camera | > 60° from center |
| **ORANGE** | Static | Stationary | < threshold |
| **YELLOW** | Drifting | Moving orthogonal (90°) to camera | ~90° |
| **GREEN** | Approaching | Moving toward camera | < 60° from center |

## Mathematical Foundation

### Spline Interpolation

Predictive smoothing using weighted quadratic spline:

```
P(t) = (1-t)²P₀ + 2(1-t)t·P₁ + t²P₂

Weighted average: 2/3 current + 1/3 predicted
```

### Discriminant State Switching

```
Δ = b² - 4ac

Δ > 0  → ORDER (green/teal)
Δ = 0  → CONSENSUS (yellow/gold)
Δ < 0  → CHAOS (red)
```

### Graph Weight

```
W = distance_weight + velocity_weight + density_weight
  = |position| × 0.001 + |velocity| × 0.01 + |nodes| × 0.01
```

## Quick Start

### Python Version

```bash
# Install dependencies
pip install -r requirements.txt

# Run the server
python server.py

# Open browser to http://localhost:8000
```

### Go Version

```bash
# Download dependencies
go mod tidy

# Run the server
go run main.go

# Open browser to http://localhost:8080
```

## Usage

1. **Move your mouse** across the screen
2. **Observe the color changes** based on movement direction:
   - Moving toward center → GREEN
   - Moving away from center → RED
   - Moving perpendicular → YELLOW
   - Standing still → ORANGE
3. **Watch the prediction ghost** (dashed circle) showing where the system predicts you'll be
4. **See the velocity vector** pointing in your movement direction

## API Endpoints

### HTTP
- `GET /api/state` - Returns current drift state as JSON

### WebSocket
- `ws://localhost:8080/ws` - Real-time bidirectional communication

### State Format
```json
{
  "nodes": {
    "cursor": {
      "id": "cursor",
      "position": {"x": 400, "y": 300, "z": 0},
      "velocity": {"x": 100, "y": 50, "z": 0},
      "weight": 1.5,
      "state": "green",
      "timestamp": 1234567890,
      "confidence": 0.95
    }
  },
  "predicted": {
    "cursor": {
      "position": {"x": 410, "y": 305, "z": 0},
      "confidence": 0.87
    }
  },
  "discriminant": 2.5,
  "channels": {
    "transmit": {"state": "order", "rwx": "2"},
    "receive": {"state": "chaos", "rwx": "4"},
    "verify": {"state": "consensus", "rwx": "7"}
  }
}
```

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    DRIFT SYSTEM                              │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│   ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    │
│   │   RED       │    │  ORANGE     │    │   GREEN     │    │
│   │  Shifting   │◄──►│   Static    │◄──►│ Approaching │    │
│   │   Away      │    │             │    │             │    │
│   └──────┬──────┘    └──────┬──────┘    └──────┬──────┘    │
│          │                  │                  │            │
│          └──────────────────┼──────────────────┘            │
│                             │                               │
│                        ┌────┴────┐                          │
│                        │ YELLOW  │                          │
│                        │ Drifting│                          │
│                        │(90°)    │                          │
│                        └─────────┘                          │
│                                                              │
│   Camera at (0, 0, -1) looking toward origin                │
│   Center of screen = origin point                           │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Trident Channels (NSIGII Integration)

| Channel | Address | Ratio | State | Permission |
|---------|---------|-------|-------|------------|
| CH0 | 127.0.0.1 | 1/3 | ORDER | WRITE (2) |
| CH1 | 127.0.0.2 | 2/3 | CHAOS | READ (4) |
| CH2 | 127.0.0.3 | 3/3 | CONSENSUS | EXECUTE (7) |

## Inverse Kinematics Analogy

The drift system operates like catching an egg:

1. **Prediction**: The system calculates where the egg (cursor) will be
2. **Smoothing**: Spline interpolation provides gradual deceleration
3. **State awareness**: Different colors indicate different "catch modes"
4. **Ahead-of-time**: The UI prepares before the action completes

## Lattice Operations

The graph supports standard lattice operations:
- **Meet** (∧): Intersection of node sets
- **Join** (∨): Union of node sets
- **Disjoint**: Non-overlapping traversal

## License

MIT - OBINexus 2026

---

*"Structure is a signal. Polarity is a strategy. Drift is the interface."*
