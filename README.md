# Rubberband Emscripten Wrapper

 Rubberband ([https://breakfastquay.com/rubberband/](website)) compiled to javascript using emscripten. This is work in an early stage of development.

## Disclaimer
This readme is written for me to jog my memory. Nothing in this repository is yet intended to be useful to anybody else. 

## Compiling to javascript

This is the current build process. See "emscripten.build.sh" for updates.

```
emcc -s USE_SDL=2 -DHAVE_LIBSAMPLERATE -s EXTRA_EXPORTED_RUNTIME_METHODS="['ccall']" -s "EXPORTED_FUNCTIONS=['_mainf']" -I../libsndfile/emscripten/src/ -I../libsamplerate/src/ -DUSE_KISSFFT -DNO_THREADING -DNO_TIMING  -I. -Isrc -Irubberband  src/rubberband-c.cpp src/RubberBandStretcher.cpp src/StretcherProcess.cpp src/StretchCalculator.cpp src/base/Profiler.cpp src/dsp/AudioCurveCalculator.cpp src/audiocurves/CompoundAudioCurve.cpp src/audiocurves/SpectralDifferenceAudioCurve.cpp src/audiocurves/HighFrequencyAudioCurve.cpp src/audiocurves/SilentAudioCurve.cpp  src/audiocurves/ConstantAudioCurve.cpp src/audiocurves/PercussiveAudioCurve.cpp src/dsp/Resampler.cpp src/dsp/FFT.cpp src/system/Allocators.cpp src/system/sysutils.cpp src/system/Thread.cpp  src/system/VectorOpsComplex.cpp src/StretcherChannelData.cpp src/StretcherImpl.cpp src/speex/resample.c main/playSDL.cpp main/main.cpp src/kissfft/*.c ../libsndfile/emscripten/libsndfile.a ../libsamplerate/emscripten/libsamplerate.a -o rubberband.js 

```

## Example html
Example html that uses react is in the making. Should be possible to just "npm install" and "npm run start" soon...

## TODOs
- Use web audio for file loading instead of libsndfile
- Add SDL for samplerate conversion and maybe playback too? (SDL is included with emscripten).
- Create nice pitch/stretch wrapper API for javacript.
