"""
OBINexus RIFT Symbol Analytic Engine
=====================================
Tripartite Tristate Execution: YES(1) / NO(0) / MAYBE(-∞→1)
Filter-Flash Functor: F³(A, B) → (A', A'')
Discriminant Gate: Δ = b² - 4ac → ORDER / CONSENSUS / CHAOS
NSIGII Trident: TRANSMIT → RECEIVE → VERIFY

Architecture: Symbolic AI x² + y² = t² (perception as calibration)
"""

import math
import time
from enum import Enum, IntEnum
from dataclasses import dataclass, field
from typing import Optional, Tuple, List, Dict, Any


# ─── TRISTATE EXECUTION MODEL ────────────────────────────────────────────────

class TriState(Enum):
    """
    Consensual tristate — from OBINexus NSIGII spec:
      YES    = 1       (confirmed, executable)
      NO     = 0       (negated, pruned)
      MAYBE  = -inf→1  (yield-hold, processing, non-ghosting)
    """
    YES   =  1
    NO    =  0
    MAYBE = -1  # represents -∞ hold until resolved

    def __repr__(self):
        return f"TriState.{self.name}({self.value})"


# ─── DISCRIMINANT FLASH STATE ─────────────────────────────────────────────────

class DiscriminantState(Enum):
    ORDER     = "ORDER"       # Δ > 0: two real roots — interpretable
    CONSENSUS = "CONSENSUS"   # Δ = 0: flash point — canonical
    CHAOS     = "CHAOS"       # Δ < 0: complex roots — repair needed


@dataclass
class FilterFlash:
    """
    F³ Filter-Flash Functor
    Δ = b² - 4ac maps content entropy to semantic state
    """
    a: float = 1.0
    b: float = 0.0
    c: float = 1.0

    def discriminant(self) -> float:
        return self.b * self.b - 4.0 * self.a * self.c

    def context_switch(self) -> DiscriminantState:
        d = self.discriminant()
        if d > 0:
            return DiscriminantState.ORDER
        elif d == 0:
            return DiscriminantState.CONSENSUS
        else:
            return DiscriminantState.CHAOS

    def filter(self, A: float, B: float) -> float:
        """F.filter(A, B) = A' — Bayesian refined output"""
        if B == 0:
            return A
        # P(A'|A,B) ∝ A * B / (A + B + ε)
        return (A * B) / (abs(A) + abs(B) + 1e-9)

    def flash(self, A: float, B: float) -> float:
        """F.flash(A, B) = A'' — MAP immediate categorization"""
        # argmax: highest confidence projection
        return A if abs(A) >= abs(B) else B

    def composite(self, A: float, B: float) -> Tuple[float, float]:
        """F.working.flashfilter → (A', A'')"""
        return self.filter(A, B), self.flash(A, B)


# ─── SYMBOL TOKEN ─────────────────────────────────────────────────────────────

@dataclass
class SymbolToken:
    """
    Atomic symbol unit from the NSIGII stream.
    Carries provenance, tristate, discriminant state, and human rights tag.
    """
    key: str
    value: str
    token_type: str                   # IDENTIFIER / NUMBER / OPERATOR / GLYPH / UNKNOWN
    tristate: TriState = TriState.MAYBE
    discriminant_state: DiscriminantState = DiscriminantState.CHAOS
    confidence: float = 0.0
    polarity: str = "+"               # + ORDER  /  - CHAOS
    timestamp: float = field(default_factory=time.time)
    frame_index: int = 0
    human_rights_tag: str = "NSIGII_HR_SYMBOL"
    rwx_flags: int = 0b000            # Read=4 Write=2 Execute=1

    def is_executable(self) -> bool:
        return bool(self.rwx_flags & 0x01)

    def grant_rwx(self):
        self.rwx_flags = 0b111

    def __repr__(self):
        return (f"[{self.token_type}] '{self.key}'='{self.value}' "
                f"| {self.tristate.name} | {self.discriminant_state.value} "
                f"| conf={self.confidence:.3f} | rwx={self.rwx_flags:03b}")


# ─── TRIPARTITE LEXER (Python port of tokenization.c) ─────────────────────────

import re

TOKEN_PATTERNS = {
    "IDENTIFIER": r"^[a-zA-Z_]\w*$",
    "NUMBER":     r"^\d+(\.\d+)?$",
    "OPERATOR":   r"^[+\-*/=<>!&|^~%]$",
    "GLYPH":      r"^[^\x00-\x7F]+$",       # Non-ASCII / symbolic glyphs
    "PUNCTUATION":r"^[.,;:()[\]{}\"]$",
    "WHITESPACE": r"^\s+$",
}

_compiled = {k: re.compile(v) for k, v in TOKEN_PATTERNS.items()}


def classify_token(token: str) -> str:
    """Tripartite lexical classification: known / invalid / user-defined"""
    for token_type, pattern in _compiled.items():
        if pattern.match(token):
            return token_type
    return "UNKNOWN"


# ─── NSIGII TRIDENT CHANNEL ───────────────────────────────────────────────────

class TridentChannel:
    """
    Three-phase symbol pipeline:
      Ch.0 TRANSMIT  — encode / WRITE
      Ch.1 RECEIVE   — decode / READ
      Ch.2 VERIFY    — discriminant gate / EXECUTE
    """

    CHANNEL_TRANSMIT = 0
    CHANNEL_RECEIVE  = 1
    CHANNEL_VERIFY   = 2

    def __init__(self):
        self.ff = FilterFlash()
        self.symbol_table: Dict[str, SymbolToken] = {}
        self.verified_stream: List[SymbolToken] = []

    def transmit(self, raw_text: str, frame_index: int = 0) -> List[SymbolToken]:
        """Ch.0 — tokenize and encode symbols with WRITE permission"""
        tokens = []
        for word in re.split(r'(\s+|[+\-*/=<>!&|^~%.,;:()[\]{}\"]+)', raw_text):
            word = word.strip()
            if not word:
                continue
            t_type = classify_token(word)
            sym = SymbolToken(
                key=word,
                value=word,
                token_type=t_type,
                rwx_flags=0b010,  # WRITE
                frame_index=frame_index,
            )
            tokens.append(sym)
        return tokens

    def receive(self, tokens: List[SymbolToken]) -> List[SymbolToken]:
        """Ch.1 — validate and set READ permission"""
        for sym in tokens:
            # Bipartite order check (sequence parity)
            sym.rwx_flags |= 0b100  # add READ
            if sym.token_type in ("IDENTIFIER", "NUMBER", "GLYPH"):
                sym.tristate = TriState.YES
                sym.polarity = "+"
            elif sym.token_type == "UNKNOWN":
                sym.tristate = TriState.MAYBE
                sym.polarity = "-"
            else:
                sym.tristate = TriState.NO
                sym.polarity = "-"
        return tokens

    def verify(self, tokens: List[SymbolToken]) -> List[SymbolToken]:
        """Ch.2 — discriminant flash gate → grant EXECUTE if ORDER/CONSENSUS"""
        verified = []
        for sym in tokens:
            # Map token confidence to quadratic parameters
            # b = consensus * 4 as per main.go bipartiteConsensus
            # UNKNOWN always → CHAOS regardless of tristate
            if sym.token_type == "UNKNOWN":
                base_conf = 0.0
            else:
                base_conf = 1.0 if sym.tristate == TriState.YES else (
                            0.5 if sym.tristate == TriState.MAYBE else 0.0)
            
            # b = consensus * 4 as per main.go bipartiteConsensus
            self.ff.a = 1.0
            self.ff.b = base_conf * 4.0
            self.ff.c = 1.0
            
            sym.discriminant_state = self.ff.context_switch()
            sym.confidence = base_conf

            if sym.discriminant_state in (DiscriminantState.ORDER,
                                           DiscriminantState.CONSENSUS):
                sym.grant_rwx()   # full rwx
                sym.tristate = TriState.YES
            elif sym.discriminant_state == DiscriminantState.CHAOS:
                # Enzyme repair — still passes but flagged
                sym.rwx_flags = 0b100  # READ only
                # MAYBE holds until user defines
                if sym.tristate != TriState.NO:
                    sym.tristate = TriState.MAYBE

            # Insert into symbol table
            self.symbol_table[sym.key] = sym
            verified.append(sym)

        self.verified_stream.extend(verified)
        return verified


# ─── SYMBOLIC INTERPRETATION LAYER ────────────────────────────────────────────

@dataclass
class InterpretationResult:
    """Output of the symbolic AI interpretation for a frame/segment"""
    frame_index: int
    raw_text: str
    symbols: List[SymbolToken]
    dominant_state: DiscriminantState
    tristate_summary: Dict[str, int]   # counts of YES/NO/MAYBE
    semantic_label: str                # human-readable interpretation
    accessibility_caption: str        # WAI-compliant caption text
    confidence_score: float
    timestamp: float = field(default_factory=time.time)

    def to_vtt_cue(self, start_ms: int, end_ms: int) -> str:
        """Export as WebVTT cue for accessibility"""
        def ms_to_vtt(ms):
            h = ms // 3600000
            m = (ms % 3600000) // 60000
            s = (ms % 60000) // 1000
            ms_r = ms % 1000
            return f"{h:02}:{m:02}:{s:02}.{ms_r:03}"
        return (f"{ms_to_vtt(start_ms)} --> {ms_to_vtt(end_ms)}\n"
                f"<v RIFT>{self.accessibility_caption}\n")


class SymbolInterpreter:
    """
    OBINexus Symbolic Interpretation AI
    
    Perception as Calibration:
      x² + y² = t²  — spatial signal in time
      Bipolar enzyme: CREATE/BUILD/RENEW vs DESTROY/BREAK
      Filter-Flash: unconscious(black) → subconscious(blue) → conscious(green)
    """

    CONSCIOUSNESS_LEVELS = {
        DiscriminantState.ORDER:     ("conscious",    "#00ff88"),
        DiscriminantState.CONSENSUS: ("subconscious", "#4488ff"),
        DiscriminantState.CHAOS:     ("unconscious",  "#333333"),
    }

    def __init__(self):
        self.trident = TridentChannel()
        self.frame_count = 0
        self.interpretation_log: List[InterpretationResult] = []

    def interpret(self, text: str) -> InterpretationResult:
        """Full tripartite interpretation pipeline"""
        fi = self.frame_count
        self.frame_count += 1

        # Three-phase Trident
        raw_tokens   = self.trident.transmit(text, frame_index=fi)
        recv_tokens  = self.trident.receive(raw_tokens)
        final_tokens = self.trident.verify(recv_tokens)

        # Aggregate tristate counts
        counts = {s.name: 0 for s in TriState}
        for sym in final_tokens:
            counts[sym.tristate.name] += 1

        total = len(final_tokens) or 1
        yes_ratio = counts["YES"] / total

        # Dominant discriminant state
        state_counts = {s: 0 for s in DiscriminantState}
        for sym in final_tokens:
            state_counts[sym.discriminant_state] += 1
        dominant = max(state_counts, key=state_counts.get)

        # Confidence score
        avg_conf = sum(s.confidence for s in final_tokens) / total

        # Semantic label from Filter-Flash
        ff = FilterFlash(a=1.0, b=yes_ratio * 4.0, c=1.0)
        filtered, flashed = ff.composite(yes_ratio, avg_conf)
        level_name, _ = self.CONSCIOUSNESS_LEVELS[dominant]

        semantic_label = self._build_semantic_label(
            final_tokens, dominant, level_name, filtered, flashed)

        # WAI accessibility caption
        caption = self._build_caption(text, semantic_label, dominant)

        result = InterpretationResult(
            frame_index=fi,
            raw_text=text,
            symbols=final_tokens,
            dominant_state=dominant,
            tristate_summary=counts,
            semantic_label=semantic_label,
            accessibility_caption=caption,
            confidence_score=avg_conf,
        )
        self.interpretation_log.append(result)
        return result

    def _build_semantic_label(self, tokens, dominant, level, filtered, flashed) -> str:
        identifiers = [s.key for s in tokens if s.token_type == "IDENTIFIER"][:5]
        numbers = [s.key for s in tokens if s.token_type == "NUMBER"][:3]
        ops = [s.key for s in tokens if s.token_type == "OPERATOR"][:3]

        parts = []
        if identifiers:
            parts.append(f"[{level.upper()}] Symbols: {', '.join(identifiers)}")
        if numbers:
            parts.append(f"Values: {', '.join(numbers)}")
        if ops:
            parts.append(f"Operations: {', '.join(ops)}")
        parts.append(f"Δ={dominant.value} filter={filtered:.3f} flash={flashed:.3f}")
        return " | ".join(parts) if parts else f"[{level.upper()}] No classifiable symbols"

    def _build_caption(self, raw: str, label: str, state: DiscriminantState) -> str:
        """WAI-WCAG compliant caption — concise, accurate, no editorial additions"""
        prefix = {
            DiscriminantState.ORDER:     "[ORDER]",
            DiscriminantState.CONSENSUS: "[CONSENSUS]",
            DiscriminantState.CHAOS:     "[CHAOS→REPAIR]",
        }[state]
        # Keep under 32 chars per WAI guideline per line; wrap at sentence
        short_raw = (raw[:80] + "…") if len(raw) > 80 else raw
        return f"{prefix} {short_raw}"

    def export_vtt(self, frame_duration_ms: int = 3000) -> str:
        """Export full interpretation log as WebVTT"""
        lines = ["WEBVTT", ""]
        for result in self.interpretation_log:
            start = result.frame_index * frame_duration_ms
            end   = start + frame_duration_ms
            lines.append(result.to_vtt_cue(start, end))
        return "\n".join(lines)

    def get_symbol_table_json(self) -> List[Dict[str, Any]]:
        """Export symbol table for the RIFT Player overlay"""
        return [
            {
                "key": sym.key,
                "type": sym.token_type,
                "tristate": sym.tristate.name,
                "state": sym.discriminant_state.value,
                "confidence": round(sym.confidence, 3),
                "rwx": f"{sym.rwx_flags:03b}",
                "tag": sym.human_rights_tag,
                "color": self.CONSCIOUSNESS_LEVELS[sym.discriminant_state][1],
            }
            for sym in self.trident.verified_stream[-50:]  # last 50
        ]
