"""Deterministic original synthesized ambience/foley. No third-party recordings."""
from pathlib import Path
import wave
import numpy as np

OUT = Path(__file__).resolve().parents[1]/'SourceAssets'/'Authored'/'Audio'
OUT.mkdir(parents=True, exist_ok=True)
RATE = 24000
rng = np.random.default_rng(1927)

def noise(seconds, low, high):
    n = int(seconds*RATE)
    x = rng.normal(0, 1, n)
    f = np.maximum(np.fft.rfftfreq(n,1/RATE),.001)
    response = 1/np.sqrt((1+(low/f)**4)*(1+(f/high)**4))
    return np.fft.irfft(np.fft.rfft(x)*response,n)

def save(name, x, loop=False):
    x = np.asarray(x)
    if loop:
        # Overlap-add the tail into the head: both value and slope stay continuous.
        n = RATE
        w = np.linspace(0,1,n)
        x[:n] = x[-n:]*(1-w) + x[:n]*w
        x = x[:-n]
    x = x/max(np.max(np.abs(x)), .001)*.65
    with wave.open(str(OUT/(name+'.wav')), 'wb') as f:
        f.setnchannels(1); f.setsampwidth(2); f.setframerate(RATE)
        f.writeframes((x*32767).astype('<i2').tobytes())

t = np.arange(25*RATE)/RATE
wind = noise(25,75,1300)*(.50+.20*np.sin(t*.37)+.12*np.sin(t*.91))
save('S_BendWind', wind, True)
water = noise(25,250,3900)*(.6+.10*np.sin(t*3.1)+.08*np.sin(t*8.7))
for start in rng.uniform(0,24,100):
    s = int(start*RATE); n = min(int(.13*RATE),len(t)-s); u = np.arange(n)/RATE
    water[s:s+n] += .12*np.sin(2*np.pi*(650*u+900*u*u))*np.exp(-u*35)
save('S_ChannelWater', water, True)
birds = np.zeros_like(t)
for start in [2.1,2.4,8.3,8.6,14.9,15.2,20.1]:
    s = int(start*RATE); n = int(.22*RATE); u = np.arange(n)/RATE
    phase = 2*np.pi*(1800*u+650*u*u+45/22*np.sin(22*u))
    birds[s:s+n] += np.sin(phase)*np.sin(np.pi*u/.22)**2
save('S_DistantBirds', birds, True)
t = np.arange(int(.30*RATE))/RATE
for name, band in [('S_StepDirt',(180,2600)),('S_StepWood',(80,900))]:
    x = noise(.30,*band)*np.exp(-t*22)
    x += .18*np.sin(2*np.pi*85*t)*np.exp(-t*28)
    x *= np.minimum(t/.007,1)
    x[-240:] *= np.linspace(1,0,240)
    save(name,x)
print('CL_FIELD_AUDIO_AUTHORED')
