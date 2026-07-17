import tensorflow as tf
from tensorflow.keras import layers, models, regularizers
# GuitarMidi-LV2 Library
 # Copyright (C) 2026 Gerald Mwangi
 #
 # This program is free software; you can redistribute it and/or
 # modify it under the terms of the GNU Lesser General Public
 # License as published by the Free Software Foundation; either
 # version 2 of the License, or (at your option) any later version.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 # Lesser General Public License for more details.
 #
 # You should have received a copy of the GNU Lesser General
 # Public License along with this program; if not, write to the
 # Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 # Boston, MA  02110-1301  USA


from common import OUTPUT_DIM_NOTES, image_height, image_width

IMG_H, IMG_W = image_height, image_width
NUM_CLASSES = 89
CHANNELS = 1
reg = None#regularizers.l2(1e-6)
reg2d = None#regularizers.l2(1e-6)

# Diagram of the model in ASCII art. Each block should be drawn as a box. The six string layers should be drawn as parallel boxes after the transformer, and then combined into a single box for the chord reasoning, followed by the output layer.:
# Input Spectrogram (148 butterworth filter outputs (37  notes, 4 harmonics per note) x 256 samples)
#     |
# Local Contrast Normalization
#     |
# Time Compression (1D Conv along time axis)
#     |
# Transformer Block acting over the frequency features (Multi-Head Self-attention + FFN)
#     |
# String-aware Slicing into 6 parallel paths
#     |         |         |         |         |         |
# String 1    String 2    String 3    String 4    String 5    String 6
#     |         |         |         |         |         |
# Chord Reasoning (Conv across strings) followed by hollow neighborhood suppression and Global Max Pooling per string
#     |
# String-specific Dense layers to produce 6x13 outputs (one per string/fret)
# String/fret outputs are then scattered to the final 37-note output space using a fixed sparse mapping based on the guitar tuning and fret range.
# Output Notes (Dense Layer)
import numpy as np
import tensorflow as tf

# Define your tuning and fret range
# (string_idx, open_midi_note, num_frets)
STRING_TUNING = [
    (0, 40, 13),  # low E  - E2
    (1, 45, 13),  # A      - A2
    (2, 50, 13),  # D      - D3
    (3, 55, 13),  # G      - G3
    (4, 59, 13),  # B      - B3
    (5, 64, 13),  # high e - E4
]


# Build scatter indices: for each (string, fret) → output note index
# Assuming your 37 notes span some MIDI range, e.g. MIDI 40-76
MIDI_MIN = 40  # lowest note in your 37-note output space

def build_scatter_map(string_tuning, midi_min, n_output=37):
    """
    Returns a (6, max_frets, n_output) boolean mask.
    string_output[s, f] scatters to output note (open_midi[s] + f - midi_min).
    """
    scatter_indices = []
    for string_idx, open_midi, num_frets in string_tuning:
        for fret in range(num_frets):
            note_idx = open_midi + fret - midi_min
            scatter_indices.append((string_idx, fret, note_idx))
    return scatter_indices

scatter_indices = build_scatter_map(STRING_TUNING, MIDI_MIN)

# Build a fixed (6*13, 37) sparse mask
N_STRINGS = 6
N_FRETS = 13
N_NOTES = 37

mask = np.zeros((N_STRINGS * N_FRETS, N_NOTES), dtype=np.float32)
for s, f, n in scatter_indices:
    if 0 <= n < N_NOTES:
        mask[s * N_FRETS + f, n] = 1.0

mask_tensor = tf.constant(mask)  # (78, 37)
# Instead of Lambda for hollow suppression, use a proper layer:
class HollowSuppression(tf.keras.layers.Layer):
    def call(self, y):
        y_padded = tf.pad(y, [[0,0],[1,1],[0,0]])
        left  = y_padded[:, :-2, :]
        right = y_padded[:, 2:,  :]
        return y - (left + right) / 2.0

# Same for append_silence and strip_silence
class AppendSilence(tf.keras.layers.Layer):
    def call(self, x):
        silence = tf.zeros_like(x[:, :1])
        return tf.concat([x, silence], axis=1)

class StripSilence(tf.keras.layers.Layer):
    def call(self, x):
        return x[:, :-1]


class SparseGuitarOutput(tf.keras.layers.Layer):
    def __init__(self, mask, **kwargs):
        super().__init__(**kwargs)
        self.mask = tf.constant(mask, dtype=tf.float32)  # (78, 37)

    def call(self, x):
        batch = tf.shape(x)[0]
        x_flat = tf.reshape(x, (batch, N_STRINGS * N_FRETS))
        x_exp = tf.expand_dims(x_flat, 2)
        mask_exp = tf.expand_dims(tf.cast(self.mask, x.dtype), 0)
        xmasked = x_exp * mask_exp
        # negative and operation of probilities to get the most prominent string/fret combination per note, without loss of the other combinations that contribute to the same note
        # its the equivalent of the logical OR operation for probabilities: P(A or B) = 1 - (1 - P(A)) * (1 - P(B))
        negprod=tf.reduce_prod(1.0 - xmasked, axis=1)

        return 1.0 - negprod



    def get_config(self):
        config = super().get_config()
        return config


def string_layer(x, start, end, max_x, training, string_idx=0):
    end = min(int(end), int(max_x))
    start = max(0, int(start))
    print(f"String {string_idx}: slicing from {start} to {end} (max_x={max_x})")
    prefix = f"str{string_idx}"
    
    s = layers.Lambda(lambda y, st=start, en=end: y[:, st:en, :], name=f"{prefix}_slice")(x)
    # res = layers.Conv1D(64, 4, strides=4, padding='valid',
    #                     kernel_regularizer=reg, name=f"{prefix}_res_proj")(s)
    x = layers.Conv1D(32, 1, padding='same', kernel_initializer='he_normal', name=f"{prefix}_backbone_squeeze", kernel_regularizer=reg)(s)
    x = layers.BatchNormalization(name=f"{prefix}_backbone_squeeze_bn")(x)
    x = layers.LeakyReLU(name=f"{prefix}_backbone_squeeze_act")(x)

    x = layers.Conv1D(32, 8, padding='same', kernel_initializer='he_normal', name=f"{prefix}_backbone_conv1", kernel_regularizer=reg)(x)
    x = layers.BatchNormalization(name=f"{prefix}_backbone_bn1")(x)
    x = layers.LeakyReLU(name=f"{prefix}_backbone_act1")(x)
    x = layers.SpatialDropout1D(0.2, name=f"{prefix}_backbone_drop1")(x)

    x = layers.Conv1D(64, 8, padding='same', kernel_initializer='he_normal', name=f"{prefix}_backbone_conv2", kernel_regularizer=reg)(x)
    x = layers.BatchNormalization(name=f"{prefix}_backbone_bn2")(x)
    s = layers.LeakyReLU(name=f"{prefix}_backbone_act2")(x)

    

    # Collapse 4 harmonic bins → 1 vector per note
    s = layers.Conv1D(64, 4, strides=4, padding='valid', 
                      kernel_regularizer=reg, name=f"{prefix}_harmonic_collapse")(s)
    s = layers.BatchNormalization(name=f"{prefix}_harmonic_bn")(s)
    s = layers.LeakyReLU(name=f"{prefix}_harmonic_act")(s)

    if string_idx==5:
        s = layers.SpatialDropout1D(0.2, name=f"{prefix}_harmonic_drop")(s)
    else:
        s = layers.SpatialDropout1D(0.1, name=f"{prefix}_harmonic_drop")(s)

    #s=layers.Add(name=f"{prefix}_res1_conv1")([s, res])
    # Now shape is (batch, 13, 64) — one vector per note



    s = HollowSuppression(name=f"{prefix}_hollow_suppress")(s)
    s = layers.LeakyReLU(name=f"{prefix}_hollow_act")(s)

    # Optional: A 1x1 conv to mix the newly sharpened features
    s = layers.Conv1D(64, 1, padding='same', kernel_regularizer=reg, name=f"{prefix}_post_suppress_conv")(s)
    s = layers.BatchNormalization(name=f"{prefix}_post_suppress_bn")(s)
    s = layers.LeakyReLU(name=f"{prefix}_post_suppress_act")(s)

    return s



def transformer_block(x, num_heads=2, head_size=32, ff_dim=128, dropout=0.1, name_prefix="tfm"):
    # --- Attention Block ---
    # 1. Apply LayerNorm BEFORE attention (Off-ramp)
    x_norm1 = layers.LayerNormalization(epsilon=1e-6, name=f"{name_prefix}_ln1")(x)
    
    # 2. Compute Attention
    attn = layers.MultiHeadAttention(
        num_heads=num_heads, key_dim=head_size, dropout=dropout, name=f"{name_prefix}_mha", kernel_regularizer=reg
    )(x_norm1, x_norm1)
    
    # 3. Add to original input WITHOUT normalizing the result (Clear highway!)
    x1 = layers.Add(name=f"{name_prefix}_attn_add")([x, attn])

    # --- Feed Forward Block ---
    # 1. Apply LayerNorm BEFORE FFN (Off-ramp)
    x_norm2 = layers.LayerNormalization(epsilon=1e-6, name=f"{name_prefix}_ln2")(x1)
    
    # 2. Compute FFN
    ffn = layers.Dense(ff_dim, activation='relu', name=f"{name_prefix}_ffn1")(x_norm2)
    ffn = layers.Dropout(dropout, name=f"{name_prefix}_ffn_drop")(ffn)
    ffn = layers.Dense(x.shape[-1], name=f"{name_prefix}_ffn2")(ffn)

    # 3. Add to x1 WITHOUT normalizing the result (Clear highway!)
    return layers.Add(name=f"{name_prefix}_ffn_add")([x1, ffn])

def chord_conv_block(string_features, filters,output_dim,training, kernel_size=(3,4), name_prefix="chord"):
    # store the original string features for later as residuals
    

    max_len = max(s.shape[1] for s in string_features)
    padded = []
    for i, s in enumerate(string_features):
        diff = max_len - s.shape[1]
        if diff > 0:
            s = layers.ZeroPadding1D((0, diff), name=f"{name_prefix}_pad_str{i}")(s)
        padded.append(s)
    string_residuals = padded.copy()
    expanded = [layers.Reshape((1, max_len, 64), name=f"{name_prefix}_expand_str{i}")(s) for i, s in enumerate(padded)]
    stacked = layers.Concatenate(axis=1, name=f"{name_prefix}_stack_strings")(expanded)  # (B, 6, max_len, 64)


    c1=layers.Conv2D(filters, kernel_size, padding='same', name=f"{name_prefix}_conv1",kernel_regularizer=reg2d)(stacked)
    c1=layers.BatchNormalization(name=f"{name_prefix}_bn1")(c1)
    c1 = layers.ELU(name=f"{name_prefix}_act1")(c1)


    c2=layers.Conv2D(filters, kernel_size, padding='same', name=f"{name_prefix}_conv2",kernel_regularizer=reg2d)(c1)
    c2=layers.BatchNormalization(name=f"{name_prefix}_bn2")(c2)
    c2=layers.LeakyReLU(name=f"{name_prefix}_act2")(c2)

    c2=layers.Conv2D(2*filters, kernel_size, padding='same', name=f"{name_prefix}_conv3",kernel_regularizer=reg2d)(c2)
    c2=layers.BatchNormalization(name=f"{name_prefix}_bn3")(c2)
    c2=layers.LeakyReLU(name=f"{name_prefix}_act3")(c2)

    # c2=layers.Conv2D(2*filters, kernel_size, padding='same', name=f"{name_prefix}_conv4",kernel_regularizer=reg2d)(c2)
    # c2=layers.BatchNormalization(name=f"{name_prefix}_bn4")(c2)
    # c2=layers.LeakyReLU(name=f"{name_prefix}_act4")(c2)
    chord=c2#layers.Add(name=f"{name_prefix}_res_c1_c2")([c2, c1])
    chord=layers.SpatialDropout2D(0.2, name=f"{name_prefix}_drop1")(chord)
    # Split the chord features back into per-string tensors
    split_chords = [layers.Lambda(lambda t, i=i: t[:, i, :, :], name=f"{name_prefix}_slice_str{i}")(chord) for i in range(len(string_features))]
    

    # Per string global max pooling and dense layers
    processed_strings = []
    for i, s in enumerate(split_chords):
        # Add residual connection from original string features
        #s = layers.Add(name=f"{name_prefix}_res_str{i}")([s, string_residuals[i]])
        s = layers.LayerNormalization(name=f"{name_prefix}_ln_str{i}")(s) 
        s = layers.LeakyReLU(name=f"{name_prefix}_act_str{i}")(s)
        s=layers.Dropout(0.1, name=f"{name_prefix}_drop_str{i}")(s)   
        s=layers.Dense(1,bias_initializer=tf.initializers.Constant(-4),name=f"{name_prefix}_fretlogits_str{i}", kernel_regularizer=reg)(s)   
        s=layers.Flatten(name=f"{name_prefix}_flatten_str{i}")(s)
        s = AppendSilence(name=f"{name_prefix}_append_silence_str{i}")(s)
        s=layers.Softmax(name=f"{name_prefix}_softmax_str{i}")(s)
        s = StripSilence(name=f"{name_prefix}_strip_silence_str{i}")(s)
        # s = layers.GlobalMaxPooling1D(name=f"{name_prefix}_gmax_str{i}")(s)
        # # s = layers.Dense(64, name=f"{name_prefix}_dense_str{i}", kernel_regularizer=reg)(s)
        # s = layers.LayerNormalization(name=f"{name_prefix}_ln_str{i}")(s) 
        # s = layers.LeakyReLU(name=f"{name_prefix}_act_str{i}")(s)
        # s=layers.Dropout(0.1, name=f"{name_prefix}_drop_str{i}")(s)
        # s=layers.Dense(output_dim, activation= None,
        #                 bias_initializer=tf.initializers.Constant(-4),
        #                 dtype='float32', name=f"{name_prefix}_output_str{i}")(s)
        processed_strings.append(s)
    return processed_strings


def build_1d_cnn_model(batch_sz=64, input_shape=(image_height, image_width),
                       output_dim=OUTPUT_DIM_NOTES, training=True):
    print("Image height: ", image_height)
    inputs = layers.Input(batch_shape=(batch_sz, *input_shape), name="input_spectrogram")
    local_mean = layers.AveragePooling2D(pool_size=(5, 1), strides=(1, 1), padding='same', name="local_mean")(inputs)
    x = layers.Subtract(name="local_contrast")([inputs, local_mean])
    # x=inputs
    # --- Stage 1: Frequency compression (B, 312, 256) → (B, 312, 512) ---
    x = layers.Reshape((image_height, 256, 1), name="reshape_to_2d")(x)
    x = layers.Conv2D(8, (1, 16), strides=(1, 4), padding='same',
                      kernel_initializer='he_normal', name="freq_compress_conv2d")(x)
    x = layers.BatchNormalization(name="freq_compress_bn")(x)
    x = layers.LeakyReLU(0.2, name="freq_compress_act")(x)
    x = layers.SpatialDropout2D(0.1, name="freq_compress_drop")(x)
    x = layers.Reshape((image_height, 512), name="reshape_to_1d")(x)

    # --- Stage 2: Conv1D backbone ---
    # x = layers.Conv1D(32, 1, padding='same', kernel_initializer='he_normal', name="backbone_squeeze", kernel_regularizer=reg)(x)
    # x = layers.BatchNormalization(name="backbone_squeeze_bn")(x)
    # x = layers.LeakyReLU(name="backbone_squeeze_act")(x)

    # x = layers.Conv1D(32, 8, padding='same', kernel_initializer='he_normal', name="backbone_conv1", kernel_regularizer=reg)(x)
    # x = layers.BatchNormalization(name="backbone_bn1")(x)
    # x = layers.LeakyReLU(name="backbone_act1")(x)
    # x = layers.SpatialDropout1D(0.2, name="backbone_drop1")(x)

    # x = layers.Conv1D(64, 8, padding='same', kernel_initializer='he_normal', name="backbone_conv2", kernel_regularizer=reg)(x)
    # x = layers.BatchNormalization(name="backbone_bn2")(x)
    # x = layers.LeakyReLU(name="backbone_act2")(x)
    # x = layers.MaxPooling1D(2, name="backbone_pool")(x)
    # x = layers.SpatialDropout1D(0.2, name="backbone_drop2")(x)
    # x = layers.Conv1D(64, 1, padding='same', kernel_initializer='he_normal', kernel_regularizer=reg, name="backbone_squeeze")(x)
    # x = layers.BatchNormalization(name="backbone_squeeze_bn")(x)
    # x = layers.LeakyReLU(name="backbone_squeeze_act")(x)

    #x = layers.MaxPooling1D(2, name="backbone_pool")(x)
    # x = layers.SpatialDropout1D(0.2, name="backbone_drop")(x)
    # --- Stage 3: Transformer ---
    x = transformer_block(x, num_heads=2, head_size=32, ff_dim=128, dropout=0.1, name_prefix="tfm_block1")

    num_pool_layers = 0
    max_x = image_height / (num_pool_layers + 1)
    print(f"Before string split: {x.shape}, max_x={max_x}")

    # --- Stage 4: String-aware slicing ---
    totalnotes = 37
    window_size = 13
    offsets = [0, 5, 10, 15, 19, 24]

    def slice_range(offset):
        s = int(offset / totalnotes * max_x)
        e = int((offset + window_size) / totalnotes * max_x)
        return s, e

    ranges = [slice_range(o) for o in offsets]
    ranges[-1] = (ranges[-1][0], int(max_x))
    print("String slice ranges (in time steps): ", ranges)
    string_features = [
        string_layer(x, s, e, max_x, training, string_idx=i)
        for i, (s, e) in enumerate(ranges)
    ]
    print("After string split: ", [s.shape for s in string_features])



    # --- Stage 5: Chord reasoning (Conv1D across strings) ---
    processed_strings = chord_conv_block(string_features,output_dim=N_FRETS,training=training, filters=64, kernel_size=(3,4), name_prefix="chord_block")

    combined = layers.Concatenate(name="string_combined")(processed_strings)

    # combined = layers.Dropout(0.0, name="final_dropout")(combined)

    # outputs = layers.Dense(output_dim, activation= None if training else 'sigmoid',
    #                     bias_initializer=tf.initializers.Constant(-1),
    #                     dtype='float32', name="output_notes")(combined)
    string_outputs = tf.keras.layers.Reshape((N_STRINGS, N_FRETS))(combined)
    outputs = SparseGuitarOutput(mask)(string_outputs)

    return models.Model(inputs, outputs, name="guitar_note_detector")


