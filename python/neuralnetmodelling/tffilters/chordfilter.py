import tensorflow as tf
import numpy as np
from common import OUTPUT_DIM_NOTES

# ============================================================
# Static setup — chord formulas, bitmask tables (unchanged from before)
# ============================================================

CHORD_FORMULAS = {
    'major':                [0, 4, 7],
    'minor':                [0, 3, 7],
    'sus4':                 [0, 5, 7],
    'sus2':                 [0, 2, 7],
    'power_chord':          [0, 7],
    'major7':               [0, 4, 7, 11],
    'dominant7':            [0, 4, 7, 10],
    'minor7':               [0, 3, 7, 10],
    'add9':                 [0, 4, 7, 14],
    'minorAdd9':            [0, 3, 7, 14],
    'major6':               [0, 4, 7, 9],
    'minor6':               [0, 3, 7, 9],
    'diminished':           [0, 3, 6],
    'diminished7':          [0, 3, 6, 9],
    'minor7b5':             [0, 3, 6, 10],
    'augmented':            [0, 4, 8],
    'minorMajor7':          [0, 3, 7, 11],
    'sus4_7':               [0, 5, 7, 10],
    'sus2_7':               [0, 2, 7, 10],
    '9':                    [0, 4, 7, 10, 14],
    'major9':               [0, 4, 7, 11, 14],
    'minor9':               [0, 3, 7, 10, 14],
    '13':                   [0, 4, 7, 10, 14, 21],
    '7#9':                  [0, 4, 7, 10, 15],
    '7b9':                  [0, 4, 7, 10, 13],
    'no3_add4':             [0, 5, 7],
}
CHORD_NAMES = list(CHORD_FORMULAS.keys())
NUM_CHORDS = len(CHORD_NAMES)
UNRECOGNIZED_IDX = NUM_CHORDS
NUM_SUFFIXES = NUM_CHORDS + 1

# SUFFIX_VOCAB now DERIVED from CHORD_NAMES — single source of truth,
# eliminates the index-desync risk from maintaining two parallel lists.
SUFFIX_VOCAB = CHORD_NAMES + ['UNRECOGNIZED']
SUFFIX_TO_IDX = {name: i for i, name in enumerate(SUFFIX_VOCAB)}
assert SUFFIX_TO_IDX['UNRECOGNIZED'] == UNRECOGNIZED_IDX  # sanity check

# ============================================================
# convert chord intervals to pitch-class bitmask and rotate by root
# ============================================================
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
# convert label vectors to pitch-class bitmasks and bass pitch class
# the pitch-class bitmask is a 12-bit integer where each bit represents the presence of a pitch class (C, C#, D, ..., B)
# the bass pitch class is the pitch class of the lowest active note
# ============================================================
def labels_to_pc_mask(labels):
    labels = tf.cast(labels, tf.int32)
    note_idx = tf.range(OUTPUT_DIM_NOTES, dtype=tf.int32)
    pc_idx = note_idx % 12 # pitch class index
    pc_onehot = tf.one_hot(pc_idx, depth=12, dtype=tf.int32)     # [OUTPUT_DIM_NOTES, 12]
    active = labels[..., :, None] * pc_onehot[None, ...]         # [..., OUTPUT_DIM_NOTES, 12]: after this multiple notes in the same pitch class will be in one section of the tensor: active[batch,frame,:,pc] = 1 if any note in that pitch class is active, regardless of the octave. This is what we want for the pitch-class bitmask.
    pc_present = tf.reduce_max(active, axis=-2)                  # [..., 12] collapse the note dimension, leaving only the pitch class dimension. pc_present[batch,frame,pc] = 1 if any note in that pitch class is active
    bit_weights = tf.constant([1 << i for i in range(12)], dtype=tf.int32)# weights for each pitch class bit in the bitmask. bit_weights[pc] = 1 << pc
    return tf.reduce_sum(pc_present * bit_weights, axis=-1)#convert the 12 dim pitch class per frame to a single integer bitmask. pc_mask[batch,frame] = sum(1 << pc for each active pitch class in that frame). this gives a unique number for all the pitch class notes in the frame

# ============================================================
# get the bass pitch class from the labels which is the pitch class of the lowest active note
# ============================================================
def bass_pc_from_labels(labels):
    labels = tf.cast(labels, tf.int32)
    note_idx = tf.range(OUTPUT_DIM_NOTES, dtype=tf.int32)
    masked_idx = tf.where(labels > 0, note_idx, tf.fill(tf.shape(labels), 999))
    lowest_note = tf.reduce_min(masked_idx, axis=-1)
    return lowest_note % 12 # project to pitch class


def get_chord_suffix_idx_batch(labels):
    """
    Vectorized replacement for GuitarChordAnalyzer(...).get_chord_suffix()['raw_name'],
    mapped to an index into CHORD_NAMES (+ UNRECOGNIZED_IDX).

    labels: [B, OUTPUT_DIM_NOTES] binary tensor. Frames with 0 or 1 active notes will
    naturally resolve to UNRECOGNIZED_IDX (no formula matches <2 notes).

    Exact pitch-class-set matches only — no subset/partial fallback. This avoids
    silently collapsing extended/altered chords (e.g. dominant7, add9, 9, 13) into
    their bare-triad subsets (major/minor) purely due to dict tiebreaking, which is
    exactly the corruption that inflated 'major' in the ground-truth histogram.

    the labels are first reduced to a pitch-class bitmask and bass pitch class, to disregard the octave information, then compared against the precomputed ROOT_CHORD_MASKS_TF table to find the best match.
    """
    labels = tf.cast(labels, tf.int32)
    B = tf.shape(labels)[0]

    # get the pitch-class bitmask and bass pitch class for each frame
    pc_mask = labels_to_pc_mask(labels)
    bass_pc = bass_pc_from_labels(labels)

    # get the chord suffix index for each frame by comparing the pitch-class
    # bitmask and bass pitch class to the precomputed chord masks
    all_masks = ROOT_CHORD_MASKS_TF[None, :, :]                  # [1, 12, NUM_CHORDS]
    pc_mask_bbb = pc_mask[:, None, None]                          # [B, 1, 1]

    exact_any = tf.equal(pc_mask_bbb, all_masks)                  # [B, 12, NUM_CHORDS]

    roots_range = tf.range(12, dtype=tf.int32)
    is_root_position = tf.equal(roots_range[None, :], bass_pc[:, None])  # [B, 12]
    is_root_position_b = is_root_position[:, :, None]

    # priority: 0 = exact + root position, 1 = exact but not root position (slash),
    # 2 = no match at all -> UNRECOGNIZED
    match_priority = tf.where(
        exact_any,
        tf.where(is_root_position_b, 0, 1),
        2
    )
    match_priority = tf.cast(match_priority, tf.int32)

    dict_order_b = tf.tile(CHORD_DICT_ORDER_TF[None, None, :], [B, 12, 1])
    combined_score = match_priority * 1000 + dict_order_b
    combined_score_flat = tf.reshape(combined_score, [B, 12 * NUM_CHORDS])

    best_flat_idx = tf.argmin(combined_score_flat, axis=-1, output_type=tf.int32)
    best_priority = tf.gather(
        tf.reshape(match_priority, [B, 12 * NUM_CHORDS]), best_flat_idx, batch_dims=1
    )
    best_chord_idx = best_flat_idx % NUM_CHORDS

    suffix_idx = tf.where(best_priority < 2, best_chord_idx, tf.fill([B], UNRECOGNIZED_IDX))
    return tf.cast(suffix_idx, tf.int32)


# ============================================================
# Balanced dataset builder — now using the vectorized path, no py_function
# ============================================================

def create_balanced_dataset_chords(dataset, max_labels, batch_size=256,
                                    max_active_notes=6, num_midi_notes=OUTPUT_DIM_NOTES,
                                    drop_unrecognized=False,
                                    pre_shuffle_buffer=200_000):
    dataset = dataset.unbatch()

    def prefilter(audio, frame_nr, labels):
        labels_int = tf.cast(labels, tf.int32)
        num_active_silent = tf.reduce_sum(labels_int)
        return (num_active_silent > 0) & (num_active_silent <= (max_active_notes + 1))

    dataset = dataset.filter(prefilter)

    # >>> KEY FIX: shuffle BEFORE the capping scan, with a large buffer, so
    # long runs of a sustained chord get broken up across the stream and
    # don't instantly saturate that suffix's counter from one song alone. <<<
    dataset = dataset.shuffle(buffer_size=pre_shuffle_buffer, reshuffle_each_iteration=True)

    VECTORIZE_CHUNK = 1024

    def attach_suffix_batch(audio, frame_nr, labels):
        note_labels = labels[:, :num_midi_notes]
        suffix_idx = get_chord_suffix_idx_batch(note_labels)
        return audio, frame_nr, labels, suffix_idx
    

    dataset = (
        dataset
        .batch(VECTORIZE_CHUNK)
        .map(attach_suffix_batch, num_parallel_calls=tf.data.AUTOTUNE)
        .unbatch()
    )

    if drop_unrecognized:
        dataset = dataset.filter(lambda a, fnr, l, idx: idx != UNRECOGNIZED_IDX)

    initial_state = tf.zeros((NUM_SUFFIXES,), dtype=tf.int32)

    def scan_fn(suffix_hist, element):
        audio, frame_nr, labels, suffix_idx = element
        current_count = tf.gather(suffix_hist, suffix_idx)
        can_keep = current_count < max_labels
        update = tf.one_hot(suffix_idx, depth=NUM_SUFFIXES, dtype=tf.int32)
        new_suffix_hist = tf.cond(can_keep, lambda: suffix_hist + update, lambda: suffix_hist)
        return new_suffix_hist, (audio, frame_nr, labels, can_keep)

    dataset = dataset.scan(initial_state=initial_state, scan_func=scan_fn)

    # A final, smaller re-shuffle is still fine/useful for batch composition,
    # but the heavy lifting against autocorrelation now happens up front.
    dataset = (
        dataset
        .filter(lambda a, fnr, l, keep: keep)
        .map(lambda a, fnr, l, k: (a, fnr, l))
        .shuffle(buffer_size=5000, reshuffle_each_iteration=True)
        .batch(batch_size)
        .prefetch(tf.data.AUTOTUNE)
    )

    return dataset