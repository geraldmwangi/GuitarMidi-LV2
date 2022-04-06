/*
  Copyright 2006-2016 David Robillard <d@drobilla.net>
  Copyright 2006 Steve Harris <steve@plugin.org.uk>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/** Include standard C headers */
#include <math.h>
#include <stdlib.h>

/**
   LV2 headers are based on the URI of the specification they come from, so a
   consistent convention can be used even for unofficial extensions.  The URI
   of the core LV2 specification is <http://lv2plug.in/ns/lv2core>, by
   replacing `http:/` with `lv2` any header in the specification bundle can be
   included, in this case `lv2.h`.
*/
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"

/**
   The URI is the identifier for a plugin, and how the host associates this
   implementation in code with its description in data.  In this plugin it is
   only used once in the code, but defining the plugin URI at the top of the
   file is a good convention to follow.  If this URI does not match that used
   in the data files, the host will fail to load the plugin.
*/
#define TESTMIDI_URI "http://github.com/geraldmwangi/TestMidi"

/**
   In code, ports are referred to by index.  An enumeration of port indices
   should be defined for readability.
*/
typedef enum
{
    TESTMIDI_OUT = 0
} PortIndex;

typedef struct
{
    LV2_URID atom_Path;
    LV2_URID atom_Resource;
    LV2_URID atom_Sequence;
    LV2_URID atom_URID;
    LV2_URID atom_eventTransfer;
    LV2_URID midi_Event;
    LV2_URID patch_Set;
    LV2_URID patch_property;
    LV2_URID patch_value;
} TestMidiURIs;
/**
   Every plugin defines a private structure for the plugin instance.  All data
   associated with a plugin instance is stored here, and is available to
   every instance method.  In this simple plugin, only port buffers need to be
   stored, since there is no additional instance data.
*/

typedef struct
{
    float beat_time;
    uint8_t size;
    uint8_t event[3];
} MIDISequence;

typedef struct
{
    // Port buffers
    LV2_Atom_Sequence *output;
    TestMidiURIs uris;
    double srate;
    int current_sample;
    int noteon_time_s;
    double note_freq;
    bool note_is_on;
} TestMidi;

/**
   The `instantiate()` function is called by the host to create a new plugin
   instance.  The host passes the plugin descriptor, sample rate, and bundle
   path for plugins that need to load additional resources (e.g. waveforms).
   The features parameter contains host-provided features defined in LV2
   extensions, but this simple plugin does not use any.

   This function is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static LV2_Handle
instantiate(const LV2_Descriptor *descriptor,
            double rate,
            const char *bundle_path,
            const LV2_Feature *const *features)
{
    TestMidi *testmidi = (TestMidi *)calloc(1, sizeof(TestMidi));
    testmidi->srate = rate;
    testmidi->current_sample = 0;
    testmidi->noteon_time_s = 1;
    testmidi->note_freq = 110;
    testmidi->note_is_on = false;

    return (LV2_Handle)testmidi;
}

/**
   The `connect_port()` method is called by the host to connect a particular
   port to a buffer.  The plugin must store the data location, but data may not
   be accessed except in run().

   This method is in the ``audio'' threading class, and is called in the same
   context as run().
*/
static void
connect_port(LV2_Handle instance,
             uint32_t port,
             void *data)
{
    TestMidi *testmidi = (TestMidi *)instance;

    switch ((PortIndex)port)
    {
    case TESTMIDI_OUT:
        testmidi->output = (LV2_Atom_Sequence *)data;
        break;
    }
}

/**
   The `activate()` method is called by the host to initialise and prepare the
   plugin instance for running.  The plugin must reset all internal state
   except for buffer locations set by `connect_port()`.  Since this plugin has
   no other internal state, this method does nothing.

   This method is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static void
activate(LV2_Handle instance)
{
}

/**
   The `run()` method is the main process function of the plugin.  It processes
   a block of audio in the audio context.  Since this plugin is
   `lv2:hardRTCapable`, `run()` must be real-time safe, so blocking (e.g. with
   a mutex) or memory allocation are not allowed.
*/
static void
run(LV2_Handle instance, uint32_t n_samples)
{
    TestMidi *testmidi = (TestMidi *)instance;
    // Get the capacity
    const uint32_t out_capacity = testmidi->output->atom.size;
    // Write an empty Sequence header to the output
    lv2_atom_sequence_clear(testmidi->output);
    testmidi->output->atom.type;

    // Struct for a 3 byte MIDI event, used for writing notes
    typedef struct
    {
        LV2_Atom_Event event;
        uint8_t msg[3];
    } MIDINoteEvent;

    int notesamples = testmidi->noteon_time_s * testmidi->srate;
    for (unsigned int i = 0; i < n_samples; i++)
    {
        (testmidi->current_sample)++;
        if (testmidi->current_sample > notesamples)
        {
            testmidi->current_sample = 0;
            if (testmidi->note_is_on)
            {
                // Send note off
                testmidi->note_is_on = false;
                printf("Note off\n");
                // MIDISequence noteoff={ 0.50, 3, {0x80,  60, 0x00} };
                // IRC:
                /**
                 *  JimsonDrift: did you see the atom util header? it makes it so you dont need to understand the full logic, just use the utlities it provides
[16:47] <JimsonDrift> falktx: No I didnt. I'll look at that
[16:47] <falktx> https://github.com/lv2/lv2/blob/master/lv2/atom/util.h
                 *
                 */
                MIDINoteEvent noteoff;
                noteoff.event.time;
                noteoff.event.body.size = 3;
                noteoff.event.body.type = testmidi->uris.midi_Event;
                // noteoff.msg={0x80,  60, 0x00};
                noteoff.msg[0] = 0x80;
                noteoff.msg[1] = 60;
                noteoff.msg[0] = 0x00;
                if (!lv2_atom_sequence_append_event(testmidi->output, out_capacity, &noteoff.event))
                    printf("Failed to write noteoff");
            }
            else
            {
                // Send note on
                testmidi->note_is_on = true;
                printf("Note on\n");
                // MIDISequence noteon={ 0.00, 3, {0x90,  60, 0x7f} };
                MIDINoteEvent noteon;
                noteon.event.body.size = 3;
                noteon.event.body.type = testmidi->uris.midi_Event;
                // noteoff.msg={0x80,  60, 0x00};
                noteon.msg[0] = 0x80;
                noteon.msg[1] = 60;
                noteon.msg[0] = 0x7f;
                if (!lv2_atom_sequence_append_event(testmidi->output, out_capacity, &noteon.event))
                    printf("Failed to write noteon");
            }
        }
    }
}

/**
   The `deactivate()` method is the counterpart to `activate()`, and is called by
   the host after running the plugin.  It indicates that the host will not call
   `run()` again until another call to `activate()` and is mainly useful for more
   advanced plugins with ``live'' characteristics such as those with auxiliary
   processing threads.  As with `activate()`, this plugin has no use for this
   information so this method does nothing.

   This method is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static void
deactivate(LV2_Handle instance)
{
}

/**
   Destroy a plugin instance (counterpart to `instantiate()`).

   This method is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static void
cleanup(LV2_Handle instance)
{
    free(instance);
}

/**
   The `extension_data()` function returns any extension data supported by the
   plugin.  Note that this is not an instance method, but a function on the
   plugin descriptor.  It is usually used by plugins to implement additional
   interfaces.  This plugin does not have any extension data, so this function
   returns NULL.

   This method is in the ``discovery'' threading class, so no other functions
   or methods in this plugin library will be called concurrently with it.
*/
static const void *
extension_data(const char *uri)
{
    return NULL;
}

/**
   Every plugin must define an `LV2_Descriptor`.  It is best to define
   descriptors statically to avoid leaking memory and non-portable shared
   library constructors and destructors to clean up properly.
*/
static const LV2_Descriptor descriptor = {
    TESTMIDI_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data};

/**
   The `lv2_descriptor()` function is the entry point to the plugin library.  The
   host will load the library and call this function repeatedly with increasing
   indices to find all the plugins defined in the library.  The index is not an
   indentifier, the URI of the returned descriptor is used to determine the
   identify of the plugin.

   This method is in the ``discovery'' threading class, so no other functions
   or methods in this plugin library will be called concurrently with it.
*/
LV2_SYMBOL_EXPORT
const LV2_Descriptor *
lv2_descriptor(uint32_t index)
{
    switch (index)
    {
    case 0:
        return &descriptor;
    default:
        return NULL;
    }
}
