# GuitarMidi-LV2
A concept for guitar to midi as an lv2 plugin. GuitarMidi-LV2 analyses the signal of a guitar in standard tuning E A D g b e extracts the notes played.
It deploys a bank of elliptic cauer bandpass filters to separate the polyphonic audio into monophonic frequency segments, which are then analysed for multiple fundamental frequencies.
### Why not use FFT?
For higher frequencies (strings g b e) FFT would be suffient at frame buffersizes of 512 samples. However for lower frequencies the FFT would need windows of 1024 samples and higher to provide suitable resolution for the notes on the E string (82Hz and higher). This results in high latency for the guitar player.
IIR based filters such as those used in this plugin offer faster frequency response at suitable resolution at low frequencies.

### So how fast is GuitarMidi-LV2
Don't expect wonders. While I havent done strict meassurements I have percieved drastically lower latency compared to my earlier FFT based method which had a guaranteed latency of 1024 samples at 48khz. This new approach is more like 500-700 samples at 48khz when playing lower frequencies at 82hz (E string). The figures are better at higher frequencies, but again: You won't be able to play funk ala Superstition from Stevie Wonder (well not at this stage).



# Installation
Packages for Debian/Ubuntu are available at https://github.com/geraldmwangi/GuitarMidi-LV2/releases

# Usage
Currently your Host must be running with 256 samples per period at 48KHz (that will change soon). 
Your guitarsignal should be clean, no distortion of sort. 
* Tune the guitar to the standard E A D g b e tuning
* Activate the bridge pickup
* Set the input gain on the audio interface to 3/4
* Connect the audio in of the guitarmidi plugin to the channel on your interface, to which you guitar is connected. 
* connect the midi out of guitarmidi to a synth plugin 
* first play single notes, adjust the input gain to improve tracking
* successivly play more strings

## Tips
GuitarMidi doesnt handle the attack phase of the guitar, it blows out a bunch of notes in that phase.
To overcome this increase the attack time in the synth.
Second, while the tracking of the bass strings E and A does work, it absolutely is not perfect.
Playing full chords on all 6 strings doesnt work well due to the tracking on E and A string. So try a rather 'John Frusciante' style, playing on the upper strings D,g,b,e. That works somewhat well for me.


### Current Bugs and missing things
* Velocity is not extracted from the audio. All midi notes have max velocity (=127)
* there is no midi panic control
* ~~latency is higher then theoretically possible. Due to an issue in the DSPFilter library I can only setup 1st order filters, although 2nd order filters
    should be possible~~ Higher order filters are working, latency is reduced but still perceivable
* Since a guitar string vibrates with many partial frequencies, it triggers 2-3 harmonic notes (playing A triggers the higher a too )
* thats why analysis is done only upto the 5th fret
* I want to look into neural nets to solve the problem of the harmonic notes, help is welcome

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



