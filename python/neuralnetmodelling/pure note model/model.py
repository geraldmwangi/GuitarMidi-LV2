from tensorflow.keras import layers, models, optimizers, Model
import tensorflow as tf



def build_cnn_model(input_shape, output_dim_notes):
    model = models.Sequential()
    # Input Layer
    model.add(layers.Input(shape=input_shape))

    # 2D CNN Block 1
    model.add(layers.Conv2D(filters=32, kernel_size=(3, 3), padding='same', activation='relu'))
    model.add(layers.BatchNormalization())
    model.add(layers.MaxPooling2D(pool_size=(2, 2), strides=(2, 2)))
    model.add(layers.SpatialDropout2D(0.2))
    # 2D CNN Block 2
    model.add(layers.Conv2D(filters=64, kernel_size=(3, 3), padding='same', activation='relu'))
    model.add(layers.BatchNormalization())
    model.add(layers.MaxPooling2D(pool_size=(2, 2), strides=(2, 2)))
    model.add(layers.SpatialDropout2D(0.25))
    # 2D CNN Block 3
    model.add(layers.Conv2D(filters=256, kernel_size=(3, 3), padding='same', activation='relu'))
    model.add(layers.BatchNormalization())
    model.add(layers.MaxPooling2D(pool_size=(2, 2), strides=(2, 2)))
    model.add(layers.SpatialDropout2D(0.3))

    # Global Pooling
    model.add(layers.GlobalAveragePooling2D())
    model.add(layers.Dropout(0.4))

    # Output Layer for MIDI notes - REMOVED 'name='note_output''
    model.add(layers.Dense(output_dim_notes, activation='sigmoid'))

    return model

# def build_cnn_model(input_shape, output_dim_notes): # Removed output_dim_onsets

#     inputs = layers.Input(shape=input_shape, dtype=tf.float32, name='input_features')

#     # --- Shared Feature Extractor ---
#     x = layers.Conv2D(filters=32, kernel_size=(3,3), padding='same', activation=None)(inputs)
#     x = layers.BatchNormalization()(x)
#     x = layers.Activation(activation='relu')(x)
#     x = layers.MaxPooling2D(pool_size=(2, 2), strides=(2, 2))(x)
#     x = layers.SpatialDropout2D(0.2)(x)

#     x = layers.Conv2D(filters=64, kernel_size=(3,3), padding='same', activation=None)(x)
#     x = layers.BatchNormalization()(x)
#     x = layers.Activation(activation='relu')(x)
#     x = layers.MaxPooling2D(pool_size=(2, 2), strides=(2, 2))(x)
#     x = layers.SpatialDropout2D(0.25)(x)
    
#     # --- Continue shared feature extraction for the Note branch ---
#     x = layers.Conv2D(filters=128, kernel_size=(3, 3), padding='same', activation=None)(x) # Use x_conv_features
#     x = layers.BatchNormalization()(x)
#     x = layers.Activation(activation='relu')(x)
#     x = layers.MaxPooling2D(pool_size=(2, 2), strides=(2, 2))(x)
#     x = layers.SpatialDropout2D(0.3)(x)

#     # --- Branch for Note prediction ---
#     note_output_branch_features = layers.GlobalAveragePooling2D()(x)
#     note_output_branch_features = layers.Dropout(0.4)(note_output_branch_features)

#     # No concatenation needed as onsets branch is removed
#     note_output = layers.Dense(output_dim_notes, activation='sigmoid', dtype=tf.float32, name='note_output')(note_output_branch_features)

#     # Model now has only one output
#     return Model(inputs=inputs, outputs=note_output)