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

def string_layer(x,start,end,max_x,training):
    end=min(end,max_x)
    start=max(0,start)
# 1. Frequency Slice
    s = layers.Lambda(lambda y, st=start, en=end: y[:, st:en, :])(x)
    
    # 2. String-specific processing
    s = layers.Conv1D(128, 7, padding='same', kernel_initializer='he_normal')(s)
    s = layers.LayerNormalization()(s)
    s = layers.LeakyReLU()(s)
    
    # --- FIXED FEATURE POOLING ---
    # We want to reduce the 128 filters to 32 (or 256 to 64 later)
    # We reshape to (Batch, Time, New_Feature_Dim, Pool_Size)
    current_channels = s.shape[-1] # 128
    s = layers.Reshape((-1, current_channels // 4, 4))(s)
    # Max pool across the last axis (the groups of 4 features)
    s = layers.Lambda(lambda t: tf.reduce_mean(t, axis=3))(s) 
    # Resulting shape: (Batch, Time, 32)
    
    s = layers.SpatialDropout1D(0.3)(s)
        
    s = layers.Conv1D(256, 7, padding='same', kernel_initializer='he_normal')(s)
    s = layers.LayerNormalization()(s)
    s = layers.LeakyReLU()(s)
        
    # Repeat Feature Pooling for the 256 -> 64 reduction
    s = layers.Reshape((-1, 64, 4))(s)
    smax = layers.Lambda(lambda t: tf.reduce_max(t, axis=3))(s)
    smean=layers.Lambda(lambda t: tf.reduce_mean(t, axis=3))(s)
    # Resulting shape: (Batch, Time, 64)
    s=layers.Add()([smax,smean])

    return layers.GlobalAveragePooling1D()(s)
def transformer_block(x, num_heads=4, head_size=64, ff_dim=512, dropout=0.2):
    """
    A lightweight Transformer layer designed for real-time audio feature maps.
    x shape: (Batch, Sequence_Length, Embedding_Dim)
    """
    # 1. Multi-Head Self-Attention
    # This allows the model to look 'across' frequency bins for harmonics
    attn_output = layers.MultiHeadAttention(
        num_heads=num_heads, 
        key_dim=head_size, 
        dropout=dropout
    )(x, x)
    
    # Residual Connection + Layer Norm
    x1 = layers.Add()([x, attn_output])
    x1 = layers.LayerNormalization(epsilon=1e-6)(x1)
    
    # 2. Feed-Forward Network (Position-wise)
    # We use a smaller ff_dim (512) to keep the parameter count lean
    ffn = layers.Dense(ff_dim, activation="relu")(x1)
    ffn = layers.Dense(x.shape[-1])(ffn) # Project back to original dim
    ffn = layers.Dropout(dropout)(ffn)
    
    # Second Residual Connection + Layer Norm
    x2 = layers.Add()([x1, ffn])
    return layers.LayerNormalization(epsilon=1e-6)(x2)

def build_1d_cnn_model(batch_sz=64, input_shape=(image_height, image_width), output_dim=OUTPUT_DIM_NOTES, training=True,
                       gru_units=128, gru_layers=1, bidirectional=True, stateful=False):  # Added GRU params
# Input: (Batch, Filters, Time)
    inputs = layers.Input(batch_shape=(batch_sz, *input_shape))
    
        # 1. Temporal Compression: Keep some temporal info rather than just 'max'
    # We use a large stride to reduce 256 -> 32 while learning features
    x = layers.Reshape((image_height, 256, 1))(inputs)
    x = layers.Conv2D(16, (1, 16), strides=(1, 8), padding='same',kernel_initializer='he_normal')(x)
    x = layers.BatchNormalization()(x)
    x = layers.LeakyReLU(0.2)(x)
    x = layers.SpatialDropout2D(0.2)(x)
    # Flatten time into features so we can use Conv1D on filters
    # Shape: (Batch, 312, 16 * 32)
    x = layers.Reshape((image_height, 512))(x)
    # x=layers.Lambda(lambda x: tf.reduce_max(x, axis=2))(inputs)

    x = transformer_block(x, num_heads=2, head_size=64, ff_dim=256, dropout=0.2)

    # x=layers.Normalization(axis=-1)(x)
    # x=layers.Lambda(lambda x: tf.math.log(tf.abs(x) + 1e-4))(x)
    print(f"Initial input shape: {x.shape}")
    # 2. Time-Domain Processing (per filter)
    # We use a small 2D kernel to look at neighboring filters and time
    x = layers.Conv1D(32, 7, padding='same', activation=None,kernel_initializer='he_normal')(x)
    x = layers.BatchNormalization()(x)
    x = layers.LeakyReLU()(x)
    # x=layers.MaxPooling1D(2)(x)
    x = layers.SpatialDropout1D(0.2)(x)
    
    x = layers.Conv1D(64, 7, padding='same', activation=None,kernel_initializer='he_normal')(x)
    x = layers.BatchNormalization()(x)
    x = layers.LeakyReLU()(x)
    x=layers.MaxPooling1D(2)(x)
    x = layers.SpatialDropout1D(0.2)(x)
    num_pool_layers=1
    max_x=image_height/(num_pool_layers+1)
    
    print(f"After first Conv2D: {x.shape}")
    # total notes 3*5+4+5+13=37
    # 4*5+4*5+4*5+4*4+4*5+4*13=148
    # [0:20]+[20:40]+[40:60]+[60:76]+[76:96]+[96:148]
    totalnotes=37
    window_size = 13

    # E string (Low)
    offset = 0
    E_begin = offset / totalnotes
    E_end = (window_size + offset) / totalnotes

    # A string (Offset 5)
    offset += 5
    A_begin = offset / totalnotes
    A_end = (window_size + offset) / totalnotes

    # d string (Offset 5)
    offset += 5
    d_begin = offset / totalnotes
    d_end = (window_size + offset) / totalnotes

    # g string (Offset 5)
    offset += 5
    g_begin = offset / totalnotes
    g_end = (window_size + offset) / totalnotes

    # B string (Offset 4 - The Major Third shift)
    offset += 4
    b_begin = offset / totalnotes
    b_end = (window_size + offset) / totalnotes

    # e string (High - Offset 5)
    offset += 5
    e_begin = offset / totalnotes
    e_end = (window_size + offset) / totalnotes # Final end is 24 + 13 = 37 (1.0)
    string_features = []
    Estr=string_layer(x,E_begin,int(E_end*max_x),max_x,training)
    Astr=string_layer(x,int(A_begin*max_x)+1,int(A_end*max_x),max_x,training)
    dstr=string_layer(x,int(d_begin*max_x)+1,int(d_end*max_x),max_x,training)
    gstr=string_layer(x,int(g_begin*max_x)+1,int(g_end*max_x),max_x,training)
    bstr=string_layer(x,int(b_begin*max_x)+1,int(b_end*max_x),max_x,training)
    estr=string_layer(x,int(e_begin*max_x)+1,int(max_x-1),max_x,training)

    



    string_features = [Estr,Astr,dstr,gstr,bstr,estr]
# 1. Stack into a 2D Map: (Batch, 6, 256)
    stacked = layers.Lambda(lambda x: tf.stack(x, axis=1))(string_features)
    
    # 2. Reshape for 2D Conv: (Batch, 6, 256, 1)
    stacked_2d = layers.Reshape((6, 64, 1))(stacked)
    
    # 3. Chord Logic Conv2D: Looks at 3 strings at a time (e.g., power chords/triads)
    x = layers.Conv2D(32, kernel_size=(3, 7), padding='same')(stacked_2d)
    x = layers.BatchNormalization()(x)
    x = layers.LeakyReLU()(x)
    x = layers.SpatialDropout2D(0.2)(x)
    x= layers.Conv2D(8,kernel_size=(1,4),strides=(1,4),padding='same',activation=None)(x)
    # 4. Final Classification
    concat = layers.Flatten()(x)
    concat = layers.Dropout(0.4)(concat)
    outputs = layers.Dense(output_dim, activation='sigmoid',dtype='float32')(concat)
    
    return models.Model(inputs, outputs)


