// stream.h - header file for stream functions
#ifndef STREAM_H
#define STREAM_H

int stream_prepare(I_O *io, MHA *mha, char *msg);
int stream_device(int io_dev);
void stream_start(I_O *, STA *);
void stream_init(MHA *mha);
void stream_show(char *line);
void stream_cleanup();
void stream_replace(MHA *mha, VAR *lvl);
void play_wave(I_O *, int);
void sound_check(double, int);

#endif // STREAM_H
