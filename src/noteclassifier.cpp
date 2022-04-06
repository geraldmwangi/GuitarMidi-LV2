#include <noteclassifier.hpp>

NoteClassifier::NoteClassifier(float samplerate, float center, float bandwidth, float passbandatten)
{
    m_centerfreq = center;
    m_passbandatten = passbandatten;
    m_bandwidth = bandwidth;
    m_samplerate = samplerate;
    mPitchDetector = 0;
    m_pitchbuffer = 0;
    m_pitchBufferCounter = 0;
    mBufferSize = 512;
    m_noteOnOffState = false;
}

void NoteClassifier::initialize()
{
    for (int i = 0; i < FILTERORDER; i++)
    {
        m_filter[i].reset();
        m_filter[i].setup(1, m_samplerate, m_centerfreq, m_bandwidth, m_passbandatten, 60.0);
    }

    if (mPitchDetector)
        del_aubio_pitch(mPitchDetector);

    mInBufSize = mBufferSize * 4;

    mPitchDetector = new_aubio_pitch("schmitt", mInBufSize, mBufferSize, m_samplerate);
    if (m_pitchbuffer)
        delete[] m_pitchbuffer;
    m_pitchbuffer = new float[mInBufSize];
    m_pitchBufferCounter = 0;

    if (m_pitchfreq)
        del_fvec(m_pitchfreq);
    m_pitchfreq = new_fvec(1);
}

void NoteClassifier::finalize()
{
    for (int i = 0; i < FILTERORDER; i++)
        m_filter[i].reset();
    if (mPitchDetector)
        del_aubio_pitch(mPitchDetector);
    if (m_pitchbuffer)
        delete[] m_pitchbuffer;
    if (m_pitchfreq)
        del_fvec(m_pitchfreq);
}

void NoteClassifier::process(int nsamples)
{
    memcpy(output, input, nsamples * sizeof(float));

    // //rectify signal
    // for(int s=0;s<nsamples;s++)
    //     output[s]=fabs(output[s]);

    for (int i = 0; i < FILTERORDER; i++)
        m_filter[i].process(nsamples, &output);

    float meanenv = 0;
    int count = 0;

    for (int s = 1; s < (nsamples - 1); s++)
    {
        if (output[s] > output[s - 1] && output[s] > output[s + 1] && output[s] > 0)
        {
            meanenv += output[s];
            count++;
        }
    }
    if (count)
        meanenv /= count;

    if (meanenv > 0.05)
    {
        memcpy(m_pitchbuffer + m_pitchBufferCounter, output, sizeof(float) * nsamples);
        m_pitchBufferCounter += nsamples;
        if (m_pitchBufferCounter >= mInBufSize)
        {
            m_pitchBufferCounter = 0;
            fvec_t Buf;
            Buf.data = m_pitchbuffer;
            Buf.length = mInBufSize;

            aubio_pitch_do(mPitchDetector, &Buf, m_pitchfreq);

            if (fabs(m_pitchfreq->data[0] - m_centerfreq) <= 4.0)
            {
                m_noteOnOffState = true;
                // printf("got signal: %f, freq: %f\n", meanenv, m_pitchfreq->data[0]);
            }
            else
                m_noteOnOffState = false;
        }
    }
    else
        m_noteOnOffState = false;
    if (m_noteOnOffState)
        printf("Note: %f on\n", m_centerfreq);
    else
        printf("Note: %f off\n", m_centerfreq);
    if (m_noteOnOffState != m_oldNoteOnOffState)
        if (m_noteOnOffState)
            printf("Note: %f on", m_centerfreq);
        else
            printf("Note: %f off", m_centerfreq);
    m_oldNoteOnOffState = m_noteOnOffState;
}