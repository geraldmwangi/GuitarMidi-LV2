import numpy as np
import seaborn as sns
from scipy import signal
num_frets=13
num_strings=6
num_harmonics=4 
class Filter:
    def __init__(self, fret,stringid,harmonic,center_freq, bw,sample_rate):
        self.id=fret*num_strings*num_harmonics+stringid*num_harmonics+harmonic
        self.sample_rate=sample_rate
            
        # create the filter
        N = 2
        low = (center_freq-bw/2) 
        high = (center_freq+bw/2) 
        #self.b, self.a = signal.ellip(N,0.5,40,[low, high], btype='band',fs=sampleRate)
        self.b, self.a =signal.butter(N, [low, high], btype='band',fs=sample_rate)
        
    def process(self,input_audio,filterbank_out: np.array):
        f=np.abs(signal.filtfilt(self.b, self.a, input_audio))
        filterbank_out[self.id]=f


class HarmonicGroup:
    def __init__(self,fret,stringid ,center_freq, bw,sample_rate):
        self.harmonics=[]
        
        for h in range(1,num_harmonics+1):
            self.harmonics.append(Filter(fret,stringid,h-1,center_freq*h,bw,sample_rate))
    
            
    def process(self, input_audio, filterbank_out: np.array):
        res=filterbank_out
        for h in self.harmonics:
            #filterbank_out.append(h.process(input_audio))
            h.process(input_audio,filterbank_out)
            
        return res
    def get_num_filters(self):
        return len(self.harmonics)
          
            
class Fret:
    def __init__(self,fret,s0,s1,s2,s3,s4,s5, bw,sample_rate):
        
        self.strings=[]
        self.strings.append(HarmonicGroup(fret,0,s0,bw,sample_rate))
   
        self.strings.append(HarmonicGroup(fret,1,s1,bw,sample_rate))

        self.strings.append(HarmonicGroup(fret,2,s2,bw,sample_rate))

        self.strings.append(HarmonicGroup(fret,3,s3,bw,sample_rate))

        self.strings.append(HarmonicGroup(fret,4,s4,bw,sample_rate))

        self.strings.append(HarmonicGroup(fret,5,s5,bw,sample_rate))

        
    def process(self, input_audio, filterbank_out: np.array):
    
        for h in self.strings:
            #filterbank_out.append(h.process(input_audio,filterbank_out))
            h.process(input_audio,filterbank_out)
      
    def get_num_filters(self):
        res=0
        for h in self.strings:
            res=res+h.get_num_filters()
            
        return res
            
class FretBoard:
    def __init__(self,bw,sample_rate):
        self.frets=[]
       
        self.frets.append(Fret(0,82,11,147,196,247,329,bw,sample_rate))
        self.frets.append(Fret(1,87,117,156,208,262,349,bw,sample_rate))
        self.frets.append(Fret(2,92,123,165,220,277,370,bw,sample_rate))
        self.frets.append(Fret(3,98,131,175,233,294,392,bw,sample_rate))
        self.frets.append(Fret(4,104,139,185,247,311,415,bw,sample_rate))
        self.frets.append(Fret(5,110,147,196,262,329,440,bw,sample_rate))
        self.frets.append(Fret(6,117,156,208,277,349,466,bw,sample_rate))
        self.frets.append(Fret(7,123,165,220,294,370,494,bw,sample_rate))
        self.frets.append(Fret(8,131,175,233,311,392,523,bw,sample_rate))
        self.frets.append(Fret(9,139,185,247,329,415,554,bw,sample_rate))
        self.frets.append(Fret(10,147,196,262,349,440,587,bw,sample_rate))
        self.frets.append(Fret(11,156,208,277,370,466,622,bw,sample_rate))
        self.frets.append(Fret(12,165,220,294,392,494,659,bw,sample_rate))
        
    def process(self, input_audio, filterbank_out: np.array):
      
        for h in self.frets:
            # filterbank_out.append(h.process(input_audio,filterbank_out))    
            res=h.process(input_audio,filterbank_out)
       
    def get_num_filters(self):
        res=0
        for h in self.frets:
            res=res+h.get_num_filters()
            
        return res