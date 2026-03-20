"""
OBINexus RIFT Symbol Engine — Unit Tests
Validates: Tristate, FilterFlash, Trident, Interpretation pipeline
"""

import sys
import os
sys.path.insert(0, os.path.dirname(os.path.dirname(__file__)))

from engine.symbol_engine import (
    TriState, DiscriminantState, FilterFlash,
    TridentChannel, SymbolInterpreter, classify_token
)


def test_tristate_values():
    assert TriState.YES.value == 1
    assert TriState.NO.value == 0
    assert TriState.MAYBE.value == -1
    print("✓ TriState values correct")


def test_filter_flash_discriminant():
    ff = FilterFlash(a=1, b=4, c=1)   # Δ = 16 - 4 = 12 → ORDER
    assert ff.discriminant() == 12.0
    assert ff.context_switch() == DiscriminantState.ORDER

    ff2 = FilterFlash(a=1, b=2, c=1)  # Δ = 4 - 4 = 0 → CONSENSUS
    assert ff2.discriminant() == 0.0
    assert ff2.context_switch() == DiscriminantState.CONSENSUS

    ff3 = FilterFlash(a=1, b=0, c=1)  # Δ = 0 - 4 = -4 → CHAOS
    assert ff3.discriminant() == -4.0
    assert ff3.context_switch() == DiscriminantState.CHAOS

    print("✓ FilterFlash discriminant correct (ORDER/CONSENSUS/CHAOS)")


def test_classify_token():
    assert classify_token("hello") == "IDENTIFIER"
    assert classify_token("42") == "NUMBER"
    assert classify_token("+") == "OPERATOR"
    assert classify_token("???") == "UNKNOWN"
    print("✓ Token classification correct")


def test_trident_pipeline():
    t = TridentChannel()
    raw    = t.transmit("x + 42 = result")
    recv   = t.receive(raw)
    final  = t.verify(recv)

    assert len(final) > 0
    # All verified tokens should have their discriminant state set
    for sym in final:
        assert sym.discriminant_state in list(DiscriminantState)

    # Identifiers should reach ORDER/CONSENSUS and get full rwx
    ids = [s for s in final if s.token_type == "IDENTIFIER"]
    for sym in ids:
        assert sym.rwx_flags == 0b111  # full rwx
        assert sym.tristate == TriState.YES

    print(f"✓ Trident pipeline: {len(final)} symbols verified")


def test_interpreter_full():
    interp = SymbolInterpreter()
    result = interp.interpret("perception calibration tristate x squared y squared t squared")
    
    assert result.frame_index == 0
    assert result.dominant_state in list(DiscriminantState)
    assert result.tristate_summary["YES"] > 0
    assert result.confidence_score >= 0.0
    assert result.accessibility_caption != ""
    assert "[" in result.accessibility_caption   # prefix present
    print(f"✓ Interpreter: dominant={result.dominant_state.value} conf={result.confidence_score:.3f}")


def test_vtt_export():
    interp = SymbolInterpreter()
    interp.interpret("hello world symbolic test")
    interp.interpret("trident verification pipeline order chaos consensus")
    vtt = interp.export_vtt(frame_duration_ms=3000)
    
    assert vtt.startswith("WEBVTT")
    assert "00:00:00.000 --> 00:00:03.000" in vtt
    assert "RIFT" in vtt
    print("✓ VTT export correct (WAI WebVTT format)")


def test_symbol_table_json():
    interp = SymbolInterpreter()
    interp.interpret("NSIGII filter flash bipolar trident")
    table = interp.get_symbol_table_json()
    
    assert isinstance(table, list)
    for entry in table:
        assert "key" in entry
        assert "type" in entry
        assert "tristate" in entry
        assert "state" in entry
        assert "color" in entry
        assert "tag" in entry
    print(f"✓ Symbol table JSON: {len(table)} entries")


def test_no_ghosting_policy():
    """MAYBE state holds indefinitely — no ghosting, no forced resolution"""
    t = TridentChannel()
    raw   = t.transmit("???")  # UNKNOWN token → MAYBE
    recv  = t.receive(raw)
    final = t.verify(recv)
    
    unknowns = [s for s in final if s.token_type == "UNKNOWN"]
    for sym in unknowns:
        # UNKNOWN tokens land in CHAOS (Δ < 0) → MAYBE held or NO — not ghosted
        assert sym.discriminant_state == DiscriminantState.CHAOS
        assert sym.rwx_flags != 0b111  # NOT fully executable — held
    print("✓ No-ghosting policy: UNKNOWN tokens held in MAYBE state")


if __name__ == "__main__":
    print("OBINexus RIFT Symbol Engine — Running Tests\n" + "="*50)
    test_tristate_values()
    test_filter_flash_discriminant()
    test_classify_token()
    test_trident_pipeline()
    test_interpreter_full()
    test_vtt_export()
    test_symbol_table_json()
    test_no_ghosting_policy()
    print("\n" + "="*50)
    print("All tests passed ✓")
