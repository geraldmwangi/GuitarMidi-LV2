import tensorflow as tf
import numpy as np
from common import OUTPUT_DIM_NOTES

# ============================================================
# Static setup — chord formulas, bitmask tables
# ============================================================

CHORD_FORMULAS = {
   # '13':                   [0, 4, 7, 10, 14, 21],
    '9':                    [0, 4, 7, 10, 14],
    'major9':               [0, 4, 7, 11, 14],
    'minor9':               [0, 3, 7, 10, 14],
    '7#9':                  [0, 4, 7, 10, 15],
    '7b9':                  [0, 4, 7, 10, 13],
    'major7':               [0, 4, 7, 11],
    'dominant7':            [0, 4, 7, 10],
    'minor7':               [0, 3, 7, 10],
    'add9':                 [0, 4, 7, 14],
    'minorAdd9':            [0, 3, 7, 14],
    'major6':               [0, 4, 7, 9],
    'minor6':               [0, 3, 7, 9],  
    'diminished7':          [0, 3, 6, 9],
    'minor7b5':             [0, 3, 6, 10],  
    'minorMajor7':          [0, 3, 7, 11],
    'sus4_7':               [0, 5, 7, 10],
    'sus2_7':               [0, 2, 7, 10],
    'minor':                [0, 3, 7],
    'sus4':                 [0, 5, 7],
    'sus2':                 [0, 2, 7],
    'diminished':           [0, 3, 6],
    'augmented':            [0, 4, 8],
    'no3_add4':             [0, 5, 7],
    'major':                [0, 4, 7],
    'power_chord':          [0, 7],
    'single_note':          [0],
    'silent':               [],  # Silence: 129th label = 1, all notes = 0
}

CHORD_NAMES = list(CHORD_FORMULAS.keys())
NUM_CHORDS = len(CHORD_NAMES)
NUM_SUFFIXES = NUM_CHORDS + 1  # +1 for UNRECOGNIZED

SUFFIX_VOCAB = CHORD_NAMES + ['UNRECOGNIZED']
SUFFIX_TO_IDX = {name: i for i, name in enumerate(SUFFIX_VOCAB)}

# Define indices once, at module level
SINGLE_NOTE_IDX = SUFFIX_TO_IDX['single_note']
POWER_CHORD_IDX = SUFFIX_TO_IDX['power_chord']
SILENT_IDX = SUFFIX_TO_IDX['silent']
UNRECOGNIZED_IDX = NUM_SUFFIXES - 1

# The 129th label (index 128) is the silence flag
SILENCE_LABEL_IDX = 128  # Index of the silence label in the 129-dimensional label vector

def _formula_to_mask(formula):
    mask = 0
    for i in formula:
        mask |= (1 << (i % 12))
    return mask

def _rotate_mask(mask, root):
    return ((mask << root) | (mask >> (12 - root))) & 0xFFF

_ROOT_CHORD_MASKS = np.zeros((12, NUM_CHORDS), dtype=np.int32)
for root in range(12):
    for c_idx, name in enumerate(CHORD_NAMES):
        base_mask = _formula_to_mask(CHORD_FORMULAS[name])
        _ROOT_CHORD_MASKS[root, c_idx] = _rotate_mask(base_mask, root)

_CHORD_DICT_ORDER = np.arange(NUM_CHORDS, dtype=np.int32)

ROOT_CHORD_MASKS_TF = tf.constant(_ROOT_CHORD_MASKS, dtype=tf.int32)
CHORD_DICT_ORDER_TF = tf.constant(_CHORD_DICT_ORDER, dtype=tf.int32)

# ============================================================
# Helper functions
# ============================================================

def labels_to_pc_mask(labels):
    """
    Convert note labels (first 128 dimensions) to 12-bit pitch class bitmask.
    Ignores the 129th silence label.
    """
    labels = tf.cast(labels, tf.int32)
    note_idx = tf.range(OUTPUT_DIM_NOTES - 1, dtype=tf.int32)  # 0..127
    pc_idx = note_idx % 12
    pc_onehot = tf.one_hot(pc_idx, depth=12, dtype=tf.int32)
    
    # Extract only the first 128 labels (note labels)
    note_labels = labels[..., :OUTPUT_DIM_NOTES - 1]  # [..., 128]
    active = note_labels[..., :, None] * pc_onehot[None, ...]  # [..., 128, 12]
    pc_present = tf.reduce_max(active, axis=-2)  # [..., 12]
    bit_weights = tf.constant([1 << i for i in range(12)], dtype=tf.int32)
    return tf.reduce_sum(pc_present * bit_weights, axis=-1)

def bass_pc_from_labels(labels):
    """
    Find the lowest MIDI note and return its pitch class (0-11).
    Uses only the first 128 labels (note labels), ignores the 129th silence label.
    """
    labels = tf.cast(labels, tf.int32)
    note_idx = tf.range(OUTPUT_DIM_NOTES - 1, dtype=tf.int32)  # 0..127
    
    # Extract only the first 128 labels (note labels)
    note_labels = labels[..., :OUTPUT_DIM_NOTES - 1]  # [..., 128]
    masked_idx = tf.where(note_labels > 0, note_idx, tf.fill(tf.shape(note_labels), 999))
    lowest_note = tf.reduce_min(masked_idx, axis=-1)
    return lowest_note % 12

def _popcount12(x):
    """Count set bits in 12-bit int32 tensor x."""
    c = tf.zeros_like(x)
    for i in range(12):
        c += tf.bitwise.bitwise_and(tf.bitwise.right_shift(x, i), 1)
    return c

# ============================================================
# Main chord matching function
# ============================================================

# In the priority scoring section, add formula size as a tie-breaker:

# ============================================================
# CHORD PRIORITY for partial matches
# ============================================================

CHORD_PRIORITY = {
    # Highest priority (most common on guitar, simplest formulas)
    'power_chord': 0,
    'single_note': 1,
    'major': 2,
    'minor': 3,
    'sus2': 4,
    'sus4': 5,
    'augmented': 6,
    'diminished': 7,
    'no3_add4': 8,
    
    # Medium: basic 6th chords
    'major6': 9,
    'minor6': 10,
    
    # 7th chords
    'dominant7': 11,
    'major7': 12,
    'minor7': 13,
    'minorMajor7': 14,
    'diminished7': 15,
    'minor7b5': 16,
    
    # sus + 7
    'sus2_7': 17,
    'sus4_7': 18,
    
    # Lowest priority: extended/colored chords (9th, add9, altered)
    'add9': 19,
    'minorAdd9': 20,
    'major9': 21,
    'minor9': 22,
    '9': 23,
    '7#9': 24,
    '7b9': 25,
    # '13' is removed entirely
}

PRIORITY_VECTOR = tf.constant(
    [CHORD_PRIORITY.get(name, 999) for name in CHORD_NAMES],
    dtype=tf.int32
)  # [NUM_CHORDS]

# ============================================================
# Updated chord matching with priority-based partial matching
# ============================================================

def get_chord_suffix_idx_batch(labels):
    """
    Classify each frame in a batch to a chord suffix index.
    
    Matching strategy:
    1. Silence/single-note: immediate classification
    2. For polyphony:
       a. Try exact match at each root (prefer root position, then dictionary order)
       b. If no exact match, try subset matches (prefer root position, then priority, then formula size)
       c. If still no match, classify as UNRECOGNIZED
    """
    labels = tf.cast(labels, tf.int32)
    B = tf.shape(labels)[0]

    # ===== Check silence flag first =====
    silence_flag = labels[:, SILENCE_LABEL_IDX]
    is_silent = tf.equal(silence_flag, 1)

    # ===== Count active notes (first 128 dims only) =====
    note_labels = labels[:, :OUTPUT_DIM_NOTES - 1]
    num_active_notes = tf.reduce_sum(note_labels, axis=-1)
    num_pitch_classes = _popcount12(labels_to_pc_mask(note_labels))

    # Initialize with silent/single_note assignments
    is_single_note = tf.equal(num_active_notes, 1)

    suffix_idx = tf.where(is_silent, 
                          tf.fill([B], SILENT_IDX), 
                          tf.fill([B], UNRECOGNIZED_IDX))
    suffix_idx = tf.where(is_single_note, 
                          tf.fill([B], SINGLE_NOTE_IDX), 
                          suffix_idx)

    # ===== CHORD MATCHING: Only for polyphonic frames (2+ notes, not silent) =====
    is_poly = tf.logical_and(
        tf.logical_not(is_silent), 
        tf.logical_not(is_single_note)
    )

    if tf.executing_eagerly() or tf.reduce_any(is_poly):
        pc_mask = labels_to_pc_mask(note_labels)
        bass_pc = bass_pc_from_labels(note_labels)

        all_masks = ROOT_CHORD_MASKS_TF[None, :, :]  # [1, 12, NUM_CHORDS]
        pc_mask_bbb = pc_mask[:, None, None]         # [B, 1, 1]

        # Match types: exact vs subset
        exact_any = tf.equal(pc_mask_bbb, all_masks)  # [B, 12, NUM_CHORDS]
        subset_any = tf.equal(
            tf.bitwise.bitwise_and(pc_mask_bbb, 
                                  tf.bitwise.invert(all_masks)), 
            0
        )  # pc_mask ⊆ formula_mask

        roots_range = tf.range(12, dtype=tf.int32)
        is_root_position = tf.equal(roots_range[None, :], bass_pc[:, None])
        is_root_position_b = is_root_position[:, :, None]

        # ===== BUILD COMPOSITE SCORE =====
        formula_sizes = tf.constant(
            [len(CHORD_FORMULAS[name]) for name in CHORD_NAMES],
            dtype=tf.int32
        )  # [NUM_CHORDS]
        formula_sizes_b = tf.tile(formula_sizes[None, None, :], [B, 12, 1])

        dict_order_b = tf.tile(
            tf.range(NUM_CHORDS, dtype=tf.int32)[None, None, :],
            [B, 12, 1]
        )

        priority_b = tf.tile(PRIORITY_VECTOR[None, None, :], [B, 12, 1])

        # ===== SCORING TIERS =====
        # Tier 0: Exact match + root position
        # Tier 1: Exact match + inversion
        # Tier 2: Subset match + root position + priority
        # Tier 3: Subset match + inversion + priority
        # Tier 100: No match

        priority_tier = tf.where(
            exact_any,
            # EXACT MATCH: prefer root position
            tf.where(is_root_position_b, 0, 1),
            # SUBSET MATCH: use priority for tie-breaking
            tf.where(subset_any, 
                     tf.where(is_root_position_b, 2, 3),
                     100)  # No match = worst tier
        )

        combined_score = (
            priority_tier * 10000000 +      # Tier 0-100 (primary decision)
            priority_b * 1000 +              # Priority (secondary: lower = better)
            formula_sizes_b * 10 +           # Formula size (tertiary: prefer smaller)
            dict_order_b                     # Dictionary order (last tiebreaker)
        )

        combined_score_flat = tf.reshape(combined_score, [B, 12 * NUM_CHORDS])
        best_flat_idx = tf.argmin(combined_score_flat, axis=-1, output_type=tf.int32)
        best_score = tf.gather(combined_score_flat, best_flat_idx, batch_dims=1)
        best_chord_idx = best_flat_idx % NUM_CHORDS

        # Only accept if within valid tiers (tier < 100)
        poly_suffix_idx = tf.where(
            best_score < 1000000000,  # (100 * 10000000)
            best_chord_idx, 
            tf.fill([B], UNRECOGNIZED_IDX)
        )

        # ===== POWER CHORD OVERRIDE =====
        # If only one pitch class but multiple notes, it's a power chord
        is_octave_unison = tf.logical_and(
            tf.equal(num_pitch_classes, 1),
            tf.greater(num_active_notes, 1)
        )
        poly_suffix_idx = tf.where(is_octave_unison, 
                                   tf.fill([B], POWER_CHORD_IDX), 
                                   poly_suffix_idx)

        # Merge: use poly result only where is_poly=True
        suffix_idx = tf.where(is_poly, poly_suffix_idx, suffix_idx)

    return tf.cast(suffix_idx, tf.int32)

# ============================================================
# Balanced dataset builder
# ============================================================

def create_balanced_dataset_chords(dataset, max_labels, batch_size=256,
                                    max_active_notes=6, num_midi_notes=OUTPUT_DIM_NOTES,
                                    drop_unrecognized=False):
    """
    Caps per-chord-suffix counts using a fully vectorized (tf-graph, no
    py_function) reimplementation of GuitarChordAnalyzer's suffix logic.
    
    Handles the 129-dimensional label vector where:
      - Dimensions 0-127: MIDI note activations
      - Dimension 128: silence flag
    """
    dataset = dataset.unbatch()

    # 1. Polyphony prefilter (per-frame)
    def prefilter(audio, frame_nr, labels):
        # Check silence flag (129th label, index 128)
        silence_flag = labels[SILENCE_LABEL_IDX]
        is_silent = tf.equal(silence_flag, 1)
        
        # Count active notes in the first 128 labels
        note_labels = tf.cast(labels[:OUTPUT_DIM_NOTES - 1], tf.int32)
        num_active = tf.reduce_sum(note_labels)
        
        # Keep: silence (flag=1), single notes (1 active), or polyphony (2..max_active_notes)
        return tf.logical_or(
            is_silent,
            tf.logical_or(
                tf.equal(num_active, 1),
                tf.logical_and(tf.greater(num_active, 1), tf.less_equal(num_active, max_active_notes))
            )
        )

    dataset = dataset.filter(prefilter)

    # 2. Batch for vectorized suffix computation
    VECTORIZE_CHUNK = 1024

    def attach_suffix_batch(audio, frame_nr, labels):
        # Pass full 129-dimensional labels to the classifier
        suffix_idx = get_chord_suffix_idx_batch(labels)
        return audio, frame_nr, labels, suffix_idx

    dataset = (
        dataset
        .batch(VECTORIZE_CHUNK)
        .map(attach_suffix_batch, num_parallel_calls=tf.data.AUTOTUNE)
    )

    if drop_unrecognized:
        dataset = dataset.filter(
            lambda a, fnr, l, idx: tf.reduce_any(idx != UNRECOGNIZED_IDX)
        )

    # 3. Stateful scan — caps count per chord-suffix bucket
    initial_state = tf.zeros((NUM_SUFFIXES,), dtype=tf.int32)

    def scan_fn(suffix_hist, batch_element):
        audio, frame_nr, labels, suffix_idx = batch_element

        # For each frame in the batch, check if we can keep it
        current_counts = tf.gather(suffix_hist, suffix_idx)
        can_keep = current_counts < max_labels

        # Increment histogram
        batch_updates = tf.scatter_nd(
            indices=tf.expand_dims(suffix_idx, axis=1),
            updates=tf.ones([tf.shape(suffix_idx)[0]], dtype=tf.int32),
            shape=[NUM_SUFFIXES]
        )
        new_suffix_hist = suffix_hist + batch_updates

        return new_suffix_hist, (audio, frame_nr, labels, can_keep)

    dataset = dataset.scan(initial_state=initial_state, scan_func=scan_fn)

    # 4. Unbatch, filter kept frames, shuffle, batch for training
    dataset = (
        dataset
        .unbatch()
        .filter(lambda a, fnr, l, keep: keep)
        .map(lambda a, fnr, l, k: (a, fnr, l))
        .shuffle(buffer_size=5000, reshuffle_each_iteration=True)
        .batch(batch_size)
        .prefetch(tf.data.AUTOTUNE)
    )

    return dataset


def debug_13_matches(dataset, num_samples=500):
    """
    Find frames classified as '13' and show their actual pitch classes.
    """
    import collections
    
    count = 0
    examples_by_pc = collections.defaultdict(list)
    
    for audio, frame_nr, labels in dataset.unbatch().take(50000):
        suffix_idx = get_chord_suffix_idx_batch(labels[None, :])[0].numpy()
        
        if suffix_idx == SUFFIX_TO_IDX['13']:
            # Extract pitch classes
            pc_mask = labels_to_pc_mask(labels[None, :])[0].numpy()
            bass_pc = bass_pc_from_labels(labels[None, :])[0].numpy()
            
            # Decode bitmask to pitch classes
            pcs = tuple(sorted([i for i in range(12) if (pc_mask >> i) & 1]))
            
            examples_by_pc[pcs].append((int(bass_pc), int(pc_mask)))
            count += 1
            
            if count >= 500:
                break
    
    print("\n=== '13' matches by actual pitch class set ===")
    for pcs in sorted(examples_by_pc.keys(), key=lambda x: -len(examples_by_pc[x]))[:20]:
        count_this = len(examples_by_pc[pcs])
        bass_pcs = set(x[0] for x in examples_by_pc[pcs])
        
        # Format the tuple as a string
        pcs_str = str(pcs)
        print(f"{pcs_str:35s} : {count_this:5d} frames, bass_pcs={sorted(bass_pcs)}")
        
        # Try to identify what chord this actually is
        pc_set = set(pcs)
        matching_chords = []
        for chord_name, formula in CHORD_FORMULAS.items():
            formula_pcs = set(x % 12 for x in formula)
            if formula_pcs == pc_set:
                matching_chords.append(chord_name)
        
        if matching_chords:
            print(f"  → Matches: {matching_chords}")
        else:
            print(f"  → No exact match in CHORD_FORMULAS")


def show_raw_notes_for_13_frames(dataset, num_frames=20):
    """Show the raw MIDI notes in frames classified as '13'"""
    count = 0
    
    for audio, frame_nr, labels in dataset.unbatch().take(100000):
        suffix_idx = get_chord_suffix_idx_batch(labels[None, :])[0].numpy()
        
        if suffix_idx == SUFFIX_TO_IDX['13']:
            # Get active MIDI notes
            note_labels = labels[:128].numpy()
            active_notes = [i for i in range(128) if note_labels[i] > 0]
            
            # Get pitch classes
            pcs = tuple(sorted(set(n % 12 for n in active_notes)))
            
            print(f"\nFrame {count}: MIDI notes={active_notes}, pitch_classes={pcs}")
            print(f"  Silence flag: {labels[128].numpy()}")
            
            count += 1
            if count >= num_frames:
                break

