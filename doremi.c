    #include <stdio.h>
    #include <math.h>
    #include <stdlib.h>
    #include <string.h>
    #include <errno.h>
    #include <stdint.h>

    #ifndef M_PI
        #define M_PI 3.14159265358979323846
    #endif
    #define NUM_NOTES 11

    FILE * wavfile_open( const char *filename );
    void wavfile_write( FILE *file, short data[], int length );
    void wavfile_close( FILE *file );
    
    static const int WAVFILE_SAMPLES_PER_SECOND = 44100;
    static const int VOLUME_MAX = 32768;

    struct wavfile_header {
      char     riff_tag[4];
      int      riff_length;     /* ChunkSize */
      char     wave_tag[4];
      char     fmt_tag[4];
      int      fmt_length;      /* Subchunk1Size: 16 for PCM */
      short    audio_format;    /* 1 for PCM */
      short    num_channels;
      int      sample_rate;
      int      byte_rate;
      short    block_align;
      short    bits_per_sample;
      char     data_tag[4];
      int      data_length;     /* Subchunk2Size */
    };
    
    FILE * wavfile_open( const char *filename )
    {
      struct wavfile_header header;
    
      int samples_per_second = WAVFILE_SAMPLES_PER_SECOND;
      int bits_per_sample = 16;
    
      strncpy( header.riff_tag, "RIFF", 4 );
      strncpy( header.wave_tag, "WAVE", 4 );
      strncpy( header.fmt_tag,  "fmt ",  4 );
      strncpy( header.data_tag, "data", 4 );
    
      header.riff_length     = 0;
      header.fmt_length      = 16;
      header.audio_format    = 1;
      header.num_channels    = 1;
      header.sample_rate     = samples_per_second;
      header.byte_rate       = samples_per_second * (bits_per_sample / 8);
      header.block_align     = bits_per_sample / 8;
      header.bits_per_sample = bits_per_sample;
      header.data_length     = 0;
    
      FILE * file = fopen( filename, "w+" );
      if ( !file ) return 0;
    
      fwrite( &header, sizeof(header), 1, file);
      fflush( file );
      return( file );
    }
    
    void wavfile_write( FILE * file, short data[], int length )
    {
      fwrite( data, sizeof(short), length, file );
    }
    
    void wavfile_close( FILE * file )
    {
      int file_length = ftell( file );
    
     /* write Subchunk2Size.
      * Subchunk2Size = NumSamples * NumChannels * BitsPerSample/8;
      */
      int data_length = file_length - sizeof( struct wavfile_header );
      fseek( file, sizeof(struct wavfile_header) - sizeof(int), SEEK_SET );
      fwrite( &data_length, sizeof(data_length), 1, file );
    
     /* write ChunkSize.
      * ChunkSize = 36 + SubChunk2Size, or more precisely:
      * 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size);
      */
      int riff_length = file_length - 8;
      fseek( file, 4, SEEK_SET );
      fwrite( &riff_length, sizeof(riff_length), 1, file );
    
      fclose( file );
    }
    
    /* END Wavefile code, begin music generation */
    
    int note_ctoi (char c)
    {
      /* convert e.g. 'A' to 0, 'G' to 6. Valid range: 'A'-'G' */
      if (c > 64 && c < 72) {
        return c - 65;
      } else return -1;
    }
    
    int accidental_ctoi(char c)
    {
      /* convert 'b' to -1, ' ' to 0, '#' to 1 */
      switch (c) {
        case 'b' :
          return -1;
        case ' ' :
          return 0;
        case '#' :
          return 1;
        default :
          return -2;
      }
    }
    
    static const double SEMITONE_RATIO = 1.059463;  /* twelfth root of 2 */
    static const int    SEMITONES_PER_OCTAVE = 12;
    static const int    SEMITONE_SCALE [] = {
      0,    /* A */
      2,    /* B */
      3,    /* C */
      5,    /* D */
      7,    /* E */
      8,    /* F */
      10    /* G */
    };
    
    struct note {
      double begin;          /* in seconds from beginning of waveform */
      int    octave;         /* range 1-11 */
      char   name;           /* range 'A'-'G' */
      char   accidental;     /* ' ', 'b', '#' */
      double duration;       /* in seconds */
      double attack;         /* in seconds */
      double decay;          /* in seconds */
    };
    
    double get_freq( struct note n )
    {
      double freq_base      = 440.0;  /* Concert A (Fourth octave) */
      int oct_base          = 4;
      int oct_offset        = n.octave - oct_base;
      int semitone_offset   = oct_offset * SEMITONES_PER_OCTAVE + SEMITONE_SCALE[ note_ctoi(n.name) ];
      semitone_offset      += accidental_ctoi(n.accidental);
      double frequency      = freq_base * pow(SEMITONE_RATIO, semitone_offset);
      return frequency;
    }
    
    int main()
    {
      /* Make sure total_duration is long enough for your last note duration, or you'll segfault */
      double total_duration = 5.0;        /* seconds */
      int wsps              = WAVFILE_SAMPLES_PER_SECOND;
      double sample;
      int samples_total     = wsps * total_duration;
      int volume            = VOLUME_MAX * .95;
      short waveform[ samples_total ];
      int i, j,
        note_start, note_finish,
        decay_start, decay_duration,
        attack_finish, attack_duration;
      double attack_factor, decay_factor;

      /* declare and initialize notes */
      struct note notes [NUM_NOTES] = {
        { .begin=0.00, .name='C', .accidental=' ', .octave=3, .duration=1., .attack=.1,  .decay=.5},
        { .begin=0.25, .name='D', .accidental=' ', .octave=3, .duration=1., .attack=.1,  .decay=.5},
        { .begin=0.50, .name='E', .accidental='b', .octave=3, .duration=1., .attack=.1,  .decay=.5},
        { .begin=0.75, .name='F', .accidental=' ', .octave=3, .duration=1., .attack=.1,  .decay=.5},
        { .begin=1.00, .name='G', .accidental=' ', .octave=3, .duration=1., .attack=.1,  .decay=.5},
        { .begin=1.25, .name='A', .accidental='b', .octave=4, .duration=1., .attack=.1,  .decay=.5},
        { .begin=1.50, .name='B', .accidental='b', .octave=4, .duration=1., .attack=.1,  .decay=.5},
        { .begin=1.75, .name='C', .accidental=' ', .octave=4, .duration=1., .attack=.1,  .decay=.5},
        { .begin=2.50, .name='C', .accidental=' ', .octave=3, .duration=2., .attack=.1,  .decay=1.},
        { .begin=2.55, .name='G', .accidental=' ', .octave=3, .duration=2., .attack=.1,  .decay=1.},
        { .begin=2.60, .name='C', .accidental=' ', .octave=4, .duration=2., .attack=.1,  .decay=1.},
      };

      /* Initialize waveform */
      for (i = 0; i < samples_total; i++ ) waveform[i] = 0;
    
      struct note n;
    
      for (j = 0; j < NUM_NOTES; j++) {
        /* for each note */
        n = notes[j];
        note_start       = n.begin * wsps;
        note_finish      = note_start + n.duration * wsps;
        decay_duration   = n.decay * wsps;
        decay_start      = note_finish - decay_duration;
        attack_duration  = n.attack * wsps;
        attack_finish    = note_start + attack_duration;
        attack_factor    = 0;
        decay_factor     = 0;
        double frequency = get_freq(n);

        for (i = note_start; i < note_finish; i++ ) {
          /* calculate new sample */
          double t = (double) i / wsps;
          sample = volume * sin(frequency * t * 2 * M_PI);
          /* attack */
          if (n.attack > 0 && i <= attack_finish) {
            attack_factor = (double)(i - note_start) / (double)attack_duration;
            sample *= attack_factor;
            /* gentle mix */
            waveform[i] = (waveform[i] + sample) * (.95 - .5 * attack_factor);
          } else if (n.decay > 0 && i >= decay_start) {
            /* decay */
            decay_factor = (double)(note_finish - i) / (double)decay_duration;
            sample *= decay_factor;
            /* gentle mix */
            waveform[i] = (waveform[i] + sample) * (.95 - .5 * decay_factor);
          } else {
            /* straight average mix */
            waveform[i] = (waveform[i] + sample) * .5;
          }
          /* Normalize */
          if (waveform[i] > VOLUME_MAX) waveform[i] = VOLUME_MAX;
          if (waveform[i] < -1 * VOLUME_MAX) waveform[i] = -1 * VOLUME_MAX;
        }
      }
    
      FILE * f = wavfile_open("output.wav");
      if( !f ) {
        printf("Couldn't open output.wav for writing: %s", strerror(errno));
        return 1;
      }
    
      wavfile_write( f, waveform, samples_total );
      wavfile_close( f );
    
      return 0;
    }
