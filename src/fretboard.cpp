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
using namespace GuitarMidi;
FretBoard::FretBoard(LV2_URID_Map *map, float samplerate)
{
    m_midioutput=make_shared<MidiOutput>(map);
    // // E string
    // float Ebw=10;
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 82.41,Ebw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 87.31,Ebw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 92.50,Ebw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 98.00,Ebw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 103.83,Ebw));

    // // // A string
    // float Abw=10;
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 110,Abw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 116.54,Abw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 123.47,Abw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 130.81,Abw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 138.59,Abw));

    // // // D string
    // float Dbw=10;
    //  m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 146.83,Dbw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 155.56,Dbw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 164.81,Dbw));



    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 174.61,Dbw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 185.00,Dbw));

    // // // g string
    // float gbw=10;
    //  m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 196.00,gbw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 207.65,gbw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 220,gbw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 233.08,gbw));

    // // // b string
    // float bbw=10;
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 246.94,bbw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 261.63,bbw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 277.18,bbw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 293.66,bbw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 311.13,bbw));

    // // // e string
    // float ebw=10;
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 329.63,ebw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 349.23,ebw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 369.99,ebw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 392,ebw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 415.30,ebw));
    // m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 440,ebw));

    for (int n = 1; n <=4; n++)
    {
        addNoteClassifier(82.41,n, map, samplerate);  // E
        addNoteClassifier(87.31,n, map, samplerate);  // F
        addNoteClassifier(92.50,n, map, samplerate);  // F#
        addNoteClassifier(98.00,n, map, samplerate);  // G
        addNoteClassifier(103.83,n, map, samplerate); // G#
        addNoteClassifier(110,n, map, samplerate);    // A
        addNoteClassifier(116.54,n, map, samplerate); // A#
        addNoteClassifier(123.47,n, map, samplerate); // B
        addNoteClassifier(130.81,n, map, samplerate); // C
        addNoteClassifier(138.59,n, map, samplerate); // C#
        addNoteClassifier(146.83,n, map, samplerate); // D
        addNoteClassifier(155.56,n, map, samplerate); // D#
        addNoteClassifier(164.81,n, map, samplerate);
        addNoteClassifier(174.61,n, map, samplerate);
        addNoteClassifier(185,n, map, samplerate);
        addNoteClassifier(196,n, map, samplerate);
        addNoteClassifier(207.65,n, map, samplerate);
        addNoteClassifier(220,n, map, samplerate);
        addNoteClassifier(233.08,n, map, samplerate);
        addNoteClassifier(246.94,n, map, samplerate);
        addNoteClassifier(261.63,n, map, samplerate);
        addNoteClassifier(277.18,n, map, samplerate);
        addNoteClassifier(293.66,n, map, samplerate);
        addNoteClassifier(329.63,n, map, samplerate);

        addNoteClassifier(349.23,n, map, samplerate);
        addNoteClassifier(369.99,n, map, samplerate);
        addNoteClassifier(392.00,n, map, samplerate);
        addNoteClassifier(415.30,n, map, samplerate);
        addNoteClassifier(440.00,n, map, samplerate);
        addNoteClassifier(466.16,n, map, samplerate);
        addNoteClassifier(493.88,n, map, samplerate);
        addNoteClassifier(523.25,n, map, samplerate);
        addNoteClassifier(554.37,n, map, samplerate);
        addNoteClassifier(587.33,n, map, samplerate);
        addNoteClassifier(622.25,n, map, samplerate);
        addNoteClassifier(659.26,n, map, samplerate);
        addNoteClassifier(698.46,n, map, samplerate);
        addNoteClassifier(739.99,n, map, samplerate);
        addNoteClassifier(783.99,n, map, samplerate);
        addNoteClassifier(830.61,n, map, samplerate);
        addNoteClassifier(880.00,n, map, samplerate);
        addNoteClassifier(932.33,n, map, samplerate);
        addNoteClassifier(987.77,n, map, samplerate);
        
    }

    for(auto group:m_harmonicGroups)
    {
        for (auto notecl : m_noteClassifiersMap)
        {
            group.second->addNoteClassifier(notecl.second);
        }

    }
}
void FretBoard::addNoteClassifier(float freq,int mult,LV2_URID_Map *map, float samplerate)
{
    freq*=mult;
    if(mult==1||freq>987.77)
    {
        m_noteClassifiersMap[freq] = make_shared<NoteClassifier>(map, samplerate, freq, 10);
        m_harmonicGroups[freq] = make_shared<HarmonicGroup>();
        m_harmonicGroups[freq]->addNoteClassifier(m_noteClassifiersMap[freq]);
    }
}
void FretBoard::setAudioInput(const float *input)
{
    for (auto notecl : m_noteClassifiersMap)
    {
        notecl.second->input = input;
    }
}

void FretBoard::setAudioOutput(float *output)
{
    for (auto notecl : m_noteClassifiersMap)
    {
        notecl.second->output = output;
    }

}

void FretBoard::setMidiOutput(LV2_Atom_Sequence *output)
{
    if (m_midioutput)
    {
        m_midioutput->setMidiOutput(output);

        for (auto notecl : m_noteClassifiersMap)
        {
            notecl.second->setMidiOutput(m_midioutput);
        }
    }
}

void FretBoard::initialize()
{
    if(m_midioutput)
        m_midioutput->initializeSequence();
    for (auto notecl : m_noteClassifiersMap)
    {
        notecl.second->initialize();    
    }
}

void FretBoard::finalize()
{
    for (auto notecl : m_noteClassifiersMap)
    {
        notecl.second->finalize();    
    }
}

void FretBoard::process(int nsamples)
{
    m_midioutput->initializeSequence();
    for (auto notecl : m_noteClassifiersMap)
    {
        notecl.second->process(nsamples);    
        notecl.second->setIsRinging(nsamples);
    }
    for(auto group:m_harmonicGroups)
    {
        group.second->process(nsamples);
    }
}