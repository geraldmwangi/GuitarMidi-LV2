import numpy as np
import tensorflow as tf
from collections import Counter
from itertools import combinations

# ================= CONFIG =================
NUM_MIDI_NOTES = 128
SILENCE_IDX = 128          # last index = silence flag
MIN_NOTES = 2
MAX_NOTES = 6
CHORD_STYLE = "short"      # 'short' or 'long', passed to get_chord_suffix
USE_RAW_NAME = True        # True -> group by 'raw_name' (stable internal key, e.g. "minor7")
                           # False -> group by 'suffix' (display string, style-dependent)
# ============================================


def label_to_midi_notes(label_row: np.ndarray) -> list[int]:
    """Convert a 129-dim multi-hot vector into active MIDI note numbers (0-127)."""
    active_idx = np.nonzero(label_row[:NUM_MIDI_NOTES])[0]
    return active_idx.tolist()


def is_silent(label_row: np.ndarray) -> bool:
    return bool(label_row[SILENCE_IDX])


# ---------------------------------------------------------------------
# Precompute pitch-class-set -> suffix lookup table.
# Keyed by (frozenset(pitch_classes), bass_pc) since get_chord_suffix's
# result depends on which note is the bass (root-position vs slash chord
# affects `is_root_position` picking, though NOT the raw suffix itself —
# only slash/inversion labeling). To be fully safe/correct we key on
# (pitch_class_set, bass_pc).
# ---------------------------------------------------------------------

def build_suffix_table(analyzer_cls, style=CHORD_STYLE, use_raw_name=USE_RAW_NAME):
    """
    Precompute suffix for every possible (pitch-class subset, bass pitch class)
    combination, 2-6 notes. Bass must be a member of the subset.
    Returns dict: (frozenset(pcs), bass_pc) -> suffix string (or None).
    """
    table = {}
    pitch_classes = range(12)

    for n in range(MIN_NOTES, MAX_NOTES + 1):
        for combo in combinations(pitch_classes, n):
            for bass_pc in combo:
                # Build a representative voicing: bass note lowest (octave 3),
                # rest in octave 4, to guarantee bass really is the lowest MIDI note.
                other_pcs = [pc for pc in combo if pc != bass_pc]
                midi_notes = [48 + bass_pc] + [60 + pc for pc in other_pcs]

                try:
                    analyzer = analyzer_cls(midi_notes)
                    result = analyzer.get_chord_suffix(style=style)
                    suffix = result['raw_name'] if use_raw_name else result['suffix']
                except Exception:
                    suffix = None

                table[(frozenset(combo), bass_pc)] = suffix

    return table


def midi_notes_to_pcs_and_bass(midi_notes: list[int]):
    sorted_notes = sorted(midi_notes)
    bass_pc = sorted_notes[0] % 12
    pcs = frozenset(n % 12 for n in sorted_notes)
    return pcs, bass_pc


def build_chord_suffix_histogram_fast(
    dataset,
    analyzer_cls,
    style=CHORD_STYLE,
    use_raw_name=USE_RAW_NAME,
    max_batches=None,
):
    print("Building pitch-class -> suffix lookup table (one-time cost)...")
    suffix_table = build_suffix_table(analyzer_cls, style=style, use_raw_name=use_raw_name)
    print(f"Lookup table built: {len(suffix_table)} entries.")

    histogram = Counter()
    total_frames = 0
    skipped_silent = 0
    skipped_note_count = 0
    skipped_unrecognized = 0

    for batch_idx, (audio_path, frame_nr, label) in enumerate(dataset.as_numpy_iterator()):
        if max_batches is not None and batch_idx >= max_batches:
            break

        for row in label:
            total_frames += 1

            if is_silent(row):
                skipped_silent += 1
                continue

            midi_notes = label_to_midi_notes(row)
            n = len(midi_notes)

            if n < MIN_NOTES or n > MAX_NOTES:
                skipped_note_count += 1
                continue

            pcs, bass_pc = midi_notes_to_pcs_and_bass(midi_notes)
            suffix = suffix_table.get((pcs, bass_pc))

            if suffix is None:
                skipped_unrecognized += 1
                continue

            histogram[suffix] += 1

    print(f"Total frames:            {total_frames}")
    print(f"Skipped (silent):        {skipped_silent}")
    print(f"Skipped (note count):    {skipped_note_count}")
    print(f"Skipped (unrecognized):  {skipped_unrecognized}")
    print(f"Chords counted:          {sum(histogram.values())}")

    return histogram


def print_histogram(histogram: Counter, top_n=None):
    total = sum(histogram.values())
    for suffix, count in histogram.most_common(top_n):
        pct = 100 * count / total if total else 0
        label = suffix if suffix is not None else "<none>"
        print(f"{label:15s} {count:8d}  ({pct:5.2f}%)")


# ---------------- USAGE ----------------
# from your_chord_module import GuitarChordAnalyzer
# dataset = create_dataset(filepaths, batch_size=256)
# histogram = build_chord_suffix_histogram_fast(dataset, GuitarChordAnalyzer)
# print_histogram(histogram, top_n=30)