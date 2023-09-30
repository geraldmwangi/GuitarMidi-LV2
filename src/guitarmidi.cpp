/* GuitarMidi-LV2 Library
 * Copyright (C) 2022 Gerald Mwangi
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/** Include standard C headers */
#include <cassert>
#include <math.h>
#include <stdlib.h>
#include <lv2/core/lv2.h>
#include <fretboard.hpp>

#define AMP_URI "http://github.com/geraldmwangi/GuitarMidi-LV2"

static LV2_Handle
instantiate(const LV2_Descriptor *descriptor,
			double rate,
			const char *bundle_path,
			const LV2_Feature *const *features)
{
	LV2_URID_Map *map = NULL;

	int i;
	const char *missing = lv2_features_query(features,
											 LV2_LOG__log, &g_logger, false,
											 LV2_URID__map, &map, true);
	lv2_log_logger_set_map(&g_logger, map);
	if (missing)
	{
		lv2_log_error(&g_logger, "Missing feature <%s>\n", missing);
		return 0;
	}
	FretBoard *fretboard = new FretBoard(map, rate);
	return (LV2_Handle)fretboard;
}

static void
connect_port(LV2_Handle instance,
			 uint32_t port,
			 void *data)
{
	FretBoard *fretboard = (FretBoard *)instance;

	switch ((PortIndex)port)
	{
	case FRETBOARD_INPUT:
		// notecl->input = (const float *)data;
		fretboard->setAudioInput((const float *)data);
		break;
	case FRETBOARD_OUTPUT:
		fretboard->setAudioOutput((float *)data);
		break;
	case FRETBOARD_MIDIOUTPUT:
		fretboard->setMidiOutput((LV2_Atom_Sequence *)data);
		break;
	}
}

static void
activate(LV2_Handle instance)
{
	FretBoard *notecl = (FretBoard *)instance;
	notecl->initialize();
}

/** Define a macro for converting a gain in dB to a coefficient. */
#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)

static void
run(LV2_Handle instance, uint32_t n_samples)
{

	timespec start = timer_start();
	FretBoard *notecl = (FretBoard *)instance;
	notecl->process(n_samples);
	auto delay = timer_end(start);
#ifdef WITH_TRACING_INFO
	lv2_log_trace(&g_logger, "processing in %ld\n", delay);
#endif
}

static void
deactivate(LV2_Handle instance)
{
	FretBoard *notecl = (FretBoard *)instance;
	notecl->finalize();
}

static void
cleanup(LV2_Handle instance)
{
	FretBoard *notecl = (FretBoard *)instance;
	delete notecl;
}

static const void *
extension_data(const char *uri)
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
	extension_data};

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
