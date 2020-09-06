/* 0BSD 2020 Denys Nykula <nykula@ukr.net> */
#include "libavfilter/buffersink.h"
#include "libavutil/opt.h"
#include <SDL.h>
int main(int argc, char **argv) {
  AVFilterContext *dry[1] = {0}, *drysplit[1] = {0}, *room[1] = {0},
                  *sink[1] = {0}, *verb[1] = {0}, *verbwet[1] = {0};
  AVFrame *f;
  AVFilterGraph *flt;
  SDL_cond *wait = 0;
  SDL_mutex *wait$ = 0;
  SDL_AudioSpec want = {0};
  if (!(f = av_frame_alloc()) || !(flt = avfilter_graph_alloc()) ||
      !(*dry = avfilter_graph_alloc_filter(flt, avfilter_get_by_name("amovie"),
                                           0)) ||
      !(*drysplit = avfilter_graph_alloc_filter(
            flt, avfilter_get_by_name("asplit"), 0)) ||
      !(*room = avfilter_graph_alloc_filter(flt, avfilter_get_by_name("amovie"),
                                            0)) ||
      !(*sink = avfilter_graph_alloc_filter(
            flt, avfilter_get_by_name("abuffersink"), 0)) ||
      !(*verb = avfilter_graph_alloc_filter(flt, avfilter_get_by_name("amix"),
                                            0)) ||
      !(*verbwet = avfilter_graph_alloc_filter(
            flt, avfilter_get_by_name("afir"), 0)) ||
      !(wait = SDL_CreateCond()) || !(wait$ = SDL_CreateMutex()) ||
      SDL_LockMutex(wait$))
    return printf("bad mem\n"), 1;

  if (av_opt_set(*dry, "filename", argv[argc - 1], 1) < 0 ||
      av_opt_set_int(*dry, "loop", 0, 1) < 0 ||
      av_opt_set(*room, "filename", argv[argc - 2], 1) < 0 ||
      av_opt_set_int_list(*sink, "sample_fmts", ((int[]){1, -1}), -1, 1) < 0 ||
      av_opt_set(*verb, "weights", "20 10", 1) < 0 ||
      avfilter_init_str(*dry, 0) < 0 || avfilter_init_str(*drysplit, 0) < 0 ||
      avfilter_init_str(*room, 0) < 0 || avfilter_init_str(*sink, 0) < 0 ||
      avfilter_init_str(*verb, 0) < 0 || avfilter_init_str(*verbwet, 0) < 0 ||
      avfilter_link(*dry, 0, *drysplit, 0) < 0 ||
      avfilter_link(*drysplit, 0, *verbwet, 0) < 0 ||
      avfilter_link(*room, 0, *verbwet, 1) < 0 ||
      avfilter_link(*drysplit, 1, *verb, 0) < 0 ||
      avfilter_link(*verbwet, 0, *verb, 1) < 0 ||
      avfilter_link(*verb, 0, *sink, 0) < 0 ||
      avfilter_graph_config(flt, 0) < 0)
    return printf("bad graph\n"), 1;

  want.channels = av_buffersink_get_channels(*sink);
  want.freq = av_buffersink_get_sample_rate(*sink);
  if (SDL_OpenAudio(&want, 0))
    return printf("%s\n", SDL_GetError()), 1;

  while (SDL_CondWaitTimeout(wait, wait$, 1)) {
    if ((int)SDL_GetQueuedAudioSize(1) > *f->linesize ||
        av_buffersink_get_frame(*sink, f) < 0)
      continue;
    printf("%s %f\n", argv[argc - 1],
           f->pts * av_q2d(av_buffersink_get_time_base(*sink)));
    SDL_PauseAudio(0), SDL_QueueAudio(1, *f->data, *f->linesize);
  }
}
