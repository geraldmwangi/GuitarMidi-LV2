from tensorflow.keras import layers, models, optimizers, Model
import tensorflow as tf
# --- 1. Define the Pure CNN Model (with two outputs) ---

def build_cnn_model(input_shape, output_dim_notes, output_dim_onsets):
    inputs = layers.Input(shape=input_shape, dtype=tf.float32, name='input_features')

    # --- Shared Feature Extractor ---
    x = layers.Conv2D(filters=32, kernel_size=(7,7), padding='same', activation=None)(inputs)
    x = layers.BatchNormalization()(x)
    x = layers.Activation(activation='relu')(x)
    x = layers.MaxPooling2D(pool_size=(2, 2), strides=(2, 2))(x)
    x = layers.SpatialDropout2D(0.2)(x)

    x = layers.Conv2D(filters=64, kernel_size=(7,7), padding='same', activation=None)(x)
    x = layers.BatchNormalization()(x)
    x = layers.Activation(activation='relu')(x)
    x = layers.MaxPooling2D(pool_size=(2, 2), strides=(2, 2))(x)
    x = layers.SpatialDropout2D(0.25)(x)

    # --- Branch for Onsets prediction (from current 'x') ---
    onsets_intermediate_features = layers.GlobalAveragePooling2D()(x) # Shape: (None, 64)
    onsets_intermediate_features = layers.Dropout(0.4)(onsets_intermediate_features)




    x_conv_features = x # Renaming for clarity, this is the 4D output after the second max pooling and dropout

    # --- Branch for Onsets prediction ---
    # Global Average Pooling on the shared features
    onsets_pooled_features = layers.GlobalAveragePooling2D()(x_conv_features) # (None, 64)
    onsets_pooled_features = layers.Dropout(0.4)(onsets_pooled_features)
    onsets_output = layers.Dense(output_dim_onsets, activation='sigmoid', dtype=tf.float32, name='onsets_output')(onsets_pooled_features)

    # --- Prepare features for Note prediction ---
    # If you want onset-related information to influence the *final stages* of note prediction,
    # the most common way is to concatenate *after* pooling both.

    note_pooled_features = layers.GlobalAveragePooling2D()(x_conv_features) # (None, 64)
    note_pooled_features = layers.Dropout(0.4)(note_pooled_features)

    # Now, if you want the high-level *pooled* features that led to onsets
    # to be *concatenated* with the high-level *pooled* features that lead to notes,
    # you can concatenate `note_pooled_features` with `onsets_pooled_features`.
    # This assumes 'onsets_output' itself isn't used directly in this concatenation,
    # but rather the features from which it was derived.

    # This is likely what you meant by "passing onset info to note branch":
    # Concatenate the pooled features that are about to go into the final dense layers.
    combined_features_for_notes = layers.concatenate([note_pooled_features, onsets_pooled_features], axis=-1)
    # The shape will be (None, 64 + 64) = (None, 128)

    # Apply the next Conv2D and MaxPooling *before* GlobalAveragePooling for notes
    # if you want deeper conv layers for notes specifically.
    # But your current model has GlobalAveragePooling right after the shared layers.

    # Given your current structure, the most logical place to "concat two layers"
    # to let onsets info influence notes (beyond shared features) would be:
    # 1. Take the `onsets_pooled_features` (shape `(None, 64)`)
    # 2. Take the `note_pooled_features` (shape `(None, 64)`)
    # 3. Concatenate these two 2D tensors before the final `Dense` layer for notes.

    # Let's adjust your model code to reflect this architectural pattern.

    # Remove the problematic concatenation and the extra Conv2D block in the middle
    # as it's structurally unusual to inject a 2D tensor back into a 4D Conv2D path directly.



    # Now, concatenate the onset *features* with the note *features* before the final dense layer.
    # onsets_pooled_features is (None, 64)
    # note_output_branch is (None, 128) (due to 128 filters in the last Conv2D)
    # These shapes are compatible for concatenation along axis=-1
    concatenated_for_note_final = layers.concatenate([note_output_branch, onsets_pooled_features], axis=-1)

    note_output = layers.Dense(output_dim_notes, activation='sigmoid', dtype=tf.float32, name='note_output')(concatenated_for_note_final)

    return Model(inputs=inputs, outputs=[note_output, onsets_output])