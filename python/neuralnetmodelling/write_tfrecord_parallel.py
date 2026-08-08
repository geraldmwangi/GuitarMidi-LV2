# GuitarMidi-LV2 Library
 # Copyright (C) 2026 Gerald Mwangi
 #
 # This program is free software; you can redistribute it and/or
 # modify it under the terms of the GNU Lesser General Public
 # License as published by the Free Software Foundation; either
 # version 2 of the License, or (at your option) any later version.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 # Lesser General Public License for more details.
 #
 # You should have received a copy of the GNU Lesser General
 # Public License along with this program; if not, write to the
 # Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 # Boston, MA  02110-1301  USA
import os
import math
import queue
import threading
import multiprocessing as mp
from multiprocessing import Pool, Queue, Manager
from pathlib import Path
from typing import Iterator
import numpy as np
import tensorflow as tf
import soundfile as sf
from common import OUTPUT_DIM_NOTES, SAMPLERATE, INPUT_SHAPE, Q_FACTOR, FRAME_LAG,make_proto
from fretboardnonredundant import FretBoard

# Worker initializer
_worker_fretboard: FretBoard|None=None
_worker_audio_cache: dict={}
_worker_feature_map: np.ndarray|None=None
_CACHE_MAX=8 #max files per worker

# Initialize a worker. Called once when the worker starts
def _worker_init():
    global _worker_fretboard,_worker_feature_map
    _worker_fretboard=FretBoard(Q_FACTOR,SAMPLERATE)
    _worker_feature_map=np.zeros((INPUT_SHAPE[0],INPUT_SHAPE[1]),dtype=np.float32)
    _worker_audio_cache.clear()



def _get_audio_worker(audio_path:str) ->np.ndarray:
    if audio_path not in _worker_audio_cache:
        if len(_worker_audio_cache) >=_CACHE_MAX:
            _worker_audio_cache.clear()
        audio,_ =sf.read(audio_path)
        _worker_audio_cache[audio_path]=audio.astype(np.float32)
    return _worker_audio_cache[audio_path]

def _run_filterbank(audio_segment: np.ndarray, start: int, end:int) ->np.ndarray:
    global _worker_fretboard, _worker_feature_map
    _worker_feature_map.fill(0.0)
    hop=INPUT_SHAPE[1]
    _worker_fretboard.reset()
    for frame in range(start,end):
        relative_frame=frame-start
        chunk=audio_segment[relative_frame*hop:(relative_frame+1)*hop]
        if len(chunk)<hop:
            chunk=np.pad(chunk,(0,hop-len(chunk)))
        _worker_fretboard.process(chunk,_worker_feature_map)
    return np.clip(_worker_feature_map*127,-128,127).astype(np.int8)# consider using uint8 since the filterbank output is non-negative, but int8 allows for easier handling of negative values if needed in the future





def _fast_stretch(data: np.ndarray, factor: float) -> np.ndarray:
    datalen=len(data)
    idx=np.clip(np.arange(datalen)*factor,0, datalen-1)
    return np.interp(idx,np.arange(datalen),data)

# Top level process function. Argument is the tuple (audio_path_str,frame_nr,label_bytes, augment)
def process_sample(audio_frame_label_aug: tuple) -> list[bytes]:
    audio_path_str,frame_nr,label_bytes,augment=audio_frame_label_aug

    audio=_get_audio_worker(audio_path_str)
    total_frames=audio.shape[0]//INPUT_SHAPE[1]
    hop=INPUT_SHAPE[1]

    start_frame=max(0,frame_nr-FRAME_LAG)
    end_frame=min(frame_nr+1,total_frames)

    # Original audio
    segment=audio[start_frame*hop:end_frame*hop]
    filtered_orig=_run_filterbank(segment,start_frame,end_frame)
    protos=[make_proto(filtered_orig,label_bytes)]
    if augment:
        # pitchup=82.0/84.5 #pitch slightly higher than note E, but still lower than F, to avoid crossing note boundaries
        # pitchdown=87.0/84.5 # pitch lower than note F, but higher than E, to avoid crossing note boundaries
        # factor= np.random.uniform(pitchup,pitchdown)
        max_cents = 15
        factor = np.random.uniform(2**(-max_cents/1200), 2**(max_cents/1200))

        # get more audio incase pitch is shifted upwards, audio is squeezed
        end_aug=min(frame_nr+2,total_frames)
        seg_aug=audio[start_frame*hop:end_aug*hop]
        seg_aug=_fast_stretch(seg_aug,factor)

        #trim back to expected length
        seg_aug=seg_aug[:((end_frame-start_frame)*hop)]
        filtered_aug=_run_filterbank(seg_aug,start_frame,end_frame)
        protos.append(make_proto(filtered_aug,label_bytes))
    return protos


def _sample_generator(dataset: tf.data.Dataset, augment: bool) ->Iterator[tuple]:
    for audio_batch,frame_nr_batch,label_batch in dataset:
        for audio,frame_nr,label in zip(audio_batch.numpy(),frame_nr_batch.numpy(),label_batch.numpy()):
            yield(
                audio.decode("utf-8"),
                int(frame_nr),
                label.tobytes(),
                augment
            )

_WRITER_SENTINEL=None

def _writer_thread_func(write_queue: queue.Queue, output_prefix: str, records_per_file: int)->None:
    writer=None
    file_index=0
    record_count=0

    def _new_writer(f_idx: int)->tf.io.TFRecordWriter:
        path=f"{output_prefix}_{f_idx:05d}.tfrecord"
        return tf.io.TFRecordWriter(path)
    
    try: 
        while True:
            item=write_queue.get()
            if item is _WRITER_SENTINEL:
                break
            for proto in item:
                if writer is None or record_count >= records_per_file:
                    if writer is not None:
                        writer.close()
                    writer=_new_writer(file_index)
                    file_index+=1
                    record_count=0

                writer.write(proto)
                record_count+=1
    finally:
        if writer is not None:
            writer.close()
        print(f"[Writer]  closed {file_index} shards", flush=True)

def write_tfrecord_parallel(
        dataset: tf.data.Dataset,
        output_prefix: str,
        records_per_file: int=1000,
        num_workers: int=0,
        queue_depth: int=512,
        augment: bool=True,
        chunksize: int=8
)->None:
    # Make the parent dir
    Path(output_prefix).parent.mkdir(parents=True,exist_ok=True)

    if num_workers<=0:
        num_workers=max(1,os.cpu_count()-2)
    print(f"Write parallel using {num_workers} workers", flush=True)

    write_queue=queue.Queue(maxsize=queue_depth)

    writer_thread=threading.Thread(
        target=_writer_thread_func,
        args=(write_queue,output_prefix,records_per_file),daemon=True
    )

    writer_thread.start()

    args_iter=_sample_generator(dataset,augment)

    total=0

    try:
        with Pool(
            processes=num_workers,
            initializer=_worker_init,
        ) as pool:
            for protos in pool.imap_unordered(
                process_sample,args_iter,chunksize=chunksize
            ):
                write_queue.put(protos)
                total+=len(protos)
                if total%5000==0:
                    print(f"{total} protos queued ...", flush=True)
    finally:
        write_queue.put(_WRITER_SENTINEL)
        writer_thread.join()
        print("End")
    print(f"Writting tfrecords done -- {total} protos written", flush=True)