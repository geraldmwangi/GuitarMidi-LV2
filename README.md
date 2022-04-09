# GuitarMidi-LV2
A concept for guitar to midi as an lv2 plugin. GuitarMidi-LV2 analyses the signal of a guitar in standard tuning E A D g b e extracts the notes played.
It deploys a bank of elliptic cauer bandpass filters to separate the polyphonic audio into monophonic frequency segments, which are then analysed by monophonic pitch detectors.
### Why not use FFT?
For higher frequencies (strings g b e) FFT would be suffient at frame buffersizes of 512 samples. However for lower frequencies the FFT would need windows of 1024 samples and higher to provides suitable resolution for the notes on the E string (82Hz and higher). This results in high latency for the guitar play.
IIR based filters such as those used in this plugin offer faster frequency response at suitable resolution at low frequencies.

### So how fast is GuitatMidi-LV2
Don't expect wonders. While I havent done strict meassurements I have percieved drastically lower latency compared to my earlier FFT based method which had a guaranteed latency of 1024 samples at 48khz. This new approach is more like 500-700 samples at 48khz when playing lower frequencies at 82hz (E string). The figures are better at higher frequencies, but again: You won't be able to play funk ala Superstition from Stevie Wonder (well not at this stage).

## Build
You will need cmake, lv2-dev, libaubio-dev.
* clone this repo to /path/to/GuitarMidi-LV2
* run cmake, make, make install:
```bash
cd GuitarMidi-LV2
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=~/.lv2 ..
make
make install
```
* in ~/.lv2 you should see guitarmidi.lv2

Point your lv2 host to ~/.lv2 (Ardour and Carla will autmatically search in this directory on debian based systems)

## Getting it to work
Load up Carla  and refresh the plugins (Add plugin->refresh) then search for Guitar Midi (again under Add plugin).
Load a synth and connect the midiout of Guitar Midi to the midi in of the synth. Connect the port of the guitar signal to the audio in in Guitar Midi and 
the audio out of the synth to the audio outs in your audio interface.
Set the main volume of the audio interface rather low for testing.
For Guitar Midi to work somewhat well you must set your input gain on the sound device to the least value so that the e string struck alone is properly detected.
Have fun hopefully.
