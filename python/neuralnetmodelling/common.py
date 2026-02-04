import  os
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
import tensorflow as tf
from fretboard import FretBoard
# Common parameters
frame_size=256
image_width = 256
image_height = 312 # Assuming this is your updated 288+some context/padding, or just 288 filter outputs
num_channels = 1
num_classes = 129 # For MIDI notes+silence class
INPUT_SHAPE = (image_height,image_width, num_channels)
INPUT_SHAPE_AUDIO = (1,image_width, num_channels)
OUTPUT_DIM_NOTES = num_classes # For notes output
OUTPUT_DIM_ONSETS = 1 # For onsets output
SAMPLERATE=48000
from scipy import signal
from fretboard import FretBoard
fretboard=FretBoard(17.5,SAMPLERATE)



def create_static_mask(fretboard_obj, num_samples, sample_rate):
    freq_bins = num_samples // 2 + 1
    f = np.linspace(0, sample_rate / 2, freq_bins)
    mask = []
    for fret in fretboard_obj.frets:
        for string in fret.strings:
            for filt in string.harmonics:
                _, h = signal.freqz(filt.b, filt.a, worN=f, fs=sample_rate)
                mask.append(np.abs(h)**2) # Simulates filtfilt
    return tf.constant(np.array(mask), dtype=tf.complex64)

# Global constant
FILTER_MASK = create_static_mask(fretboard, INPUT_SHAPE_AUDIO[1], SAMPLERATE)
print("Filter mask created with shape:", FILTER_MASK.shape)
# def fast_gpu_map(ipath,training=True):
#     parsed = tf.io.parse_single_example(ipath, feature_description)
#     audio = tf.io.decode_raw(parsed["input"], tf.float32)
#     label = tf.io.decode_raw(parsed["output"], tf.int8)
    
#     if training:
#         audio, label = augment_audio(audio, label)
    
#     # --- Vectorized Filtering ---
#     audio_fft = tf.signal.rfft(audio) 
#     filtered_fft = FILTER_MASK * tf.cast(audio_fft, tf.complex64)
#     envelopes = tf.abs(tf.signal.irfft(filtered_fft)) # Shape: [312, 256]
    
#     # --- Vectorized Normalization ---
#     # We find the peak activation across all filters for this specific window
#     # max_val = tf.reduce_max(envelopes)
    
#     # If the peak is > 0.1, we scale the whole tensor down so the peak is 1.0
#     # Using tf.where prevents division by zero and applies the condition element-wise
#     # envelopes = tf.cond(
#     #     max_val > 0.1,
#     #     lambda: envelopes / max_val, 
#     #     lambda: envelopes
#     # )
    
#     # Reshape for CNN
#     input_tensor = tf.cast(envelopes, tf.float32)
#     input_tensor = tf.expand_dims(input_tensor, axis=-1)
#     output_tensor = tf.cast(tf.reshape(label, [OUTPUT_DIM_NOTES]), tf.float32)
    
#     return input_tensor, output_tensor

def generate_long_ir_kernels(fretboard_obj, ir_length=1024):
    """Captures the full decay of your Butterworth filters."""
    impulse = np.zeros(ir_length, dtype=np.float32)
    impulse[0] = 1.0
    
    num_filts = fretboard_obj.get_num_filters()
    ir_buffer = np.zeros((num_filts, ir_length), dtype=np.float32)
    
    # Process the impulse through your existing FretBoard class
    fretboard_obj.process(impulse, ir_buffer)
    
    # Apply a Hanning taper to the last 15% to prevent truncation clicks
    taper_len = int(ir_length * 0.15)
    taper = np.ones(ir_length)
    taper[-taper_len:] = np.hanning(taper_len * 2)[taper_len:]
    ir_buffer = ir_buffer * taper

    # Conv1D Shape: [kernel_width, in_channels, out_channels]
    ir_kernels = ir_buffer.T[:, np.newaxis, :] 
    return tf.constant(ir_kernels, dtype=tf.float32)

# Global Filterbank Kernel

fretboard = FretBoard(17.5, SAMPLERATE)
IR_KERNELS = generate_long_ir_kernels(fretboard, 1024)

def fast_gpu_map(ipath, training=True):
    parsed = tf.io.parse_single_example(ipath, feature_description)
    audio = tf.io.decode_raw(parsed["input"], tf.float32)
    label = tf.io.decode_raw(parsed["output"], tf.int8)
    
    if training:
        # Standard volume/noise augmentation
        gain = tf.random.uniform([], 0.8, 1.2)
        audio = audio * gain
        audio = audio + tf.random.normal(tf.shape(audio), stddev=0.0001)

    # --- 1D Convolutional Filterbank ---
    audio_in = tf.reshape(audio, [1, frame_size, 1])
    # Uses 1024-sample kernels on 256-sample audio
    filtered = tf.nn.conv1d(audio_in, IR_KERNELS, stride=1, padding='SAME')
    
    # --- Rectification & Envelope Extraction ---
    envelopes = tf.abs(tf.squeeze(filtered)) # [256, 312]
    envelopes = tf.transpose(envelopes)      # [312, 256]
    
    # --- Stability Buffer ---
    # Adding epsilon (1e-7) prevents NaN in log-based loss functions
    envelopes = envelopes + 1e-7
    
    input_tensor = tf.expand_dims(tf.cast(envelopes, tf.float32), axis=-1)
    output_tensor = tf.cast(tf.reshape(label, [OUTPUT_DIM_NOTES]), tf.float32)
    
    return input_tensor, output_tensor



def augment_audio(audio, label):
    # Randomly scale volume (0.5x to 1.2x)
    gain = tf.random.uniform([], 0.9, 1.1)
    audio = audio * gain
    
    # Add a tiny bit of white noise to mask filter "ringing"
    noise = tf.random.normal(shape=tf.shape(audio), stddev=0.0001)
    return audio + noise, label
# Common functions
def save_data_slices(output_dir,nn_slices,batch_size,filenum_offset=0):
    totalsamples=nn_slices.shape[0]
    filenum_offset=filenum_offset//frame_size
    # Create directories if they don't exist
    os.makedirs(output_dir, exist_ok=True)
    print(f'Saving {totalsamples} samples to disk with filenamuber offset {filenum_offset}...')
    for i in range(0,totalsamples,batch_size):
        current_in=None
        
        if (totalsamples-i)<batch_size:
            current_in=nn_slices[i:]
        else:
            current_in=nn_slices[i:(i+batch_size)]
            
        # Define file paths for the current slice
        input_filepath = os.path.join(output_dir, f'slice_{i+filenum_offset:05d}.npy') # 05d for zero-padding up to 99999

        # Save the slices
        np.save(input_filepath, current_in)
        # if i % 1000 == 0:
        #     print(f"Saved slice {i}/{totalsamples}")

    print(f"Serialization complete. {totalsamples} Files saved in '{output_dir}'.")

# Load a single sample from files    
def load_sample_from_files(input_path_tensor):
    input_path = input_path_tensor.numpy().decode('utf-8')
    inputname=os.path.basename(input_path)
    
    parentdir=os.path.dirname(os.path.dirname(input_path))
    # print("current dir: "+parentdir)
    output_path=os.path.join(parentdir,'output',inputname)

    # print("input: "+input_path)
    # print("output: "+output_path)
    # Load data
    image = (np.load(input_path).astype(np.float32)/127.0).reshape(INPUT_SHAPE)
    label = (np.load(output_path).astype(np.float32)/127.0).reshape(OUTPUT_DIM_NOTES)

    # Ensure shape
    image = tf.ensure_shape(image, INPUT_SHAPE)
    label = tf.ensure_shape(label, (OUTPUT_DIM_NOTES,)) 
    
    # Return features and label
    return image, label
feature_description = {
    "input":  tf.io.FixedLenFeature([], tf.string),
    "output": tf.io.FixedLenFeature([], tf.string),
}


# TensorFlow wrapper for loading sample from files
def tf_load_sample_from_files(ipath):
    parsed = tf.io.parse_single_example(ipath, feature_description)
 
    # Decode as int8 as planned
    input_raw = tf.io.decode_raw(parsed["input"], tf.int8)
    output_raw = tf.io.decode_raw(parsed["output"], tf.int8)

    # Explicitly cast to float16 to match your 5080's Mixed Precision policy
    # This is faster than implicit casting during division
    input_tensor = tf.cast(tf.reshape(input_raw, INPUT_SHAPE), tf.float32)
    output_tensor = tf.cast(tf.reshape(output_raw, [OUTPUT_DIM_NOTES]), tf.float32)

    # Use multiplication by the reciprocal (1/127 ≈ 0.007874016) 
    # Multiplications are generally faster for CPUs than divisions
    return input_tensor * 0.007874016, output_tensor


    
def plot_heatmap(plotdata,downsample_factor=1000):
    num_cols=plotdata.shape[1]
    num_rows=plotdata.shape[0]

    # --- Downsampling the data ---
    print(f"Downsampling data by a factor of {downsample_factor}...")
    # Calculate the new number of columns after downsampling
    new_num_cols = num_cols // downsample_factor

    # Ensure the original number of columns is a multiple of the downsample_factor
    # If not, you might lose some data at the end or need a more complex aggregation.
    # For simplicity, we'll slice to a multiple of downsample_factor
    effective_cols = new_num_cols * downsample_factor
    data_sliced = plotdata[:, :effective_cols]
    print(data_sliced.shape)
    # Reshape the data for averaging:
    # -1: infer dimension
    # downsample_factor: group columns into blocks
    # num_rows: keep rows as isp
    # This reshapes (19, M*N) to (19, M, N)
    reshaped_data = data_sliced.reshape(num_rows, new_num_cols, downsample_factor)

    # Average along the last axis (the downsample_factor axis)
    downsampled_data = np.max(reshaped_data, axis=2)

    print(f"Downsampled array shape: {downsampled_data.shape}")

    # --- Plotting the Heatmap ---
    print("Creating heatmap...")
    plt.figure(figsize=(20, 8)) # Adjust figure size as needed, especially width for more columns
    sns.heatmap(downsampled_data, cmap='viridis', cbar_kws={'label': 'Value'})
    plt.title(f'Heatmap of ({num_rows}, {num_cols}) Array (Downsampled by {downsample_factor})')
    plt.xlabel(f'Column Bins (Each bin represents {downsample_factor} original columns)')
    plt.ylabel('Row Index')
    plt.show()
    print("Heatmap displayed.")
    
    
def reshape_to_nn_input(indata):
    return reshape_to_nn_output(indata,collapse_time=False)
    # num_cols=indata.shape[1]
    # num_rows=indata.shape[0]
    # downsample_factor = frame_size
    # # --- Downsampling the data ---
    # print(f"reshape data by a factor of {downsample_factor}...")
    # # Calculate the new number of columns after downsampling
    # new_num_cols = num_cols // downsample_factor

    # # Ensure the original number of columns is a multiple of the downsample_factor
    # # If not, you might lose some data at the end or need a more complex aggregation.
    # # For simplicity, we'll slice to a multiple of downsample_factor
    # effective_cols = new_num_cols * downsample_factor
    # data_sliced = indata[:, :effective_cols]
    # print(data_sliced.shape)
    # # Reshape the data for averaging:
    # # -1: infer dimension
    # # downsample_factor: group columns into blocks
    # # num_rows: keep rows as isp
    # # This reshapes (19, M*N) to (19, M, N)
    # reshaped_data = np.max(data_sliced.reshape(num_rows, new_num_cols, downsample_factor),axis=2)
    # reshaped_data=np.swapaxes(reshaped_data,0,1)
    # # reshaped_data=np.swapaxes(reshaped_data,1,2)
    
    # print('Reshaped the input data to  ')
    # print(reshaped_data.shape)
    # return reshaped_data

def reshape_to_nn_output(outdata,collapse_time=True):
    num_samples=outdata.shape[1]
    num_midi_classes=outdata.shape[0]
    downsample_factor = frame_size
    # --- Downsampling the data ---
    print(f"reshape data by a factor of {downsample_factor}...")
    # Calculate the new number of columns after downsampling
    num_frames = num_samples // downsample_factor

    # Ensure the original number of columns is a multiple of the downsample_factor
    # If not, you might lose some data at the end or need a more complex aggregation.
    # For simplicity, we'll slice to a multiple of downsample_factor
    effective_cols = num_frames * downsample_factor
    data_sliced = outdata[:, :effective_cols]
    print(data_sliced.shape)
    # Reshape the data for averaging:
    # -1: infer dimension
    # downsample_factor: group columns into blocks
    # num_rows: keep rows as isp
    # This reshapes (19, M*N) to (19, M, N)
    reshaped_data = data_sliced.reshape(num_midi_classes, num_frames, downsample_factor)
    
    #Take only one sample per frame
    if collapse_time:
        reshaped_data=np.max(reshaped_data,axis=2)
    reshaped_data=np.swapaxes(reshaped_data,0,1)
    
    print('Reshaped the output data to  ')
    print(reshaped_data.shape)
    return reshaped_data