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

    # The actual onset prediction output
    onsets_output = layers.Dense(output_dim_onsets, activation='sigmoid', dtype=tf.float32, name='onsets_output')(onsets_intermediate_features)

    # Now, to feed onsets info back into 'x' for notes,
    # you need to either transform onsets_intermediate_features to match x's spatial dims,
    # or you process 'x' further and then combine the global features.

    # Option 1a: If you want to use the high-level *dense* features of onsets to influence notes,
    # you should probably apply the final convolutional layer to 'x' first, THEN pool and concat
    # the pooled 'x' with 'onsets_intermediate_features' for the note branch.
    # This is often more effective for separate heads.

    # Your original model's structure was more like this:
    # x (shared feature extractor) -> GlobalAveragePooling -> Dense (note_output)
    # x (shared feature extractor) -> GlobalAveragePooling -> Dense (onsets_output)
    # This is a valid multi-head approach where both heads see the same features from 'x'.

    # If you truly want to concatenate the onset info *back into the convolutional path*:
    # This is generally tricky because onsets_output is (None, 1) and x is (None, H, W, C)
    # You'd need to expand onsets_output to match the spatial dimensions of x, which is not trivial
    # and might not be semantically meaningful.

    # A more common approach if you want shared features to influence each other's *further processing*
    # is to have a shared encoder, then split, process, and potentially *merge* again at a later dense layer.

    # Let's revert to your working model's logic for the output branches and reconsider the concatenation.
    # The error "A `Concatenate` layer should be called on a list of inputs. Received: inputs=<KerasTensor shape=(None, 64, 78, 64), dtype=float16, sparse=False, name='tf.math.truediv_1/output:0'>"
    # is because you tried to concatenate a 4D feature map with a 2D scalar output.

    # --- Reverting to a more standard multi-head approach or a carefully designed concat ---

    # Let's assume you want the *features before the final onset prediction* to influence the *features for note prediction*.
    # You cannot concatenate `onsets_output` (which is already a final prediction) with `x` (a feature map).
    # You also can't concatenate `onsets_intermediate_features` (2D) with `x` (4D) directly.

    # If the goal is for the *intermediate spatial features* from the onset branch to influence the note branch:
    # You would need an architecture like this, where the 'onsets_output_branch_before_pooling' is *not* pooled yet.
    # But your original model pools `x` for both branches.

    # The most likely scenario where concatenation makes sense in a multi-task model like this
    # is if you're concatenating high-level *extracted features* from different branches,
    # or if you upsample the 'onsets_intermediate_features' to match 'x's spatial shape.
    # Upsampling a (None, 64) tensor back to (None, 64, 78, 64) would involve Dense -> Reshape -> UpSampling2D or Conv2DTranspose.
    # This is generally overkill and often not the best architectural choice for this problem.

    # Let's modify the build_cnn_model to correctly perform concatenation, assuming you want to
    # merge *global features* (after pooling) for further processing, before the final note output.

    # --- Shared Feature Extractor (up to where 'x' is 4D) ---
    # ... (your existing shared layers for x)
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

    x = layers.Conv2D(filters=128, kernel_size=(7, 7), padding='same', activation=None)(x_conv_features) # Use x_conv_features
    x = layers.BatchNormalization()(x)
    x = layers.Activation(activation='relu')(x)
    x = layers.MaxPooling2D(pool_size=(2, 2), strides=(2, 2))(x)
    x = layers.SpatialDropout2D(0.3)(x)

    # --- Branch for Note prediction ---
    # This branch processes the features 'x' from the (now deeper) shared extractor
    note_output_branch = layers.GlobalAveragePooling2D()(x) # Use the further processed 'x'
    note_output_branch = layers.Dropout(0.4)(note_output_branch)

    # Now, concatenate the onset *features* with the note *features* before the final dense layer.
    # onsets_pooled_features is (None, 64)
    # note_output_branch is (None, 128) (due to 128 filters in the last Conv2D)
    # These shapes are compatible for concatenation along axis=-1
    concatenated_for_note_final = layers.concatenate([note_output_branch, onsets_pooled_features], axis=-1)

    note_output = layers.Dense(output_dim_notes, activation='sigmoid', dtype=tf.float32, name='note_output')(concatenated_for_note_final)

    return Model(inputs=inputs, outputs=[note_output, onsets_output])