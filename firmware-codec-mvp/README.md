# **NSIGII LTE Codec**  
*A polygatic, trident‑verified, link‑then‑execute multimedia codec for RIFT‑based systems.*

---

## **Overview**
The **NSIGII LTE Codec** is a next‑generation, *polygatic* video codec designed around the **LTF (Linkable‑Then‑Format/File)** execution model. It integrates:

- **RIFT** (flexible translator + compiler chain)  
- **Trident Channel Architecture** (ORDER → CHAOS → CONSENSUS)  
- **Discriminant Flash Verification** (Δ = b² − 4ac)  
- **Sparse Duplex Encoding (ROPEN)**  
- **RB‑AVL Hybrid Trees** for confidence‑based pruning  
- **Bipolar Enzyme Model** for self‑repair  
- **Quadratic Spline Interpolation** for frame transitions  
- **NSIGII Container Format** (`.nsigii`)

This repository contains the full Go implementation of the codec, including the LTF pipeline, trident channels, and the NSIGII container writer.

---

## **Key Features**

### 🔺 **Trident Architecture (3‑Channel Execution Model)**
Every frame passes through three logical channels:

| Channel | Role | Meaning |
|--------|------|---------|
| **0 – Transmitter** | ORDER | Encoding, ROPEN, symbol binding |
| **1 – Receiver** | CHAOS | Hash verification, polarity checks |
| **2 – Verifier** | CONSENSUS | Discriminant flash verification |

Each channel has its own:

- RB‑AVL tree  
- Flash verifier  
- Enzyme model  
- Loopback address (`127.0.0.1`, `.2`, `.3`)  
- RWX permission model  

---

### ⚡ **Discriminant Flash Verification**
The verifier computes:

\[
\Delta = b^2 - 4ac
\]

Derived from payload entropy + wheel position.

| Δ | State | Meaning |
|---|--------|---------|
| **Δ > 0** | ORDER | Coherent frame |
| **Δ = 0** | CONSENSUS | Flash point (perfect balance) |
| **Δ < 0** | CHAOS | Requires enzyme repair |

This is the codec’s constitutional verification step.

---

### 🔧 **ROPEN Sparse Duplex Encoding (2 → 1)**
Two physical bytes become one logical byte using:

- Polarity conjugation  
- Nibble XOR  
- RB‑AVL insertion with confidence scoring  
- Pruning based on polarity + threshold  

This reduces bandwidth while preserving structural information.

---

### 🧬 **Bipolar Enzyme Model**
The codec uses a biological metaphor for error correction:

- **ORDER sequence:** Create → Build → Renew → Repair  
- **CHAOS sequence:** Destroy → Break → Repair  

Used when Δ < 0.

---

### 🎨 **Quadratic Spline Interpolation**
Smooth transitions between frames using Bézier‑style curves:

\[
P(t) = (1-t)^2P_0 + 2(1-t)tP_1 + t^2P_2
\]

---

### 📦 **NSIGII Container Format**
Each `.nsigii` file begins with:

| Field | Size | Description |
|-------|------|-------------|
| Magic | 8 bytes | `"NSIGII\0\0"` |
| Version | 8 bytes | `"7.0.0"` |
| Width | 4 bytes | Frame width |
| Height | 4 bytes | Frame height |
| FrameCount | 4 bytes | Filled at end |
| Reserved | 4 bytes | Future use |

Each frame is stored as:

```
uint32 size
<DEFLATE-compressed YUV420 frame>
```

---

## **LTF Pipeline (Linkable → Then → Execute)**

The codec is not a traditional ELF binary.  
It follows the **LTF constitutional pipeline**:

1. **LINK**  
   - RIFT → `.so.a` → trident wiring  
   - Symbol binding  
   - Channel polarity resolution  

2. **THEN**  
   - RB‑AVL population  
   - Flash buffer unification  
   - Discriminant verification  

3. **EXECUTE**  
   - `go run main.go`  
   - NSIGII container emission  

This ensures the codec is *structurally valid* before execution.

---

## **Usage**

### **Pipe Mode (LTF)**
```powershell
'.\video.mp4' | go run .\main.go
```

### **Explicit Input**
```bash
go run main.go -input video.mp4
```

### **Raw RGB24 Input**
```bash
go run main.go -input frame.rgb24 -width 1920 -height 1080
```

### **Specify Output**
```bash
go run main.go -input video.mp4 -output out.nsigii
```

---

## **Dependencies**
- **Go 1.20+**
- **FFmpeg** (for RGB24 extraction)
- **FFprobe** (for auto‑detecting dimensions)

---
