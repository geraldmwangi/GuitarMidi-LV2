// -------------------------------------------------------------------------
//
//    Copyright (C) 2009-2014 Fons Adriaensen <fons@linuxaudio.org>
//    
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// -------------------------------------------------------------------------


#include <stdlib.h>
#include <string.h>
#include "audiofile.h"


const char *Audiofile::_typestr [] = { "other", "caf", "wav", "amb", "aiff", "flac" };
const char *Audiofile::_formstr [] = { "other", "16bit", "24bit", "32bit", "float" };
const char *Audiofile::_dithstr [] = { "none", "rect", "tri", "lips" };



Audiofile::Audiofile (void) :
    _sndfile (0),
    _dith_proc (0),
    _dith_buff (0),
    _data_buff (0)
{
    clear ();
}


Audiofile::~Audiofile (void)
{
    close ();
}


int Audiofile::enc_type (const char *s)
{
    for (int i = 1; i < 6; i++)	if (!strcmp (s, _typestr [i])) return i;
    return -1;
}


int Audiofile::enc_form (const char *s)
{
    for (int i = 1; i < 5; i++)	if (!strcmp (s, _formstr [i])) return i;
    return -1;
}


int Audiofile::enc_dith (const char *s)
{
    for (int i = 0; i < 4; i++)	if (!strcmp (s, _dithstr [i])) return i;
    return -1;
}


void Audiofile::clear (void)
{
    _mode = MODE_NONE;
    _type = TYPE_OTHER;
    _form = FORM_OTHER;
    _rate = 0;
    _chan = 0;
    _size = 0;
    _dith_type = 0;
    delete[] _dith_proc;
    delete[] _dith_buff;
    delete[] _data_buff;
    _dith_proc = 0;
    _dith_buff = 0;
    _data_buff = 0;
}


int Audiofile::open_read (const char *name)
{
    SF_INFO I;

    if (_mode) return ERR_MODE;
    if ((_sndfile = sf_open (name, SFM_READ, &I)) == 0) return ERR_OPEN;

    _mode = MODE_READ;
    switch (I.format & SF_FORMAT_TYPEMASK)
    {
    case SF_FORMAT_CAF:
	_type = TYPE_CAF;
	break;
    case SF_FORMAT_WAV:
	_type = TYPE_WAV;
	break;
    case SF_FORMAT_WAVEX:
        if (sf_command (_sndfile, SFC_WAVEX_GET_AMBISONIC, 0, 0) == SF_AMBISONIC_B_FORMAT)
	    _type = TYPE_AMB;
	else
            _type = TYPE_WAV;
	break;
    case SF_FORMAT_AIFF:
	_type = TYPE_AIFF;
	break;
    case SF_FORMAT_FLAC:
	_type = TYPE_FLAC;
	break;
    default:
	_type = TYPE_OTHER;
    }
    switch (I.format & SF_FORMAT_SUBMASK)
    {
    case SF_FORMAT_PCM_16:
	_form = FORM_16BIT;
	break;
    case SF_FORMAT_PCM_24:
	_form = FORM_24BIT;
	break;
    case SF_FORMAT_PCM_32:
	_form = FORM_32BIT;
	break;
    case SF_FORMAT_FLOAT:
	_form = FORM_FLOAT;
	break;
    default:
	_form = FORM_OTHER;
    }
    _rate = I.samplerate;
    _chan = I.channels;
    _size = I.frames;

    return 0;
}


int Audiofile::open_write (const char *name, int type, int form, int rate, int chan)    
{
    SF_INFO I;

    if (_mode) return ERR_MODE;
    if ((rate < 1) || (chan < 1)) return ERR_OPEN;

    switch (type)
    {
    case TYPE_CAF:
	I.format = SF_FORMAT_CAF;
	break;
    case TYPE_WAV:
    case TYPE_AMB:
        I.format = (chan > 2) ? SF_FORMAT_WAVEX : SF_FORMAT_WAV;
	break;
    case TYPE_AIFF:
	I.format = SF_FORMAT_AIFF;
	break;
    case TYPE_FLAC:
	I.format = SF_FORMAT_FLAC;
	break;
    default:
        return ERR_TYPE;
    }
    switch (form)
    {
    case FORM_16BIT:
	I.format |= SF_FORMAT_PCM_16;
	break;
    case FORM_24BIT:
	I.format |= SF_FORMAT_PCM_24;
	break;
    case FORM_32BIT:
	I.format |= SF_FORMAT_PCM_32;
	break;
    case FORM_FLOAT:
	I.format |= SF_FORMAT_FLOAT;
	break;
    default:
        return ERR_FORM;
    }
    I.samplerate = rate;
    I.channels = chan;
    I.sections = 1;
    if ((_sndfile = sf_open (name, SFM_WRITE, &I)) == 0) return ERR_OPEN;
    if (type == TYPE_AMB)
    {
        sf_command (_sndfile, SFC_WAVEX_SET_AMBISONIC, 0, SF_AMBISONIC_B_FORMAT);
    }

    _mode = MODE_WRITE;
    _type = type;
    _form = form;
    _rate = rate;
    _chan = chan;

    return 0;
}


float *Audiofile::get_buffer (void)
{
    if (_mode == MODE_NONE) return 0;
    if (_data_buff == 0) _data_buff = new float [_chan * BUFFSIZE];
    return _data_buff;
}


int Audiofile::set_dither (int type)
{
    if (_mode != MODE_WRITE) return ERR_MODE;
    if (_form != FORM_16BIT) return ERR_FORM; 
    if (type == DITHER_NONE)
    {
	delete[] _dith_proc;
	delete[] _dith_buff;
	_dith_proc = 0;
	_dith_buff = 0;
    }
    else if (_dith_type == DITHER_NONE)
    {
        _dith_proc = new Dither [_chan];
        _dith_buff = new int16_t [_chan * BUFFSIZE]; 
    }
    _dith_type = type;
    return 0;
}


int Audiofile::close (void)
{
    if (_sndfile)
    {
        sf_close (_sndfile);
       _sndfile = 0;
    }
    clear ();
    return 0;
}


int64_t Audiofile::seek (int64_t posit, int mode)
{
    if (!_sndfile) return ERR_MODE;
    if (sf_seek (_sndfile, posit, mode) != posit) return ERR_SEEK;
    return 0;
}


int Audiofile::read (float *data, uint64_t frames)
{
    if (_mode != MODE_READ) return ERR_MODE;
    return sf_readf_float (_sndfile, data, frames);
}


int Audiofile::write (float *data, uint64_t frames)
{
    int       i;
    uint32_t  k, n, r;
    float     *p, v;
    int16_t   *q;
    Dither    *D;

    if (_mode != MODE_WRITE) return ERR_MODE;
    if (_dith_type == DITHER_NONE)
    {
	if (_form != FORM_FLOAT)
	{
  	    for (i = 0; i < _chan; i++)
	    {
		p = data + i;
	        for (k = 0; k < frames; k++)
	        {
		    v = *p;
		    if      (v >  1.0f) v =  1.0f;
		    else if (v < -1.0f) v = -1.0f;
		    *p = v;
		    p += _chan;
		}
	    }
	}
        return sf_writef_float (_sndfile, data, frames);
    }
    else
    {
	n = 0;
	while (frames)
	{
	    k = (frames > BUFFSIZE) ? BUFFSIZE : frames;
  	    p = data;
            q = _dith_buff;
            D = _dith_proc;
	    for (i = 0; i < _chan; i++)
	    {
		switch (_dith_type)
		{
		case DITHER_RECT:
		    D->proc_rectangular (k, p, q, _chan, _chan);
		    break;
		case DITHER_TRIA:
		    D->proc_triangular (k, p, q, _chan, _chan);
		    break;
		case DITHER_LIPS:
		    D->proc_lipschitz (k, p, q, _chan, _chan);
		    break;
		}
		p++;
		q++;
		D++;
	    }
            r = sf_writef_short (_sndfile, _dith_buff, k);
	    n += r;
	    if (r != k) return n;
	    data += k * _chan;
	    frames -= k;
	}
    }
    return 0;
}


