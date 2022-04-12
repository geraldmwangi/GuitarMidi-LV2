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
#include <fretboard.hpp>

FretBoard::FretBoard(LV2_URID_Map *map, float samplerate)
{
    m_midioutput=make_shared<MidiOutput>(map);
    // E string
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 82.41,10));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 87.31,10));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 92.50,10));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 98.00,10));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 103.83,10));

    // // A string
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 110,20));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 116.54,20));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 123.47,20));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 130.81,20));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 138.59,20));

    // // D string
     m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 146.83,30));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 155.56,30));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 164.81,30));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 174.61,30));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 185.00,30));

    // // g string
     m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 196.00,40));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 207.65,40));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 220,40));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 233.08,40));

    // // b string
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 246.94,50));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 261.63,50));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 277.18,50));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 293.66,50));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 311.13,50));

    // // e string
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 329.63));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 349.23));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 369.99));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 392));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 415.30));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 440));
}

void FretBoard::setAudioInput(const float *input)
{
    for (auto notecl : m_noteClassifiers)
    {
        notecl->input = input;
    }
}

void FretBoard::setAudioOutput(float *output)
{
    for (auto notecl : m_noteClassifiers)
    {
        notecl->output = output;
    }
}

void FretBoard::setMidiOutput(LV2_Atom_Sequence *output)
{
    if(m_midioutput)
    {
        m_midioutput->setMidiOutput(output);
        for (auto notecl : m_noteClassifiers)
        {
            notecl->setMidiOutput(m_midioutput);
        }
    }
}

void FretBoard::initialize()
{
    m_midioutput->initializeSequence();
    for (auto notecl : m_noteClassifiers)
        notecl->initialize();
}

void FretBoard::finalize()
{
    for (auto notecl : m_noteClassifiers)
        notecl->finalize();
}

void FretBoard::process(int nsamples)
{
    m_midioutput->initializeSequence();
    for (auto notecl : m_noteClassifiers)
        notecl->process(nsamples);
}