import tensorflow as tf
from tensorflow.keras import layers, models
from tensorflow.keras.models import Model
from common import OUTPUT_DIM_NOTES,image_height,image_width
    # --- Configuration for your specific input ---
IMG_H, IMG_W = 312, 256  
NUM_CLASSES = 89        
PATCH_H, PATCH_W = 24, 16 # Chosen patch size
CHANNELS = 1            
HIDDEN_D = 384           # Chosen hidden dimension
TRANSFORMER_LAYERS = 4   # Chosen layer count
NUM_HEADS = 6            # Chosen number of heads (divisor of 384)
DROPOUT_RATE = 0.1
def partitioned_average_pooling(x):
    splits = tf.split(x, [7,7,7,6,6,6], axis=1)   # sum=39 height slices
    pooled = [tf.reduce_mean(part, axis=[1,2]) for part in splits]
    return tf.concat(pooled, axis=1)

def partitioned_average_pooling_1d(x):
    splits = tf.split(x, [7,7,7,6,6,6], axis=1)   # sum=39 height slices
    pooled = [tf.reduce_mean(part, axis=[1]) for part in splits]
    return tf.concat(pooled, axis=1)

def build_1d_cnn_model(batch_sz=64, input_shape=(image_height, image_width), output_dim=OUTPUT_DIM_NOTES, training=True,
                       gru_units=128, gru_layers=1, bidirectional=True, stateful=False):  # Added GRU params
# Input shape: (Batch, 312, 256)
    inputs = layers.Input(batch_shape=(batch_sz, *input_shape))
    
    # 1. Initial Temporal Compression
    # We reduce the 256 time-samples to 32 to keep the "shape" of the attack
    x = layers.Reshape((312, 256, 1))(inputs)
    x = layers.Conv2D(16, (1, 16), strides=(1, 8), padding='same')(x)
    x = layers.BatchNormalization()(x)
    x = layers.LeakyReLU(0.2)(x) 
    # Current Shape: (Batch, 312, 32, 16)

    # 2. Harmonic Stacking Reshape
    # We turn the 312 filters into (6 Strings, 13 Frets, 4 Harmonics)
    # This allows the model to look at all 4 harmonics of a note simultaneously
    x = layers.Reshape((6, 13, 4, 32, 16))(x)
    
    # Flatten the 'Time' and 'Features' dimensions for convolution
    # New Shape: (Batch, 6, 13, 4, 512)
    x = layers.Reshape((6, 13, 4, 512))(x)

    string_outputs = []
    
    # 3. String-Specific Processing (Branching)
    for i in range(6):
        # Slice one string: (Batch, 13 Frets, 4 Harmonics, 512 Features)
        s = layers.Lambda(lambda y, idx=i: y[:, idx, :, :, :])(x)
        
        # Cross-Harmonic Convolution
        # Kernel (1, 4) looks across all 4 harmonics for each fret
        s = layers.Conv2D(64, (1, 4), padding='valid')(s)
        s = layers.LeakyReLU(0.2)(s)
        
        # Fret-Wise Processing
        # Kernel (3, 1) looks at neighboring frets (helpful for slides/vibrato)
        s = layers.Conv2D(128, (3, 1), padding='same')(s)
        s = layers.BatchNormalization()(s)
        s = layers.SpatialDropout2D(0.2)(s, training=training)
        
        # Reduce to a per-string feature vector
        s = layers.GlobalMaxPooling2D()(s)
        string_outputs.append(s)

    # 4. Global Feature Integration
    concat = layers.Concatenate()(string_outputs)
    x = layers.Dense(512)(concat)
    x = layers.LeakyReLU(0.2)(x)
    x = layers.Dropout(0.4)(x, training=training)
    
    # 5. Output Head
    # Using 1e-4 epsilon for mixed_float16 stability
    outputs = layers.Dense(output_dim, activation='sigmoid', dtype='float32')(x)
    
    model = models.Model(inputs, outputs)
    return model
