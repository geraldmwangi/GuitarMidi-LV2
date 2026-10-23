// ----------------------------------------------------------------------------
//
//  Copyright (C) 2010-2020 Fons Adriaensen <fons@linuxaudio.org>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ----------------------------------------------------------------------------


#ifndef __LFQUEUE_H
#define __LFQUEUE_H


#include <stdint.h>
#include <string.h>


// Base class for lock-free queues.
//
// The logic implemented here is somewhat different from
// how e.g. Jack's lock-free queues work. It does nothing
// to stop the user overflowing or underflowing the queue.
// Avoiding these conditions is easy enough, and allowing
// them can be useful in some applications.
// The logic keeps correct read/write counts as long as the 
// over/underflow is less than 2^31 elements.
// Even if data is corrupted, this allows the client process
// to recover while maintaining a defined latency. 
// To make this work, the logical queue size (the number of
// elements, regardless of element size in bytes) must be a
// power of two. The constructor will if necessary round up
// the size.


class Queuebase
{
public:

    
    Queuebase (int32_t nelm);
    ~Queuebase (void);
    
    // Return queue size.
    int32_t nelm (void) const { return _nelm; }

    // Reset queue state to empty.
    void reset (void)
    {
	_kwrite = 0;
	_kread = 0;
    }
    
    // Return number of elements that can be written
    // without overflow. A negative value indicates
    // that the queue is already in overflow.
    int32_t write_avail (void) const
    {
	return _nelm - _kwrite + _kread;
    }
    
    // Return number of elements that can be written
    // without wraparound. Note: this only reflects
    // the postion of the write pointer relative to
    // the end of the buffer, so it can be more than
    // write_avail().
    int32_t write_nowrap (void) const
    {
        return _nelm - (_kwrite & _mask);
    }
    
    // Adjust queue state, reflecting nelm elements
    // have been written.
    void write_commit (int32_t nelm)
    {
	_kwrite += nelm;
    }    
    
    // Return number of elements that can be read
    // without underflow. A negative value indicates
    // that the queue is already in underflow.
    int32_t read_avail (void) const
    {
	return _kwrite - _kread;
    }
   
    // Return number of elements that can be read
    // without wraparound. Note: this only reflects
    // the position of the read pointer relative to
    // the end of the buffer, so it can be more than
    // read_avail().
    int32_t read_nowrap (void) const
    {
	return _nelm - (_kread & _mask);
    }
   
    // Adjust queue state, reflecting nelm elements
    // have been read.
    void read_commit (int32_t nf)
    {
	_kread += nf;
    }    
    
protected:

    int32_t   _nelm;
    int32_t   _mask;
    int32_t   _kwrite;
    int32_t   _kread;
};


// Multichannel lock-free audio sample queue. 
// Channels can be separate or interleaved.
// The way this queue works is different from
// Jack's lock-free queues in two ways:
//
// 1. See Queuebase above.
//
// 2. For reading or writing, only a pointer is
//    provided, so the user has to do the work.
//    In many cases this can avoid to need for
//    intermediate copies. It also means that
//    wraparound is exposed to the user, but
//    handling this is quite easy.
//
// Note that all methods inherited from Queuebase
// return a number of frames, not samples.





class Audioqueue : public Queuebase
{
public:

    Audioqueue (int32_t minsize, int nchannel, bool interleaved);
    ~Audioqueue (void);

    int32_t nchan (void) const { return _nchan; }

    float *write_ptr (int ch = 0) const
    {
	uint32_t k = _kwrite & _mask;
	if (_inter) k = k * _nchan + ch;
	else        k += ch * _nelm;
	return _data + k;
    }

    float *read_ptr (int ch = 0) const
    {
	uint32_t k = _kread & _mask;
	if (_inter) k = k * _nchan + ch;
	else        k += ch * _nelm;
	return _data + k;
    }

private:	
	
    int32_t   _nchan;
    bool      _inter;
    float    *_data;
};


#endif

