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


# class FASTBlock(layers.Layer):
#     """
#     A single FAST (MobileViT-inspired) block for Audio Spectrograms.
    
#     This block integrates CNN-based local processing with Transformer-based 
#     global processing and subsequent fusion.
#     """
#     def __init__(self, dim, patch_size=(16, 16), num_heads=12, name='fast_block', **kwargs):
#         super().__init__(name=name, **kwargs)
#         self.dim = dim
#         self.ph, self.pw = patch_size
        
#         # --- Local Feature Extraction ---
#         # 3x3 Conv + 1x1 Point-wise Conv (to increase channels to `dim`)
#         self.conv_3x3 = layers.Conv2D(dim, kernel_size=3, padding='same', use_bias=False)
#         self.conv_1x1_local = layers.Conv2D(dim, kernel_size=1, padding='same')
        
#         # --- Global Feature Extraction (Transformer) ---
#         # NOTE: The paper uses a custom CenterNorm and Scaled Cosine Similarity Attention (SCSA).
#         # We use standard LayerNormalization and MultiHeadAttention as structural placeholders.
#         # The user MUST substitute these with the custom Lipschitz-Aware layers for the official FAST model.
        
#         self.norm1 = layers.LayerNormalization(epsilon=1e-6) # Placeholder for CenterNorm
#         self.mha = layers.MultiHeadAttention(num_heads=num_heads, key_dim=dim // num_heads, dropout=DROPOUT_RATE) # Placeholder for SCSA
#         self.norm2 = layers.LayerNormalization(epsilon=1e-6) # Placeholder for CenterNorm
        
#         self.mlp = tf.keras.Sequential([
#             layers.Dense(dim * 4, activation=tf.nn.gelu),
#             layers.Dense(dim),
#             layers.Dropout(DROPOUT_RATE)
#         ], name="mlp")
        
#         # --- Feature Fusion ---
#         self.conv_1x1_fusion = layers.Conv2D(dim, kernel_size=1, padding='same')

#     def call(self, inputs, training=False):
#         B, H, W, C = tf.shape(inputs)[0], tf.shape(inputs)[1], tf.shape(inputs)[2], tf.shape(inputs)[3]
        
#         # 1. Local Representation (CNN-based)
#         # 3x3 Conv -> Activation (ReLU) -> 1x1 Conv
#         x_local = self.conv_3x3(inputs)
#         x_local = layers.ReLU()(x_local)
#         x_local = self.conv_1x1_local(x_local) # Output shape: (B, H, W, dim)
        
#         # 2. Global Representation (Transformer-based)
#         # Unroll into patches
#         # Reshape (B, H, W, dim) -> (B * N_patches, Ph, Pw, dim)
#         x_unrolled = tf.reshape(x_local, (B * (H // self.ph) * (W // self.pw), self.ph, self.pw, self.dim))
        
#         # Flatten patches (B * N_patches, Ph, Pw, dim) -> (B * N_patches, Ph*Pw, dim)
#         x_patches = tf.reshape(x_unrolled, (-1, self.ph * self.pw, self.dim))
        
#         # Transformer Block (MHA + MLP)
#         x_norm = self.norm1(x_patches)
#         attn_output = self.mha(x_norm, x_norm)
        
#         # Add residual connection (NOTE: Paper uses Weighted Residual Shortcuts)
#         x_transformer = x_patches + attn_output
#         x_transformer = x_transformer + self.mlp(self.norm2(x_transformer))
        
#         # Re-roll patches back to feature map
#         # Reshape (B * N_patches, Ph*Pw, dim) -> (B, H, W, dim)
#         x_rerolled = tf.reshape(x_transformer, (B, H, W, self.dim))

#         # 3. Feature Fusion (CNN-based)
#         # Output is combined with the Local Representation through a 1x1 Conv
#         fused_output = self.conv_1x1_fusion(x_rerolled + x_local)
        
#         return fused_output
    

# def create_fast_model(input_shape=(IMG_H, IMG_W, CHANNELS), num_classes=NUM_CLASSES, 
#                       hidden_dim=HIDDEN_D, patch_size=(PATCH_H, PATCH_W), 
#                       transformer_layers=TRANSFORMER_LAYERS):
#     """
#     Constructs the overall FAST Model with corrected CNN stem for perfect feature map division.
#     """
#     inputs = layers.Input(shape=input_shape)
#     x = inputs

#     # --- CNN Stem (Corrected for Divisibility) ---
    
#     # 1. First Downsample (312 -> 156, 256 -> 128) - Stride 2
#     x = layers.Conv2D(32, kernel_size=3, strides=2, padding='same', use_bias=False)(x)
#     x = layers.BatchNormalization()(x)
#     x = layers.ReLU()(x)
    
#     # 2. Second Downsample (156 -> 78, 128 -> 64) - Stride 2
#     x = layers.Conv2D(hidden_dim // 2, kernel_size=3, strides=2, padding='same', use_bias=False)(x)
#     x = layers.BatchNormalization()(x)
#     x = layers.ReLU()(x)
    
#     # 3. Third Downsample (78 -> 72, 64 -> 64) - Stride is set to 1 for Height, but we need 78 to go to 72.
#     # To get 78 -> 72, we need a separate block, or we modify the previous strides.
#     # To simplify and ensure perfect divisibility: use three downsampling blocks where:
#     # 312 -> 156 (S=2) -> 78 (S=2) -> 72 (Custom, padding/valid conv)
    
#     # *Alternative Fix: Let's use 3 Conv blocks to reach a manageable (78, 64) and rely on the model 
#     # to handle the slight indivisibility or use a final 1x1 Conv with padding:
    
#     # Let's trust the MobileViT structure, which forces the output shape to be divisible:
#     # 312 -> 78, 256 -> 64. (Final FM Shape: 78x64)
#     # The fix is to add a small VALID padding/cropping layer to get 78 -> 72.
    
#     # Crop layer to make 78 -> 72 (72 is divisible by 24)
#     # 78 - 72 = 6. Crop 3 from top, 3 from bottom.
#     cropping_pixels_h = 3
#     x = layers.Cropping2D(cropping=((cropping_pixels_h, cropping_pixels_h), (0, 0)))(x)
    
#     # Final Feature Map Shape entering FASTBlock is (72, 64) - divisible by (24, 16)!
#     print(f"--- Input to FASTBlock will have a feature map size of: {x.shape[1]}x{x.shape[2]} ---")
    
#     # --- Stacked FAST Blocks ---
#     for i in range(transformer_layers):
#         x = FASTBlock(dim=hidden_dim, patch_size=patch_size, num_heads=NUM_HEADS, name=f'fast_block_{i}')(x)
        
#     # --- Classification Head ---
#     x = layers.GlobalAveragePooling2D()(x)
#     x = layers.Dropout(0.5)(x)
    
#     # Output activation: 'sigmoid' for multi-label (common for large audio datasets)
#     output_activation = 'sigmoid' if num_classes > 2 else 'softmax'

#     outputs = layers.Dense(num_classes, activation=output_activation)(x)

#     # --- Model Compilation ---
#     model = Model(inputs, outputs, name="FAST_AudioSpectrogramTransformer")
    
#     return model

# # Create and summarize the model
# fast_model = create_fast_model()
# fast_model.summary()