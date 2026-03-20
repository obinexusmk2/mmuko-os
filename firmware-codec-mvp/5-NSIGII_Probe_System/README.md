# NSIGII Command & Control System

**NSIGII** (November Sierra Indigo Golf India India) - Command and Control Human Rights Verification System

## Overview

NSIGII is a peer-to-peer command and control system built on the **Rectorial Reasoning Rational Wheel Framework**. It implements on-the-fly realtime verification through a **Trident Topology** consisting of three channels:

- **Channel 0 (Transmitter)**: 1 × 1/3 codec - WRITE permission
- **Channel 1 (Receiver)**: 2 × 2/3 codec - READ permission  
- **Channel 2 (Verifier)**: 3 × 3/3 codec - EXECUTE permission

## Core Concepts

### Here and Now Protocol
The system operates in the present moment, tracking past, present, and future states to ensure food, water, and shelter are delivered based on actual need rather than scheduled assumptions.

### LMAC (Loopback MAC Address)
A real-time hardware address tied to physical location:
```
LMAC = f(MAC, lat, lon, time)
```

### Bipartite Consensus
Uses graph theory to model relationships between parties:
```
Δ = coherence² - 4αβ
```
Where:
- α = power_alpha (requester energy)
- β = power_beta (provider energy)
- coherence = √(α × β)

### Trident Voting
Three-party consensus requiring 2/3 majority:
- YES + YES + MAYBE = YES (better safe than sorry)
- NO + NO + MAYBE = NO

## Installation

```bash
pip install -r requirements.txt
```

## Usage

### Start the Server
```bash
python server.py
```

The server will start on `http://localhost:5000`

### Access the Dashboard
Open your browser and navigate to `http://localhost:5000`

## API Endpoints

### System Status
```
GET /api/system/status
```

### Run System Probe
```
GET /api/probe
POST /api/probe {"need": "food"}
```

### Get LMAC Status
```
GET /api/lmac
```

### Submit Trident Vote
```
POST /api/trident/vote
{
  "votes": ["yes", "yes", "maybe"]
}
```

### Request Need (Food/Water/Shelter)
```
POST /api/need/request
{
  "need": "food",
  "location": {
    "lat": 51.5074,
    "lon": -0.1278
  }
}
```

### Update Bipartite State
```
POST /api/bipartite
{
  "power_alpha": 0.7,
  "power_beta": 0.3
}
```

## Keyboard Shortcuts

- **P** - Run system probe
- **F** - Request food
- **W** - Request water
- **S** - Request shelter
- **↑↑↓↓←→←→BA** - Konami code (auxiliary stop)

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    NSIGII SYSTEM v7.0.0                     │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ┌──────────────┐    ┌──────────────┐    ┌──────────────┐ │
│   │ Transmitter  │───▶│  Receiver    │───▶│  Verifier    │ │
│   │  (127.0.0.1) │    │  (127.0.0.2) │    │  (127.0.0.3) │ │
│   │   WRITE      │    │    READ      │    │   EXECUTE    │ │
│   │   0°         │    │   120°       │    │   240°       │ │
│   └──────────────┘    └──────────────┘    └──────────────┘ │
│          │                   │                   │          │
│          └───────────────────┴───────────────────┘          │
│                              │                              │
│                    ┌──────────────┐                         │
│                    │   CONSENSUS  │                         │
│                    │   (360°)     │                         │
│                    └──────────────┘                         │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

## Human Rights Protocol

The system prioritizes three fundamental needs:

1. **Food** - Sustenance and nutrition
2. **Water** - Hydration and sanitation
3. **Shelter** - Safety and protection

These are non-optional human rights delivered through peer-to-peer topology without middleman servers.

## License

OBINexus Computing - Human Rights Born of Blood, Sweat and Tears

---

*"Breathing is never optional. Survival is never optional."*
