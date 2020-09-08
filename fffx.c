/* 0BSD 2020 Denys Nykula <nykula@ukr.net> */
#include "libavfilter/buffersink.h"
#include "libavutil/opt.h"
#include <SDL.h>
static struct TT { AVFilterGraph *g; } TT;
int fltnew(AVFilterContext **f, const char *n) {
  return !(*f = avfilter_graph_alloc_filter(TT.g, avfilter_get_by_name(n), 0));
}
int main(int argc, char **argv) {
  AVFilterContext *band[1] = {0}, *dry[1] = {0}, *nul[1] = {0},
                  *nulsink[1] = {0}, *room[1] = {0}, *sink[1] = {0},
                  *verb[1] = {0}, *verbsplit[1] = {0}, *verbwet[1] = {0};
  AVFrame *f;
  SDL_cond *wait = 0;
  SDL_mutex *wait$ = 0;
  SDL_AudioSpec want = {0};
  if (!(f = av_frame_alloc()) || !(TT.g = avfilter_graph_alloc()) ||
      fltnew(band, "bandpass") || fltnew(dry, "amovie") ||
      fltnew(nul, "anullsrc") || fltnew(nulsink, "anullsink") ||
      fltnew(room, "amovie") || fltnew(sink, "abuffersink") ||
      fltnew(verb, "amix") || fltnew(verbsplit, "asplit") ||
      fltnew(verbwet, "afir") || !(wait = SDL_CreateCond()) ||
      !(wait$ = SDL_CreateMutex()) || SDL_LockMutex(wait$))
    return printf("bad mem\n"), 1;

  if (av_opt_set(*dry, "filename", argv[argc - 1], 1) ||
      av_opt_set_int(*dry, "loop", 0, 1) ||
      av_opt_set(*room, "filename", argv[argc - 2], 1) ||
      av_opt_set_int_list(*sink, "sample_fmts", ((int[]){1, -1}), -1, 1) ||
      av_opt_set(*verb, "weights", "20 10", 1) || avfilter_init_str(*band, 0) ||
      avfilter_init_str(*dry, 0) || avfilter_init_str(*nul, 0) ||
      avfilter_init_str(*nulsink, 0) || avfilter_init_str(*room, 0) ||
      avfilter_init_str(*sink, 0) || avfilter_init_str(*verb, 0) ||
      avfilter_init_str(*verbsplit, 0) || avfilter_init_str(*verbwet, 0) ||
      avfilter_link(*nul, 0, *nulsink, 0) || avfilter_link(*dry, 0, *band, 0) ||
      avfilter_link(*band, 0, *verbsplit, 0) ||
      avfilter_link(*verbsplit, 0, *verbwet, 0) ||
      avfilter_link(*room, 0, *verbwet, 1) ||
      avfilter_link(*verbsplit, 1, *verb, 0) ||
      avfilter_link(*verbwet, 0, *verb, 1) ||
      avfilter_link(*verb, 0, *sink, 0) || avfilter_graph_config(TT.g, 0))
    return printf("bad graph\n"), 1;

  want.channels = av_buffersink_get_channels(*sink);
  want.freq = av_buffersink_get_sample_rate(*sink);
  if (SDL_OpenAudio(&want, 0))
    return printf("%s\n", SDL_GetError()), 1;

  for (SDL_PauseAudio(0); SDL_CondWaitTimeout(wait, wait$, 1);) {
    if ((int)SDL_GetQueuedAudioSize(1) > f->channels * f->nb_samples * 2 ||
        av_buffersink_get_frame(*sink, f))
      continue;
    printf("%s %f\n", argv[argc - 1],
           f->pts * av_q2d(av_buffersink_get_time_base(*sink)));
    SDL_QueueAudio(1, *f->data, f->channels * f->nb_samples * 2);
  }
}
