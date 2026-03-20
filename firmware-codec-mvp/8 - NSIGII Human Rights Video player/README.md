# RIFTPlayer — RIFT Symbol Engine
**OBINexus Computing | Symbolic Intent Infrastructure**

> *Tripartite Tristate: YES(1) / NO(0) / MAYBE(-∞→1)*

---

## Overview

RIFTPlayer is the symbolic interpretation runtime for the OBINexus RIFT ecosystem. It implements the **Tripartite Tristate** logic model — a three-state truth system (`YES`, `NO`, `MAYBE`) that grounds all token classification, FilterFlash discrimination, and Trident pipeline verification in a formally defined, machine-verifiable framework.

Unlike binary boolean systems, RIFT's tristate model treats `MAYBE` as a first-class epistemic state: unknown tokens are held in `MAYBE` state rather than forced into a false resolution. This is the computational implementation of the **No-Ghosting Policy** — ambiguity is acknowledged, not discarded.

---

## Project Structure

```
RIFTPlayer/
├── engine/
│   ├── __pycache__/
│   │   └── symbol_engine.cpython-313.pyc
│   └── symbol_engine.py       # Core RIFT Symbol Engine
├── index.html                 # Web interface
├── server.py                  # Flask server (localhost:5000)
└── test_engine.py             # Test suite
```

---

## Core Concepts

### Tripartite Tristate Logic
| State   | Value     | Meaning                          |
|---------|-----------|----------------------------------|
| YES     | 1         | Confirmed / Permission granted   |
| NO      | 0         | Denied / Intent resolved false   |
| MAYBE   | -∞ → 1   | Unknown / Held pending resolution|

### FilterFlash Discriminant
Classifies symbolic intent across three registers:
- **ORDER** — structured, deterministic resolution
- **CONSENSUS** — negotiated, multi-party resolution  
- **CHAOS** — unresolved, requiring further probing

### Trident Pipeline
A 5-symbol verification pipeline that validates symbolic expression trees through dominant-state interpretation. A passing Trident run yields a `dominant` classification with a `conf` (confidence) score.

### No-Ghosting Policy
UNKNOWN tokens are never silently dropped. They are held in `MAYBE` state, ensuring every input receives an explicit tristate classification — even if that classification is "not yet resolved." This policy is fundamental to OBINexus legal and ethical compliance architecture.

---

## Test Suite

Run the full test suite:

```bash
python test_engine.py
```

**Passing tests (all ✓):**
- TriState values correct
- FilterFlash discriminant correct (ORDER / CONSENSUS / CHAOS)
- Token classification correct
- Trident pipeline: 5 symbols verified
- Interpreter: `dominant=ORDER`, `conf=1.000`
- VTT export correct (WAI WebVTT format)
- Symbol table JSON: 5 entries
- No-ghosting policy: UNKNOWN tokens held in MAYBE state

---

## Running the Server

```bash
python server.py
```

Serves the RIFT Symbol Engine at `http://localhost:5000`.

> ⚠️ This is a development server. For production, use a WSGI server (e.g., Gunicorn or uWSGI).

**Debugger PIN:** `268-738-298` *(development only — rotate before any deployment)*

---

## WAI / Accessibility

The VTT export is compliant with **WAI WebVTT format**, supporting accessibility requirements for symbol stream output. This aligns with OBINexus' commitment to inclusive, human-rights-grounded system design.

---

## Toolchain Context

RIFTPlayer sits within the broader OBINexus build pipeline:

```
riftlang.exe → .so.a → rift.exe → gosilang
                                      ↓
                              nlink → polybuild
```

The Symbol Engine is the semantic layer that feeds token classification upstream into the RIFT compiler and downstream into OBIA (Ontological Bayesian Intelligence Architecture) inference pipelines.

---

## License & Policy

This project operates under the **OBINexus Computing Legal Policy Framework**:
- Milestone-based investment model
- `#NoGhosting` compliance in all token handling
- OpenSense recruitment standards for contributors

Unauthorized ghosting of UNKNOWN states — forcing them to YES or NO without resolution — is a policy violation.

---

## Author

**OBINexus Computing**  
`obinexus@OBINexus`  
`/mnt/c/Users/OBINexus/Documents/RIFTPlayer`
