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
#include <cassert>
#include <math.h>
#include <stdlib.h>


#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include <noteclassifier.hpp>


#define AMP_URI "http://github.com/geraldmwangi/GuitarMidi-LV2"







static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{

   NoteClassifier* notecl=new NoteClassifier(rate);
	return (LV2_Handle)notecl;
}


static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	NoteClassifier* notecl = (NoteClassifier*)instance;

	switch ((PortIndex)port) {
	case NOTECL_INPUT:
		notecl->input = (const float*)data;
		break;
	case NOTECL_OUTPUT:
		notecl->output = (float*)data;
		break;
	}
}


static void
activate(LV2_Handle instance)
{
   NoteClassifier* notecl = (NoteClassifier*)instance;
   notecl->initialize();
}

/** Define a macro for converting a gain in dB to a coefficient. */
#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)


static void
run(LV2_Handle instance, uint32_t n_samples)
{
	 NoteClassifier* notecl = ( NoteClassifier*)instance;
   notecl->process(n_samples);

	
}


static void
deactivate(LV2_Handle instance)
{
   NoteClassifier* notecl = (NoteClassifier*)instance;
   notecl->finalize();
}


static void
cleanup(LV2_Handle instance)
{
	NoteClassifier* notecl = ( NoteClassifier*)instance;
   delete notecl;
}


static const void*
extension_data(const char* uri)
{
	return NULL;
}


static const LV2_Descriptor descriptor = {
	AMP_URI,
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};


LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:  return &descriptor;
	default: return NULL;
	}
}
