#include <fretboard.hpp>

FretBoard::FretBoard(LV2_URID_Map *map, float samplerate)
{
    // E string
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 82.41));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 87.31));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 92.50));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 98.00));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 103.83));

    // // A string
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 110));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 116.54));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 123.47));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 130.81));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 138.59));

    // // D string
     m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 146.83));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 155.56));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 164.81));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 174.61));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 185.00));

    // // g string
     m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 196.00));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 207.65));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 220));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 233.08));

    // // b string
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 246.94));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 261.63));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 277.18));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 293.66));
    m_noteClassifiers.push_back(make_shared<NoteClassifier>(map,samplerate, 311.13));

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
    for (auto notecl : m_noteClassifiers)
    {
        notecl->setMidiOutput(output);
    }
}

void FretBoard::initialize()
{
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
    for (auto notecl : m_noteClassifiers)
        notecl->process(nsamples);
}