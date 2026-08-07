import tensorflow as tf
import numpy as np
from common import OUTPUT_DIM_NOTES

# ============================================================
# Static setup — chord formulas, bitmask tables
# ============================================================

CHORD_FORMULAS = {





    'silent':               [],  # Silence: 129th label = 1, all notes = 0
    'single_note':          [0],

    'power_chord':          [0, 7],
    'major':                [0, 4, 7],
    'minor':                [0, 3, 7],
    'sus4':                 [0, 5, 7],
    'sus2':                 [0, 2, 7],
    'diminished':           [0, 3, 6],
    'augmented':            [0, 4, 8],
    'no3_add4':             [0, 5, 7],

    'major7':               [0, 4, 7, 11],
    'minor7':               [0, 3, 7, 10],
    'dominant7':            [0, 4, 7, 10],

    'add9':                 [0, 4, 7, 14],
    'minorAdd9':            [0, 3, 7, 14],
    'major6':               [0, 4, 7, 9],
    'minor6':               [0, 3, 7, 9],  
    'diminished7':          [0, 3, 6, 9],
    'minor7b5':             [0, 3, 6, 10],  
    'minorMajor7':          [0, 3, 7, 11],
    'sus4_7':               [0, 5, 7, 10],
    'sus2_7':               [0, 2, 7, 10],
   # '13':                   [0, 4, 7, 10, 14, 21],
    '9':                    [0, 4, 7, 10, 14],
    'major9':               [0, 4, 7, 11, 14],
    'minor9':               [0, 3, 7, 10, 14],
    '7#9':                  [0, 4, 7, 10, 15],
    '7b9':                  [0, 4, 7, 10, 13],
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

# this funtion  creates a histogram over all chord intervals in the dataset. It counts all interval patterns. The root is always the lowest note in the chord.
# it first finds the lowest note (bass) and rotates the mask to root position. It then counts the occurrences of each interval pattern.
# the function is highly vectorized and uses tf.data.Dataset to process the data efficiently. It returns a dictionary mapping interval patterns (as tuples of pitch classes) to their counts.
import collections
import json
import pickle
from pathlib import Path

import tensorflow as tf

def _process_labels_to_bitpositions(labels,include_silence=True):
        note_labels = labels[:, :37]                     # [B, 37]
        active = note_labels > 0                          # [B, 37] bool

        active_count = tf.reduce_sum(tf.cast(active, tf.int32), axis=1)  # [B]

        if include_silence:
            valid_mask = tf.ones_like(active_count, dtype=tf.bool)
        else:
            valid_mask = active_count >= 1                 # only skip true silence

        is_silent = tf.equal(active_count, 0)
        # Find the lowest active note (root) for each frame
        # first, create a tensor of indices 0..36 (for 37 notes)
        idx = tf.range(37, dtype=tf.int32)                # [37]

        # Broadcast idx to match the batch size. this will create a [B, 37] tensor where each row is [0, 1, 2, ..., 36]
        idx_b = tf.broadcast_to(idx, tf.shape(active))     # [B, 37]

        # Use a sentinel value (e.g., 37) for inactive notes, so that when we take the min, we ignore them. This is safe because the max index is 36.
        sentinel = tf.constant(37, dtype=tf.int32)

        # Use tf.where to replace inactive indices with the sentinel value. This will give us a tensor where active notes have their original index, and inactive notes have 37.
        masked_idx = tf.where(active, idx_b, sentinel)     # [B, 37]

        # Now, we can safely take the min across axis=1 to find the lowest active note index for each frame. If a frame is silent, this will return 37, which we can handle later.
        root_note = tf.reduce_min(masked_idx, axis=1)      # [B]

        # For silent frames, we can set the root_note to a safe value (e.g., 0) since we won't use it. This avoids issues with negative indices or invalid operations later.
        safe_root = tf.where(is_silent, tf.zeros_like(root_note), root_note)

        # Now, compute the relative offsets of all active notes from the root. This will give us a tensor of shape [B, 37] where each entry is the interval from the root note. For silent frames, this will be ignored.
        rel_offsets = idx_b - safe_root[:, None]           # [B, 37]

        # Create a boolean mask for valid intervals: active notes that are within the range 0..36. This ensures we only consider notes that are actually present and within the valid range.
        bit_valid = active & (rel_offsets >= 0) & (rel_offsets < 37)
        bit_valid = bit_valid & (~is_silent[:, None])
        # Now, we can compute the bit positions for the valid intervals. For each valid interval, we will use its relative offset as the bit position in a 128-bit integer. This will allow us to create a unique bitmask for each interval pattern.
        bit_positions = tf.where(bit_valid, rel_offsets, tf.zeros_like(rel_offsets))

        return bit_positions, bit_valid, is_silent,rel_offsets,valid_mask


def compute_chord_interval_histogram_tf(
    dataset,
    batch_size=4096,
    include_silence=False,
    save_path=None,
    save_format="json",
):
    """
    TensorFlow-optimized version of compute_chord_interval_histogram.

    For each frame, finds the lowest active MIDI note (root), computes
    intervals of all active notes relative to that root (NOT wrapped to
    pitch class -- matches original semantics exactly), encodes the
    resulting interval-pattern as a 128-bit bitmask (since intervals can
    range 0..127), and accumulates global counts per unique bitmask using
    a batched Counter merge.

    Single notes are now included (pattern = (0,)).
    Silent frames (no active notes) are skipped by default since there is
    no root to anchor intervals to; set include_silence=True to count them
    under a special empty-pattern key ().

    Args:
        dataset: tf.data.Dataset yielding (audio, labels) batches or single
            examples (will be unbatched/rebatched internally).
        batch_size: batch size used for internal vectorized processing.
        include_silence: whether to count silent frames under key ().
        save_path: if provided, write the resulting histogram to disk at
            this path. Directory is created if it doesn't exist.
        save_format: "json" (human-readable, keys stringified) or
            "pickle" (preserves exact tuple keys and Counter type).

    Returns:
        dict mapping interval pattern (tuple of ints) -> count
    """

    flat_ds = dataset.unbatch()
    flat_ds = flat_ds.batch(batch_size)

    def process_batch(audio, labels):
        bit_positions, bit_valid, is_silent, rel_offsets, valid_mask = _process_labels_to_bitpositions(labels, include_silence=include_silence)

        # Split the bit positions into two groups: those that fit in the lower 64 bits and those that fit in the upper 64 bits. This is necessary because we are using two 64-bit integers to represent the full 128-bit bitmask.
        low_mask = bit_valid & (bit_positions < 64)
        high_mask = bit_valid & (bit_positions >= 64)

        # Now, we can compute the low and high words (64-bit integers) for each frame. We will use bitwise operations to set the appropriate bits in each word based on the valid bit positions.
        low_bits = tf.where(low_mask, bit_positions, tf.zeros_like(bit_positions))
        high_bits = tf.where(high_mask, bit_positions - 64, tf.zeros_like(bit_positions))

        # Compute the low and high words by left-shifting 1 by the bit positions and summing them up. This will give us a unique integer representation for each interval pattern.
        low_pow = tf.bitwise.left_shift(
            tf.ones_like(low_bits, dtype=tf.int64),
            tf.cast(low_bits, tf.int64)
        )
        low_pow = tf.where(low_mask, low_pow, tf.zeros_like(low_pow))
        low_word = tf.reduce_sum(low_pow, axis=1)          # [B] int64

        high_pow = tf.bitwise.left_shift(
            tf.ones_like(high_bits, dtype=tf.int64),
            tf.cast(high_bits, tf.int64)
        )
        high_pow = tf.where(high_mask, high_pow, tf.zeros_like(high_pow))
        high_word = tf.reduce_sum(high_pow, axis=1)        # [B] int64

        low_word = tf.where(is_silent, tf.constant(-1, dtype=tf.int64), low_word)
        high_word = tf.where(is_silent, tf.constant(-1, dtype=tf.int64), high_word)

        return low_word, high_word, valid_mask
    # Map the process_batch function over the dataset with parallel calls for efficiency. This will allow us to process multiple batches concurrently, improving performance when computing the histogram.
    mapped_ds = flat_ds.map(process_batch, num_parallel_calls=tf.data.AUTOTUNE)

    pattern_counts = collections.Counter()
    # Iterate over the mapped dataset and accumulate counts for each unique interval pattern. We will convert the low and high words to numpy arrays for easier processing and use a Counter to keep track of the occurrences of each pattern.
    for low_word, high_word, valid_mask in mapped_ds.prefetch(tf.data.AUTOTUNE):
        low_np = low_word.numpy()
        high_np = high_word.numpy()
        valid_np = valid_mask.numpy()

        keys = list(zip(low_np[valid_np].tolist(), high_np[valid_np].tolist()))
        local_counter = collections.Counter(keys)
        pattern_counts.update(local_counter)

    def decode_bitmask(low_word, high_word):
        if low_word == -1 and high_word == -1:
            return ()  # silent/empty pattern
        offsets = []
        for k in range(64):
            if (low_word >> k) & 1:
                offsets.append(k)
        for k in range(64):
            if (high_word >> k) & 1:
                offsets.append(k + 64)
        return tuple(sorted(offsets))
    # Convert the pattern_counts from (low_word, high_word) keys to actual interval patterns (tuples of offsets). This will give us a more human-readable representation of the interval patterns and their counts.
    interval_histogram = collections.Counter({
        decode_bitmask(low, high): count
        for (low, high), count in pattern_counts.items()
    })
    # If a save_path is provided, persist the histogram to disk in the specified format (JSON or pickle). This allows for easy sharing and reuse of the computed histogram without needing to recompute it from the dataset.
    if save_path is not None:
        _save_histogram(interval_histogram, save_path, save_format)

    return interval_histogram


def _save_histogram(histogram, save_path, save_format="json"):
    """
    Persist an interval histogram to disk.

    JSON format: keys are stringified tuples (e.g. "0,4,7") sorted by
    descending count, plus metadata (total count, num unique patterns).
    Pickle format: exact Counter object with tuple keys, round-trips
    perfectly via pickle.load.
    """
    save_path = Path(save_path)
    save_path.parent.mkdir(parents=True, exist_ok=True)

    if save_format == "pickle":
        with open(save_path, "wb") as f:
            pickle.dump(histogram, f, protocol=pickle.HIGHEST_PROTOCOL)

    elif save_format == "json":
        total = sum(histogram.values())
        sorted_items = sorted(histogram.items(), key=lambda kv: kv[1], reverse=True)

        payload = {
            "total_count": total,
            "num_unique_patterns": len(histogram),
            "histogram":{
                ",".join(map(str, pattern)): count for pattern, count in sorted_items
            },
            
            
        }

        with open(save_path, "w") as f:
            json.dump(payload, f, indent=2)

    else:
        raise ValueError(f"Unknown save_format: {save_format!r} (use 'json' or 'pickle')")

    print(f"Saved interval histogram ({len(histogram)} unique patterns) to {save_path}")


def load_histogram(load_path):
    """
    Load a histogram saved by _save_histogram.

    Auto-detects format by extension (.pkl/.pickle -> pickle, else JSON).
    Returns a collections.Counter with tuple keys in both cases.
    """
    load_path = Path(load_path)

    if load_path.suffix in (".pkl", ".pickle"):
        with open(load_path, "rb") as f:
            return pickle.load(f)

    with open(load_path, "r") as f:
        payload = json.load(f)

    return collections.Counter({
        tuple(entry["pattern"]): entry["count"]
        for entry in payload["histogram"]
    })


def load_and_compute_chord_weights(histogram_path, weight_cap=1000,weight_cap_low=0.1,include_counts=False):
    # Load a histogram from disk and compute weights for each interval pattern based on its frequency.
    with open(histogram_path, 'r') as f:
        pos_chord_weights=dict()
        data = json.load(f)

        # Get the metadata from the histogram
        total_count = data['total_count']
        num_unique_patterns = data['num_unique_patterns']

        # Compute weights for each interval pattern based on its frequency in the histogram. The weight is calculated as (total_count - count) / count, which gives higher weights to less frequent patterns. If include_counts is True, the weight is returned along with the count for each pattern.
        histogram = data['histogram']
        for pattern_str, count in histogram.items():
            pattern = tuple(map(int, pattern_str.split(','))) if pattern_str!='' else None
            # Compute the weight for the pattern based on its frequency in the histogram
            if include_counts:
                pos_chord_weights[pattern] = ((total_count-count)/count if count != 0 else 1.0,count)
            else:
                pos_chord_weights[pattern] = (total_count-count)/count if count != 0 else 1.0
        if include_counts:
            #get the min weight
            min_weight = min(weight[0] for pattern, weight in pos_chord_weights.items())
            print("Minimum weight:", min_weight)
            #normalize the weights by dividing by the min weight
            pos_chord_weights = {pattern: (min(weight_cap, max(weight_cap_low, weight[0]/min_weight)), weight[1]) for pattern, weight in pos_chord_weights.items()}
        else:
            #get the min weight
            min_weight = min(pos_chord_weights.values())
            print("Minimum weight:", min_weight)
            #normalize the weights by dividing by the min weight
            pos_chord_weights = {pattern: min(weight_cap, max(weight_cap_low, weight/min_weight)) for pattern, weight in pos_chord_weights.items()}
        return pos_chord_weights, total_count, num_unique_patterns


def build_pattern_weight_table(histogram_path, weight_cap=1000,weight_cap_low=0.1, default_weight=1.0):
    """
    Build a TensorFlow StaticHashTable mapping interval patterns (as tuples of pitch classes) to their corresponding weights, based on a precomputed histogram.
    histogram_path: Path to the JSON file containing the histogram of interval patterns and their counts.
    weight_cap: Maximum weight to assign to any pattern (to avoid extreme values).
    weight_cap_low: Minimum weight to assign to any pattern (to avoid extreme values).
    default_weight: Weight to assign to patterns not found in the histogram.
    Returns:
        A tf.lookup.StaticHashTable that can be used to look up weights for interval patterns.
    """

    # Load the histogram and compute weights for each interval pattern. The weights are normalized and capped to avoid extreme values. The function returns a dictionary mapping interval patterns (as tuples) to their corresponding weights, along with the total count of patterns and the number of unique patterns.
    pos_chord_weights, total_count, num_unique_patterns = load_and_compute_chord_weights(histogram_path, weight_cap=weight_cap, weight_cap_low=weight_cap_low, include_counts=False)
    keys = []
    values = []
    for pattern, weight in pos_chord_weights.items():
        key_str = "" if pattern is None else ",".join(map(str, pattern))
        keys.append(key_str)
        values.append(float(weight))
    # Create TensorFlow tensors for the keys and values, which will be used to initialize the StaticHashTable. The keys are converted to strings, and the values are converted to float32 for compatibility with TensorFlow operations.
    keys_tensor = tf.constant(keys, dtype=tf.string)
    values_tensor = tf.constant(values, dtype=tf.float32)

    # Create a StaticHashTable that maps interval pattern strings to their corresponding weights. The table is initialized with the keys and values tensors, and a default weight is specified for patterns not found in the histogram. This allows for efficient lookups of weights during model training or inference.
    with tf.device('/GPU:0'):
        table = tf.lookup.StaticHashTable(
            tf.lookup.KeyValueTensorInitializer(keys_tensor, values_tensor),
            default_value=default_weight,
        )
    return table



def y_true_to_interval_key(y_true, num_notes=37):
    """
    Convert a batch of one-hot note labels to interval pattern keys (as strings).
    Each key is a comma-separated string of intervals relative to the lowest active note (root).
    If a frame is silent (no active notes), the key is an empty string.
    Args:
        y_true: Tensor of shape [batch_size, num_notes] with one-hot note labels
        num_notes: Number of note dimensions (default 37)
    Returns:
        Tensor of shape [batch_size] with string keys representing interval patterns"""

    bit_positions, bit_valid, is_silent, rel_offsets, valid_mask = _process_labels_to_bitpositions(y_true, include_silence=True)

    def row_to_key(offsets_row, valid_row, silent_row):
        return tf.cond(
            silent_row,
            lambda: tf.constant("", dtype=tf.string),
            lambda: tf.strings.reduce_join(
                tf.strings.as_string(tf.boolean_mask(offsets_row, valid_row)),
                separator=",",
            ),
        )
    # Use tf.map_fn to apply the row_to_key function to each row in the batch, generating a string key for each frame based on its interval pattern. The output signature is specified as tf.string to ensure the correct data type is returned.
    keys = tf.map_fn(
        lambda args: row_to_key(args[0], args[1], args[2]),
        (rel_offsets, bit_valid, is_silent),
        fn_output_signature=tf.string,
    )

    return keys