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
# Input: (Batch, Filters, Time)
    inputs = layers.Input(batch_shape=(batch_sz, *input_shape))
    
        # 1. Temporal Compression: Keep some temporal info rather than just 'max'
    # We use a large stride to reduce 256 -> 32 while learning features
    x = layers.Reshape((312, 256, 1))(inputs)
    x = layers.Conv2D(16, (1, 16), strides=(1, 8), padding='same')(x)
    x = layers.LeakyReLU(0.2)(x)
    
    # Flatten time into features so we can use Conv1D on filters
    # Shape: (Batch, 312, 16 * 32)
    x = layers.Reshape((312, 512))(x)
    print(f"Initial input shape: {x.shape}")
    # 2. Time-Domain Processing (per filter)
    # We use a small 2D kernel to look at neighboring filters and time
    x = layers.Conv1D(32, 5, padding='same', activation=None)(x)
    x = layers.BatchNormalization()(x)
    x = layers.LeakyReLU()(x)
    # x=layers.MaxPooling1D(2)(x)
    x = layers.SpatialDropout1D(0.3)(x, training=training)
    x = layers.Conv1D(64, 5, padding='same', activation=None)(x)
    x = layers.BatchNormalization()(x)
    x = layers.LeakyReLU()(x)
    x=layers.MaxPooling1D(2)(x)
    x = layers.SpatialDropout1D(0.3)(x, training=training)
    print(f"After first Conv2D: {x.shape}")
    #x = layers.MaxPooling2D((1, 4))(x) # Reduce time, keep filter resolution
    # print(f"After first Conv2D and MaxPooling2D: {x.shape}")
    # 3. String-Specific Partitioning
    # Instead of manual slicing at the END, slice now.
    # Assuming 312 filters / 6 strings = 52 filters per string
    string_features = []
    for i in range(6):
        start=i*26
        end=(i+1)*26
        print(f"Extracting string {i+1} from filters {start} to {end}")
        s = layers.Lambda(lambda y, st=start, en=end: y[:, st:en, :])(x)
        print(f"String {i+1} section shape: {s.shape}")
        # String-specific processing
 # String-specific processing with Dilation to capture temporal shape
        s = layers.Conv1D(128, 5, padding='same', dilation_rate=1)(s)
        s = layers.LeakyReLU(0.2)(s)
        # s = layers.Conv1D(64, 5, padding='same', dilation_rate=2)(s) # Sees further in time
        s = layers.BatchNormalization()(s)

        print(f"String {i+1} after first Conv1D: {s.shape}")
        s=layers.MaxPooling1D(4)(s)
        s = layers.SpatialDropout1D(0.3)(s, training=training)



        s_max = layers.GlobalMaxPooling1D()(s)
        # s_avg = layers.GlobalAveragePooling1D()(s)
        # s_combined = layers.Concatenate()([s_max, s_avg])
        string_features.append(s_max)#(s_combined)
    
    # 4. Recombine for Note Classification
    concat = layers.Concatenate()(string_features)
    # x = layers.Dense(256, activation='relu')(concat)
    concat = layers.Dropout(0.4)(concat, training=training)
    outputs = layers.Dense(output_dim, activation='sigmoid',dtype='float32')(concat)
    
    return models.Model(inputs, outputs)
