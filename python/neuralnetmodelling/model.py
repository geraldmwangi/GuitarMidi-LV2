from tensorflow.keras import layers, models, optimizers, Model
import tensorflow as tf
# --- 1. Define the Pure CNN Model (with two outputs) ---

def convolutional_layer(inputs,filters, dropout):
    x = layers.Conv2D(filters=filters, kernel_size=(3,3), padding='same', activation=None)(inputs)
    x = layers.BatchNormalization()(x)
    x = layers.Activation(activation='relu')(x)
    x = layers.MaxPooling2D(pool_size=(2, 2), strides=(2, 2))(x)
    x = layers.SpatialDropout2D(dropout)(x)  
    return x

def build_cnn_model(input_shape, output_dim_notes, output_dim_onsets):
    inputs = layers.Input(shape=input_shape, dtype=tf.float32, name='input_features')

    # --- Shared Feature Extractor ---
    x=convolutional_layer(inputs,32,0.2)
    # --- Branch for Onsets prediction (from current 'x') ---
    onsets_intermediate_features = layers.GlobalAveragePooling2D()(x) # Shape: (None, 64)
    onsets_intermediate_features = layers.Dropout(0.4)(onsets_intermediate_features)

    # The actual onset prediction output
    onsets_output = layers.Dense(output_dim_onsets, activation='sigmoid', dtype=tf.float32, name='onsets_output')(onsets_intermediate_features)
   # x = layers.concatenate([x, onsets_intermediate_features], axis=-1)
    x=convolutional_layer(x,64,0.25)
    x=convolutional_layer(x,128,0.3)

    # --- Branch for Note prediction ---
    # This branch processes the features 'x' from the (now deeper) shared extractor
    note_output_branch = layers.GlobalAveragePooling2D()(x) # Use the further processed 'x'
    note_output_branch = layers.Dropout(0.4)(note_output_branch)

    #note_output_branch = layers.concatenate([note_output_branch, onsets_intermediate_features], axis=-1)

    note_output = layers.Dense(output_dim_notes, activation='sigmoid', dtype=tf.float32, name='note_output')(note_output_branch)

    return Model(inputs=inputs, outputs=[note_output, onsets_output])