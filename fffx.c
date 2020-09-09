/* 0BSD 2020 Denys Nykula <nykula@ukr.net> */
#include "libavfilter/buffersink.h"
#include "libavutil/opt.h"
#include <SDL.h>
static struct TT { AVFilterGraph *g; } TT;
int fltnew(AVFilterContext **f, const char *n) {
  return !(*f = avfilter_graph_alloc_filter(TT.g, avfilter_get_by_name(n), 0));
}
int main(int argc, char **argv) {
  AVFrame *af, *vf;
  AVFilterContext *band[1] = {0}, *dry[1] = {0}, *nul[1] = {0},
                  *nulsink[1] = {0}, *room[1] = {0}, *show[1] = {0},
                  *showsplit[1] = {0}, *sink[1] = {0}, *verb[1] = {0},
                  *verbsplit[1] = {0}, *verbwet[1] = {0}, *vsink[1] = {0};
  SDL_Event ev;
  SDL_Rect r = {0};
  SDL_Renderer *sdl;
  SDL_Texture *tx;
  SDL_AudioSpec want = {0};
  SDL_Window *win;
  if (!(af = av_frame_alloc()) || !(vf = av_frame_alloc()) ||
      !(TT.g = avfilter_graph_alloc()) || fltnew(band, "bandpass") ||
      fltnew(dry, "amovie") || fltnew(nul, "anullsrc") ||
      fltnew(nulsink, "anullsink") || fltnew(room, "amovie") ||
      fltnew(show, "showfreqs") || fltnew(showsplit, "asplit") ||
      fltnew(sink, "abuffersink") || fltnew(verb, "amix") ||
      fltnew(verbsplit, "asplit") || fltnew(verbwet, "afir") ||
      fltnew(vsink, "buffersink"))
    return printf("bad mem\n"), 1;

  if (av_opt_set(*dry, "filename", argv[argc - 1], 1) ||
      av_opt_set_int(*dry, "loop", 0, 1) ||
      av_opt_set(*room, "filename", argv[argc - 2], 1) ||
      av_opt_set(*show, "s", "256x128", 1) ||
      av_opt_set(*show, "colors", "cyan", 1) ||
      av_opt_set_int_list(*sink, "sample_fmts", ((int[]){1, -1}), -1, 1) ||
      av_opt_set(*verb, "weights", "20 10", 1) || avfilter_init_str(*band, 0) ||
      avfilter_init_str(*dry, 0) || avfilter_init_str(*nul, 0) ||
      avfilter_init_str(*nulsink, 0) || avfilter_init_str(*room, 0) ||
      avfilter_init_str(*show, 0) || avfilter_init_str(*showsplit, 0) ||
      avfilter_init_str(*sink, 0) || avfilter_init_str(*verb, 0) ||
      avfilter_init_str(*verbsplit, 0) || avfilter_init_str(*verbwet, 0) ||
      avfilter_init_str(*vsink, 0) || avfilter_link(*nul, 0, *nulsink, 0) ||
      avfilter_link(*dry, 0, *band, 0) ||
      avfilter_link(*band, 0, *verbsplit, 0) ||
      avfilter_link(*verbsplit, 0, *verbwet, 0) ||
      avfilter_link(*room, 0, *verbwet, 1) ||
      avfilter_link(*verbsplit, 1, *verb, 0) ||
      avfilter_link(*verbwet, 0, *verb, 1) ||
      avfilter_link(*verb, 0, *showsplit, 0) ||
      avfilter_link(*showsplit, 0, *show, 0) ||
      avfilter_link(*showsplit, 1, *sink, 0) ||
      avfilter_link(*show, 0, *vsink, 0) || avfilter_graph_config(TT.g, 0))
    return printf("bad graph\n"), 1;

  want.channels = av_buffersink_get_channels(*sink);
  want.freq = av_buffersink_get_sample_rate(*sink);
  if (av_buffersink_get_frame(*vsink, vf))
    return printf("bad frame\n"), 1;
  if (SDL_OpenAudio(&want, 0) ||
      SDL_CreateWindowAndRenderer(256, 256, 0, &win, &sdl) ||
      !(tx = SDL_CreateTexture(sdl, SDL_PIXELFORMAT_ABGR8888,
                               SDL_TEXTUREACCESS_STREAMING, 256, 128)))
    return printf("%s\n", SDL_GetError()), 1;

  for (SDL_PauseAudio(0);;) {
    if ((int)SDL_GetQueuedAudioSize(1) > af->channels * af->nb_samples * 2) {
      SDL_Delay(1);
      continue;
    }
    av_frame_unref(af), av_frame_unref(vf);
    // Why two pictures for each sound?
    if (av_buffersink_get_frame(*sink, af) ||
        av_buffersink_get_frame(*vsink, vf) || (av_frame_unref(vf), 0) ||
        av_buffersink_get_frame(*vsink, vf))
      return printf("bad frame\n"), 1;
    printf("%s %f\n", argv[argc - 1],
           af->pts * av_q2d(av_buffersink_get_time_base(*sink)));
    SDL_QueueAudio(1, *af->data, af->channels * af->nb_samples * 2);
    r.h = r.w = 256, r.y = 0;
    SDL_SetRenderDrawColor(sdl, 0, 0, 0, 255), SDL_RenderFillRect(sdl, &r);
    SDL_UpdateTexture(tx, 0, *vf->data, *vf->linesize);
    r.h = 128, r.w = 256, r.y = 92, SDL_RenderCopy(sdl, tx, 0, &r);

    switch (SDL_RenderPresent(sdl), SDL_PollEvent(&ev), ev.type) {
    case SDL_KEYDOWN:
      switch (ev.key.keysym.sym) {
      case SDLK_q:
        return 0;
      }
      break;
    case SDL_QUIT:
      return 0;
    }
  }
}
