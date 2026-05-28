# GuitarMidi-LV2
A concept for guitar to midi as an lv2 plugin. GuitarMidi-LV2 analyses the signal of a guitar in standard tuning E A D g b e extracts the notes played.
It deploys a bank of butterworth bandpass filters to separate the polyphonic audio into monophonic frequency segments, which are then analysed for multiple fundamental frequencies by a custom neural net.




# Installation
Packages for Debian/Ubuntu are available at https://github.com/geraldmwangi/GuitarMidi-LV2/releases

# Usage
Currently your Host must be running with 256 samples per period at 48KHz (that will change soon). 
Your guitarsignal should be clean, no distortion of sort. 
* Tune the guitar to the standard E A D g b e tuning
* Activate the bridge pickup
* Set the input gain on the audio interface to 50%
* Connect the audio in of the guitarmidi plugin to the channel on your interface, to which you guitar is connected. 
* connect the midi out of guitarmidi to a synth plugin 
* first play single notes, adjust the input gain to improve tracking
* successively play more strings/chords

# Plugin Parameters
The plugin exposes several LV2 control ports to tune detection behavior:

* **Onset Smoothing** — [0.0, 0.9], default `0.5`. Smooths the detected note onset signal. Higher values reduce false triggers but may make note starts feel less responsive.
* **Offset Smoothing** — [0.0, 0.9], default `0.5`. Smooths note release detection. Higher values keep notes held longer and reduce chatter on note endings.
* **Onset Confidence Threshold** — [0.00, 0.99], default `0.80`. Controls how confident the detector must be before sending a note-on event. Increase to reduce spurious onsets.
* **Offset Confidence Threshold** — [0.00, 0.99], default `0.10`. Controls how confident the detector must be before sending a note-off event. Increase to make notes release sooner.
* **Onset Energy Threshold (dB)** — [-20.0, 3.0], default `0.80`. Minimum volume required for an onset to be accepted. Increase to reduce spurious onsets of lower notes.  
* **Offset Energy Threshold (dB)** — [-20.0, 3.0], default `-15.0`. Minimum volume required to keep a note alive before releasing it.

These controls let you balance sensitivity and stability for your guitar signal and playing style.

# Model description (written with AI)
The neural network model in this project is designed to analyze guitar audio and predict which notes are being played, converting the sound into MIDI data for music production. Here's a simple explanation for non-experts:

## What It Does
Imagine you're listening to a guitar. The model acts like an extremely attentive listener that can "hear" individual notes even when multiple are played together (chords) or when the sound is noisy. It takes raw audio and outputs a list of probabilities for each possible guitar note, telling you how likely each note is to be sounding at that moment.

## How It Works 
The process is like breaking down the guitar sound into manageable pieces and then reassembling them intelligently:

1. **Pre-Processing the Audio**: Before the neural network sees the audio, it's run through a filterbank. This filterbank uses digital filters tuned to the frequencies of guitar notes and their harmonics (overtones). For each of the 13 frets on 6 strings, plus 4 harmonics per note, it creates 148 filtered signals. This turns the raw audio into a kind of "spectrogram" – a visual-like representation of the sound's frequency content over time.

2. **Input to the Model**: The neural network receives this 148x256 "image" (148 frequency bands by 256 time samples) as input.

3. **Feature Extraction**:
   - **Local Contrast Normalization**: It adjusts the input to highlight differences, like making quiet parts stand out against loud ones. Suggested by Gemini
   - **Time Compression**: It compresses the time dimension using convolutional layers to focus on patterns.
   - **Transformer Block**: This is like a smart attention mechanism that looks at relationships between different frequency bands, helping the model understand how notes interact.

4. **String-Aware Processing**: The model splits the data into 6 parallel paths, one for each guitar string. Each path processes its own set of notes:
   - Convolutional layers extract features for each string.
   - "Hollow Neighborhood Suppression" reduces interference from nearby frets (like preventing a note from being confused with its neighbors).
   - It predicts probabilities for each of the 13 frets on that string.

5. **Chord Reasoning**: A 2D convolutional layer looks across all strings simultaneously to understand chord patterns and how notes combine.

6. **Output**: The model combines the per-string predictions into a final output of 37 possible notes (covering the guitar's range from low E to high E). It uses a special "sparse guitar output" layer that maps string/fret combinations to MIDI notes, ensuring only valid guitar notes are predicted. The output is probabilities (0-1) for each note, using multiple softmax activations for multi-label detection (multiple notes can be active at once).

## Key Technical Details 
- **Architecture**: Convolutional Neural Network (CNN) with transformer elements, specialized for guitar.
- **Training**: Uses TensorFlow/Keras, trained on labeled audio data with binary cross-entropy loss (good for multi-label problems).
- **Output Format**: 37-note vector with mixture of softmax probabilities, not a single "best guess."
- **Real-Time Friendly**: Designed to be efficient for audio processing, with techniques like mixed-precision training for GPUs.

In essence, it's a sophisticated pattern-matching system that learns to recognize guitar notes from filtered audio data, much like how your brain recognizes familiar sounds but with math and computers. The model is trained on examples of guitar playing to get better at this task.

### Current Bugs and missing things
* Velocity is not extracted from the audio. All midi notes have max velocity (=127)
* Only notes upto e5 (12th fret on the e-string) are detected. This is due to the current nvme shortage, I only have enough nvme to store 19000 songs with 37 notes
* The plugin detects mostly major and minor chords since those are dominant in the training data
* there is no midi panic control
* ~~latency is higher then theoretically possible. Due to an issue in the DSPFilter library I can only setup 1st order filters, although 2nd order filters
    should be possible~~ Higher order filters are working, latency is reduced but still perceivable
* Since a guitar string vibrates with many partial frequencies, it may trigger 2-3 harmonic notes 



# Donations
You can support the development by donating via [Github Sponsors](https://github.com/sponsors/geraldmwangi)


or my paypal

[![](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/donate/?hosted_button_id=7ELJYWWJ2BKRQ)



# Build
If your distro is not supported by any package at  https://github.com/geraldmwangi/GuitarMidi-LV2/releases:
You will need cmake, lv2-dev.
* clone this repo to /path/to/GuitarMidi-LV2
* run cmake, make, make install:
```bash
cd GuitarMidi-LV2
git submodule update --init
mkdir build
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=~/.lv2 ..
cmake --build .
cmake --install .
```
* in ~/.lv2 you should see guitarmidi.lv2

Point your lv2 host to ~/.lv2 (Ardour and Carla will autmatically search in this directory on debian based systems)

# Acknowledgments
I'd like to thank the team at https://synthtab.dev/ for the great dataset they put out.
Also I should mention Gemini and Claude for helping me learn tensorflow. The ideas of the model are mine truely, except where noted otherwise.
The two AI services helped with the details of the model and its training until I had enough knowledge of tensorflow. Thanks


