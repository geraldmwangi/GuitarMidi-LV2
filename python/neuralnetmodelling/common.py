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

def extract_iir_coefficients(fretboard_obj):
    b_list = []
    a_list = []
    for fret in fretboard_obj.frets:
        for string in fret.strings:
            for filt in string.harmonics:
                # Store as [b0, b1, b2] and [1.0, a1, a2]
                b_list.append(filt.b)
                a_list.append(filt.a)
    
    return (tf.constant(b_list, dtype=tf.float32), 
            tf.constant(a_list, dtype=tf.float32))

B_COEFFS, A_COEFFS = extract_iir_coefficients(fretboard)
# Shapes: [312, 3]
def apply_iir_filterbank(audio, b, a):
    x = tf.expand_dims(audio, axis=-1) 
    initial_state = tf.zeros((tf.shape(b)[0], 2), dtype=tf.float32)

    # Note the change here: 'state' is now unpacked into (s_internal, y_prev)
    def iir_step(state, x_n):
        s_internal, _ = state  # s_internal is [312, 2]
        
        # y[n] = b0*x[n] + s1
        y_n = b[:, 0] * x_n + s_internal[:, 0]
        
        # Update internal states
        new_s1 = b[:, 1] * x_n - a[:, 1] * y_n + s_internal[:, 1]
        new_s2 = b[:, 2] * x_n - a[:, 2] * y_n
        
        # Return a tuple matching the initializer structure
        return (tf.stack([new_s1, new_s2], axis=-1), y_n)

    # Scan results in a tuple of (states_history, outputs_history)
    _, outputs = tf.scan(iir_step, x, initializer=(initial_state, tf.zeros(312)))
    
    return tf.transpose(outputs) # [312, 256]

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
def fast_gpu_map(ipath,training=True):
    parsed = tf.io.parse_single_example(ipath, feature_description)
    audio = tf.io.decode_raw(parsed["input"], tf.float32)
    label = tf.io.decode_raw(parsed["output"], tf.int8)
    
    if training:
        audio, label = augment_audio(audio, label)
    
    # --- Vectorized Filtering ---
    audio_fft = tf.signal.rfft(audio) 
    filtered_fft = FILTER_MASK * tf.cast(audio_fft, tf.complex64)
    envelopes = tf.abs(tf.signal.irfft(filtered_fft)) # Shape: [312, 256]
    
    # --- Vectorized Normalization ---
    # We find the peak activation across all filters for this specific window
    # max_val = tf.reduce_max(envelopes)
    
    # If the peak is > 0.1, we scale the whole tensor down so the peak is 1.0
    # Using tf.where prevents division by zero and applies the condition element-wise
    # envelopes = tf.cond(
    #     max_val > 0.1,
    #     lambda: envelopes / max_val, 
    #     lambda: envelopes
    # )
    
    # Reshape for CNN
    input_tensor = tf.cast(envelopes, tf.float32)
    input_tensor = tf.expand_dims(input_tensor, axis=-1)
    output_tensor = tf.cast(tf.reshape(label, [OUTPUT_DIM_NOTES]), tf.float32)
    
    return input_tensor, output_tensor


# def fast_gpu_map(ipath, training=True):
#     parsed = tf.io.parse_single_example(ipath, feature_description)
#     audio = tf.io.decode_raw(parsed["input"], tf.float32)
#     label = tf.io.decode_raw(parsed["output"], tf.int8)
    
#     if training:
#         audio, label = augment_audio(audio, label)
    
#     # --- Time-Domain Forward-Backward Filtering ---
#     # 1. Forward pass
#     fwd = apply_iir_filterbank(audio, B_COEFFS, A_COEFFS)
    
#     # 2. Backward pass (simulates zero-phase / squared magnitude)
#     rev_input = tf.reverse(fwd, axis=[-1])
#     # Note: We must process each filter's reverse output individually
#     # but apply_iir_filterbank already handles 312 channels.
#     # We map over the 312 channels to re-filter
#     bwd = apply_iir_filterbank_reversed(rev_input, B_COEFFS, A_COEFFS)
#     envelopes = tf.abs(tf.reverse(bwd, axis=[-1]))

#     # --- Reshape and Normalization ---
#     # (Optional: insert your peak-normalization logic here)
    
#     input_tensor = tf.expand_dims(tf.cast(envelopes, tf.float32), axis=-1)
#     output_tensor = tf.cast(tf.reshape(label, [OUTPUT_DIM_NOTES]), tf.float32)
    
#     return input_tensor, output_tensor

def apply_iir_filterbank_reversed(audio_bank, b, a):
    x_bank = tf.transpose(audio_bank) # [256, 312]
    initial_state = tf.zeros((tf.shape(b)[0], 2), dtype=tf.float32)

    def iir_step_bank(state, x_n_bank):
        s_internal, _ = state 
        
        y_n = b[:, 0] * x_n_bank + s_internal[:, 0]
        
        new_s1 = b[:, 1] * x_n_bank - a[:, 1] * y_n + s_internal[:, 1]
        new_s2 = b[:, 2] * x_n_bank - a[:, 2] * y_n
        
        return (tf.stack([new_s1, new_s2], axis=-1), y_n)

    _, outputs = tf.scan(iir_step_bank, x_bank, initializer=(initial_state, tf.zeros(312)))
    return tf.transpose(outputs)


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