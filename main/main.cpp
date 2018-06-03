/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2018 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for the
    Rubber Band Library obtained by agreement with the copyright
    holders, you may redistribute and/or modify it under the terms
    described in that licence.

    If you wish to distribute code using the Rubber Band Library
    under terms other than those of the GNU General Public License,
    you must obtain a valid commercial licence before doing so.
*/

#include "rubberband/rubberband-c.h"

#include <emscripten.h>
#include <iostream>
#include <sndfile.h>
#include <cmath>
#include <time.h>
#include <cstdlib>
#include <cstring>
#include <string>

#include <fstream>

#include "system/sysutils.h"

#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>

#include "base/Profiler.h"

using namespace std;
using namespace RubberBand;

RubberBandState rb;
SNDFILE *sndfile;
SNDFILE *sndfileOut;
SF_INFO sfinfo;
SF_INFO sfinfoOut;
int ibs = 1024;
size_t channels;
float *fbuf;
float **ibuf;
int frame = 0;
int percent = 0;
size_t countIn = 0, countOut = 0;
timeval tv;

extern "C" void playSDL(); 

double tempo_convert(const char *str)
{
    char *d = strchr((char *)str, ':');

    if (!d || !*d)
    {
        double m = atof(str);
        if (m != 0.0)
            return 1.0 / m;
        else
            return 1.0;
    }

    char *a = strdup(str);
    char *b = strdup(d + 1);
    a[d - str] = '\0';
    double m = atof(a);
    double n = atof(b);
    free(a);
    free(b);
    if (n != 0.0 && m != 0.0)
        return m / n;
    else
        return 1.0;
}

extern "C" void flush()
{

    cout << "\r    " << endl;

    int avail;

    while ((avail = rubberband_available(rb)) >= 0)
    {

        cout << "(completing) available = " << avail << endl;

        if (avail > 0)
        {
            float **obf = new float *[channels];
            for (size_t i = 0; i < channels; ++i)
            {
                obf[i] = new float[avail];
            }
            rubberband_retrieve(rb, obf, avail);
            countOut += avail;
            float *fobf = new float[channels * avail];
            for (size_t c = 0; c < channels; ++c)
            {
                for (int i = 0; i < avail; ++i)
                {
                    float value = obf[c][i];
                    if (value > 1.f)
                        value = 1.f;
                    if (value < -1.f)
                        value = -1.f;
                    fobf[i * channels + c] = value;
                }
            }

            sf_writef_float(sndfileOut, fobf, avail);
            delete[] fobf;
            for (size_t i = 0; i < channels; ++i)
            {
                delete[] obf[i];
            }
            delete[] obf;
        }
        else
        {
            usleep(10000);
        }
    }

    sf_close(sndfile);
    sf_close(sndfileOut);

    //cout << "in: " << countIn << ", out: " << countOut << ", ratio: " << float(countOut) / float(countIn) << ", ideal output: " << lrint(countIn * ratio) << ", error: " << abs(lrint(countIn * ratio) - int(countOut)) << endl;

    timeval etv;
    (void)gettimeofday(&etv, 0);

    etv.tv_sec -= tv.tv_sec;
    if (etv.tv_usec < tv.tv_usec)
    {
        etv.tv_usec += 1000000;
        etv.tv_sec -= 1;
    }
    etv.tv_usec -= tv.tv_usec;

    double sec = double(etv.tv_sec) + (double(etv.tv_usec) / 1000000.0);
    cout << "elapsed time: " << sec << " sec, in frames/sec: " << countIn / sec << ", out frames/sec: " << countOut / sec << endl;
}

extern "C" void mainloop()
{
    int count = -1;
    if (frame >= sfinfo.frames)
    {
        emscripten_cancel_main_loop();

        flush();

        int x = EM_ASM_INT({
            Module.print('I received: ' + $0);
            return $0 + 1;
        },
                           100);

        printf("%d\n", x);

        playSDL();

        //EM_ASM(Module.onMainLoopFinished());

        cout << "Done." << endl;
        return;
    }

    if ((count = sf_readf_float(sndfile, fbuf, ibs)) < 0)
        return;

    countIn += count;

    for (size_t c = 0; c < channels; ++c)
    {
        for (int i = 0; i < count; ++i)
        {
            float value = fbuf[i * channels + c];
            ibuf[c][i] = value;
        }
    }

    bool final = (frame + ibs >= sfinfo.frames);

    rubberband_process(rb, ibuf, count, final);

    int avail = rubberband_available(rb);

    if (avail > 0)
    {
        float **obf = new float *[channels];
        for (size_t i = 0; i < channels; ++i)
        {
            obf[i] = new float[avail];
        }
        rubberband_retrieve(rb, obf, avail);
        countOut += avail;
        float *fobf = new float[channels * avail];
        for (size_t c = 0; c < channels; ++c)
        {
            for (int i = 0; i < avail; ++i)
            {
                float value = obf[c][i];
                if (value > 1.f)
                    value = 1.f;
                if (value < -1.f)
                    value = -1.f;
                fobf[i * channels + c] = value;
            }
        }

        sf_writef_float(sndfileOut, fobf, avail);
        delete[] fobf;
        for (size_t i = 0; i < channels; ++i)
        {
            delete[] obf[i];
        }
        delete[] obf;
    }

    frame += ibs;
}

extern "C" int mainf(int argc, char **argv)
{
    int c;
    double ratio = 2.0;
    double duration = 0.0;
    double pitchshift = 0.0;
    double frequencyshift = 1.0;
    int debug = 0;
    bool realtime = true;
    bool precise = true;
    int threading = 0;
    bool lamination = true;
    bool longwin = false;
    bool shortwin = false;
    bool smoothing = true;
    bool hqpitch = true;
    bool formant = true;
    bool together = false;
    bool crispchanged = false;
    int crispness = 0;
    bool help = false;
    bool version = false;
    bool quiet = false;
    bool haveRatio = true;
    char *fileName = "in.wav";     //strdup(argv[optind++]);
    char *fileNameOut = "out.wav"; //strdup(argv[optind++]);

    enum
    {
        NoTransients,
        BandLimitedTransients,
        Transients
    } transients = Transients;

    enum
    {
        CompoundDetector,
        PercussiveDetector,
        SoftDetector
    } detector = CompoundDetector;

    switch (crispness)
    {
    case -1:
        crispness = 5;
        break;
    case 0:
        detector = CompoundDetector;
        transients = NoTransients;
        lamination = false;
        longwin = true;
        shortwin = false;
        break;
    case 1:
        detector = SoftDetector;
        transients = Transients;
        lamination = false;
        longwin = true;
        shortwin = false;
        break;
    case 2:
        detector = CompoundDetector;
        transients = NoTransients;
        lamination = false;
        longwin = false;
        shortwin = false;
        break;
    case 3:
        detector = CompoundDetector;
        transients = NoTransients;
        lamination = true;
        longwin = false;
        shortwin = false;
        break;
    case 4:
        detector = CompoundDetector;
        transients = BandLimitedTransients;
        lamination = true;
        longwin = false;
        shortwin = false;
        break;
    case 5:
        detector = CompoundDetector;
        transients = Transients;
        lamination = true;
        longwin = false;
        shortwin = false;
        break;
    case 6:
        detector = PercussiveDetector;
        transients = Transients;
        lamination = false;
        longwin = false;
        shortwin = true;
        break;
    };

    memset(&sfinfo, 0, sizeof(SF_INFO));

    sndfile = sf_open(fileName, SFM_READ, &sfinfo);
    if (!sndfile)
    {
        cout << "ERROR: Failed to open input file \"" << fileName << "\": "
             << sf_strerror(sndfile) << endl;
        return 1;
    }

    if (duration != 0.0)
    {
        if (sfinfo.frames == 0 || sfinfo.samplerate == 0)
        {
            cout << "ERROR: File lacks frame count or sample rate in header, cannot use --duration" << endl;
            return 1;
        }
        double induration = double(sfinfo.frames) / double(sfinfo.samplerate);
        if (induration != 0.0)
            ratio = duration / induration;
    }

    sfinfoOut.channels = sfinfo.channels;
    sfinfoOut.format = sfinfo.format;
    sfinfoOut.frames = int(sfinfo.frames * ratio + 0.1);
    sfinfoOut.samplerate = sfinfo.samplerate;
    sfinfoOut.sections = sfinfo.sections;
    sfinfoOut.seekable = sfinfo.seekable;

    sndfileOut = sf_open(fileNameOut, SFM_WRITE, &sfinfoOut);
    if (!sndfileOut)
    {
        cout << "ERROR: Failed to open output file \"" << fileNameOut << "\" for writing: "
             << sf_strerror(sndfileOut) << endl;
        return 1;
    }

    channels = sfinfo.channels;

    RubberBandOptions options = 0;
    if (realtime)
        options |= RubberBandOptionProcessRealTime;
    if (precise)
        options |= RubberBandOptionStretchPrecise;
    if (!lamination)
        options |= RubberBandOptionPhaseIndependent;
    if (longwin)
        options |= RubberBandOptionWindowLong;
    if (shortwin)
        options |= RubberBandOptionWindowShort;
    if (false)
        options |= RubberBandOptionSmoothingOn;
    if (formant)
        options |= RubberBandOptionFormantPreserved;
    if (true)
        options |= RubberBandOptionPitchHighQuality;
    if (true)
        options |= RubberBandOptionChannelsTogether;

    switch (threading)
    {
    case 0:
        options |= RubberBandOptionThreadingAuto;
        break;
    case 1:
        options |= RubberBandOptionThreadingNever;
        break;
    case 2:
        options |= RubberBandOptionThreadingAlways;
        break;
    }

    switch (transients)
    {
    case NoTransients:
        options |= RubberBandOptionTransientsSmooth;
        break;
    case BandLimitedTransients:
        options |= RubberBandOptionTransientsMixed;
        break;
    case Transients:
        options |= RubberBandOptionTransientsCrisp;
        break;
    }

    switch (detector)
    {
    case CompoundDetector:
        options |= RubberBandOptionDetectorCompound;
        break;
    case PercussiveDetector:
        options |= RubberBandOptionDetectorPercussive;
        break;
    case SoftDetector:
        options |= RubberBandOptionDetectorSoft;
        break;
    }

    if (pitchshift != 0.0)
    {
        frequencyshift *= pow(2.0, pitchshift / 12);
    }

    cout << "Using time ratio " << ratio;
    cout << " and frequency ratio " << frequencyshift << endl;

    (void)gettimeofday(&tv, 0);

    rb = rubberband_new(sfinfo.samplerate,
                        channels,
                        options,
                        ratio,
                        frequencyshift);

    rubberband_set_max_process_size(rb, ibs);

    rubberband_set_expected_input_duration(rb, sfinfo.frames);

    fbuf = new float[channels * ibs];
    ibuf = new float *[channels];

    for (size_t i = 0; i < channels; ++i)
        ibuf[i] = new float[ibs];

    sf_seek(sndfile, 0, SEEK_SET);

    //#ifdef __EMSCRIPTEN__
    // void emscripten_set_main_loop(em_callback_func func, int fps, int simulate_infinite_loop);
    emscripten_set_main_loop(mainloop, 120, 0);

    return 0;
}
