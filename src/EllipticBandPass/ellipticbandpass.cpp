#include "ellipticbandpass.h"
#include <math.h>
START_NAMESPACE_DISTRHO
EllipticBandPass::EllipticBandPass():
    Plugin(4, 0, 0)
{
//    for(int r=0;r<MAXORDER;r++)
//        m_filters[MAXORDER].setOutputBufferSize(getBufferSize());
    m_centerfreq=110;
    m_bandwidth=40;
    m_passbandatten=1;
    m_order=4;
    setFilterParameters();

}

EllipticBandPass::~EllipticBandPass()
{

}

void EllipticBandPass::initParameter(uint32_t index, Parameter &parameter)
{

    if(index==0)
    {
//        m_cutoff=1000.0;
        parameter.name="Center";
        parameter.ranges.min=1;
        parameter.ranges.max=4500.0;
        parameter.unit="Hz";
        parameter.ranges.def=m_centerfreq;
    }
    else if(index==1)
    {
        parameter.name="Bandwidth";
        parameter.ranges.min=0.01;
        parameter.ranges.max=100.0;
        parameter.unit="Hz";
        parameter.ranges.def=m_bandwidth;
    }
    else if(index==2)
    {
        parameter.name="Passband Attenuation";
        parameter.ranges.max=20.0;
        parameter.ranges.min=0.001;
        parameter.unit="dB";
        parameter.ranges.def=m_passbandatten;
    }
    else if(index==3)
    {
        parameter.name="Order";
        parameter.ranges.min=1;
        parameter.ranges.max=MAXORDER;
        parameter.unit="";
        parameter.hints=kParameterIsInteger;
        parameter.ranges.def=m_order;
    }

}

float EllipticBandPass::getParameterValue(uint32_t index) const
{
    if(index==0)
    {
//        m_cutoff=1000.0;
//        parameter.name="Center";
//        parameter.ranges.min=1;
//        parameter.ranges.max=4500.0;
//        parameter.unit="Hz";
//        parameter.ranges.def=m_centerfreq;
        return m_centerfreq;
    }
    else if(index==1)
    {
//        parameter.name="Bandwidth";
//        parameter.ranges.min=1;
//        parameter.ranges.max=100.0;
//        parameter.unit="Hz";
//        parameter.ranges.def=m_bandwidth;
        return m_bandwidth;
    }
    else if(index==2)
    {
//        parameter.name="Passband Attenuation";
//        parameter.ranges.min=0;
//        parameter.ranges.max=10.0;
//        parameter.unit="dB";
//        parameter.ranges.def=m_passbandatten;
        return m_passbandatten;
    }
    else if(index==3)
    {
//        parameter.name="Order";
//        parameter.ranges.min=1;
//        parameter.ranges.max=5;
//        parameter.unit="";
//        parameter.hints=kParameterIsInteger;
//        parameter.ranges.def=m_order;
        return m_order;
    }
}

void EllipticBandPass::setParameterValue(uint32_t index, float value)
{

    if(index==0)
    {
//        m_cutoff=1000.0;
//        parameter.name="Center";
//        parameter.ranges.min=1;
//        parameter.ranges.max=4500.0;
//        parameter.unit="Hz";
//        parameter.ranges.def=m_centerfreq;
        m_centerfreq=value;
    }
    else if(index==1)
    {
//        parameter.name="Bandwidth";
//        parameter.ranges.min=1;
//        parameter.ranges.max=100.0;
//        parameter.unit="Hz";
//        parameter.ranges.def=m_bandwidth;
        m_bandwidth=value;
    }
    else if(index==2)
    {
//        parameter.name="Passband Attenuation";
//        parameter.ranges.min=0;
//        parameter.ranges.max=10.0;
//        parameter.unit="dB";
//        parameter.ranges.def=m_passbandatten;
        m_passbandatten=value;
    }
    else if(index==3)
    {
//        parameter.name="Order";
//        parameter.ranges.min=1;
//        parameter.ranges.max=5;
//        parameter.unit="";
//        parameter.hints=kParameterIsInteger;
//        parameter.ranges.def=m_order;
        m_order=value;
    }
    setFilterParameters();
}

void EllipticBandPass::run(const float **inputs, float **outputs, uint32_t frames)
{

    memcpy(outputs[0],inputs[0],sizeof(float)*frames);
    for(int i=0;i<m_order;i++)
        m_filter[i].process(frames,outputs);


}

void EllipticBandPass::setFilterParameters()
{
    float highf=m_centerfreq+0.5*m_bandwidth;
    float lowf=m_centerfreq-0.5*m_bandwidth;
    for(int i=0;i<m_order;i++)
        m_filter[i].setup(1,getSampleRate(),m_centerfreq,m_bandwidth,m_passbandatten,60.0);



}


Plugin* createPlugin()
{
    return new EllipticBandPass();
}
END_NAMESPACE_DISTRHO
