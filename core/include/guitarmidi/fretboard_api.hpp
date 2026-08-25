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
#pragma once

#include <concepts>
#include <string>

#include <lv2/atom/atom.h>


template <typename T>
concept FretBoardAPI = requires(
	T& fretboard,
	const float* audio_input,
	LV2_Atom_Sequence* midi_output,
	const std::string& bundle_path,
	float* control,
	int samplerate,
	int buffer_size,
	int nsamples)
{
	{ fretboard.setAudioInput(audio_input) } -> std::same_as<void>;
	{ fretboard.setMidiOutput(midi_output) } -> std::same_as<void>;

	{ fretboard.setSmoothing(control) } -> std::same_as<void>;
	{ fretboard.setOnsetThreshold(control) } -> std::same_as<void>;
	{ fretboard.setOffsetThreshold(control) } -> std::same_as<void>;
	{ fretboard.setSmoothingOffset(control) } -> std::same_as<void>;
	{ fretboard.setOnsetEnergyThreshold(control) } -> std::same_as<void>;
	{ fretboard.setOffsetEnergyThreshold(control) } -> std::same_as<void>;
	{ fretboard.setGain(control) } -> std::same_as<void>;
	{ fretboard.setExpressivity(control) } -> std::same_as<void>;

	{ fretboard.initialize(bundle_path, samplerate, buffer_size) } -> std::same_as<bool>;
	{ fretboard.finalize() } -> std::same_as<void>;
	{ fretboard.process(nsamples) } -> std::same_as<void>;

};

