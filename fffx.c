/* 0BSD 2020 Denys Nykula <nykula@ukr.net> */
#include "libavfilter/buffersink.h"
#include "libavutil/opt.h"
#include <SDL.h>
static struct TT {
  char buf[16];
  AVFilterGraph *g;
  double ss;
} TT;
int fltnew(AVFilterContext **f, const char *n) {
  return !(*f = avfilter_graph_alloc_filter(TT.g, avfilter_get_by_name(n), 0));
}
int fltset(AVFilterContext *f, char *opt, char *val) {
  printf("%f %s %s %s\n", TT.ss, f->name, opt, val);
  return avfilter_graph_send_command(TT.g, f->name, opt, val, 0, 0, 0);
}
int main(int argc, char **argv) {
  AVFrame *af, *vf;
  AVFilterContext *band[9] = {0}, *dry[9] = {0}, *master = 0, *nul[9] = {0},
                  *nulsink[9] = {0}, *room[9] = {0}, *show[9] = {0},
                  *showsplit[9] = {0}, *sink = 0, *verb[9] = {0},
                  *verbsplit[9] = {0}, *verbwet[9] = {0}, *vol[9] = {0},
                  *vsink[9] = {0};
  long buf[2] = {0};
  SDL_Event ev;
  int i;
  unsigned char *opt = 0;
  SDL_Rect r = {0};
  SDL_Renderer *sdl;
  SDL_Texture *tx;
  double volval[9] = {0};
  SDL_AudioSpec want = {0};
  SDL_Window *win;

  if (argc < 3)
    return printf("usage: fffx reverb.wav 1.wav 2.wav ... 8.wav\n"), 1;
  if (!(TT.g = avfilter_graph_alloc()) || !(af = av_frame_alloc()) ||
      !(vf = av_frame_alloc()) || fltnew(&master, "amix") ||
      fltnew(show, "showfreqs") || fltnew(showsplit, "asplit") ||
      fltnew(&sink, "abuffersink") || fltnew(vol, "volume") ||
      fltnew(vsink, "buffersink") ||
      av_opt_set_int(master, "inputs", 9 - 1, 1) ||
      av_opt_set(*show, "s", "128x32", 1) ||
      av_opt_set(*show, "fscale", "log", 1) ||
      av_opt_set_int_list(sink, "sample_fmts", ((int[]){1, -1}), -1, 1) ||
      av_opt_set(*vol, "volume", "9.0", 1) || avfilter_init_str(master, 0) ||
      avfilter_init_str(*show, 0) || avfilter_init_str(*showsplit, 0) ||
      avfilter_init_str(sink, 0) || avfilter_init_str(*vol, 0) ||
      avfilter_link(master, 0, *vol, 0) ||
      avfilter_link(*vol, 0, *showsplit, 0) ||
      avfilter_link(*showsplit, 0, *show, 0) ||
      avfilter_link(*showsplit, 1, sink, 0) ||
      avfilter_link(*show, 0, *vsink, 0))
    return printf("bad master\n"), 1;
  for (i = 1; i < 9; i++)
    if (fltnew(&band[i], "bandpass") || fltnew(&dry[i], "amovie") ||
        fltnew(&nul[i], "anullsrc") || fltnew(&nulsink[i], "anullsink") ||
        fltnew(&room[i], "amovie") || fltnew(&show[i], "showfreqs") ||
        fltnew(&showsplit[i], "asplit") || fltnew(&verb[i], "amix") ||
        fltnew(&verbsplit[i], "asplit") || fltnew(&verbwet[i], "afir") ||
        fltnew(&vol[i], "volume") || fltnew(&vsink[i], "buffersink") ||
        !(snprintf(TT.buf, 16, "band+%d", i), band[i]->name = strdup(TT.buf)) ||
        !(snprintf(TT.buf, 16, "verb+%d", i), verb[i]->name = strdup(TT.buf)) ||
        !(snprintf(TT.buf, 16, "vol+%d", i), vol[i]->name = strdup(TT.buf)) ||
        av_opt_set_int(band[i], "f", 48 * (int)pow(2, i), 1) ||
        av_opt_set(band[i], "width_type", "h", 1) ||
        av_opt_set_int(band[i], "w", 36 * (int)pow(2, i - 1), 1) ||
        av_opt_set(dry[i], "filename", argv[2 + (i - 1) % (argc - 2)], 1) ||
        av_opt_set_int(dry[i], "loop", 0, 1) ||
        av_opt_set(room[i], "filename", argv[1], 1) ||
        av_opt_set(show[i], "fscale", "log", 1) ||
        av_opt_set(show[i], "s", "128x32", 1) ||
        (snprintf(TT.buf, 6, "%d %d", 32 / 4 * 3, 32 / 4),
         av_opt_set(verb[i], "weights", TT.buf, 1)) ||
        av_opt_set_int(verbwet[i], "dry", 5, 1) ||
        av_opt_set_int(verbwet[i], "wet", 10, 1) ||
        (snprintf(TT.buf, 4, "%.1f", (volval[i] = 1.0)),
         av_opt_set(vol[i], "volume", TT.buf, 1)) ||
        avfilter_init_str(band[i], 0) || avfilter_init_str(dry[i], 0) ||
        avfilter_init_str(nul[i], 0) || avfilter_init_str(nulsink[i], 0) ||
        avfilter_init_str(room[i], 0) || avfilter_init_str(show[i], 0) ||
        avfilter_init_str(showsplit[i], 0) || avfilter_init_str(verb[i], 0) ||
        avfilter_init_str(verbsplit[i], 0) ||
        avfilter_init_str(verbwet[i], 0) || avfilter_init_str(vol[i], 0) ||
        avfilter_init_str(vsink[i], 0) ||
        avfilter_link(nul[i], 0, nulsink[i], 0) ||
        avfilter_link(dry[i], 0, band[i], 0) ||
        avfilter_link(band[i], 0, verbsplit[i], 0) ||
        avfilter_link(verbsplit[i], 0, verbwet[i], 0) ||
        avfilter_link(room[i], 0, verbwet[i], 1) ||
        avfilter_link(verbsplit[i], 1, verb[i], 0) ||
        avfilter_link(verbwet[i], 0, verb[i], 1) ||
        avfilter_link(verb[i], 0, vol[i], 0) ||
        avfilter_link(vol[i], 0, showsplit[i], 0) ||
        avfilter_link(showsplit[i], 0, show[i], 0) ||
        avfilter_link(showsplit[i], 1, master, i - 1) ||
        avfilter_link(show[i], 0, vsink[i], 0))
      return printf("bad ch%d %s\n", i, argv[1 + i]), 1;
  if (avfilter_graph_config(TT.g, 0))
    return printf("bad graph\n"), 1;

  want.channels = av_buffersink_get_channels(sink);
  want.freq = av_buffersink_get_sample_rate(sink);
  if (av_buffersink_get_frame(*vsink, vf))
    return printf("bad frame\n"), 1;
  if (SDL_OpenAudio(&want, 0) ||
      SDL_CreateWindowAndRenderer(128 + 32, 32 * 9, 0, &win, &sdl) ||
      !(tx = SDL_CreateTexture(sdl, SDL_PIXELFORMAT_ABGR8888,
                               SDL_TEXTUREACCESS_STREAMING, 128, 32)) ||
      SDL_SetRenderDrawBlendMode(sdl, SDL_BLENDMODE_BLEND))
    return printf("%s\n", SDL_GetError()), 1;

  for (SDL_PauseAudio(0);;) {
    if ((int)SDL_GetQueuedAudioSize(1) > af->channels * af->nb_samples * 2) {
      SDL_Delay(1);
      continue;
    }
    if (av_frame_unref(af), av_buffersink_get_frame(sink, af))
      return printf("bad frame\n"), 1;
    SDL_QueueAudio(1, *af->data, af->channels * af->nb_samples * 2);
    r.h = 32 * 9, r.w = 128 + 32, r.x = r.y = 0;
    SDL_SetRenderDrawColor(sdl, 0, 0, 0, 255), SDL_RenderFillRect(sdl, &r);
    for (i = 0; i < 9; i++, r.x = 0) {
      if (av_frame_unref(vf), av_buffersink_get_frame(vsink[i], vf))
        return printf("bad ch%d frame\n", i), 1;
      SDL_UpdateTexture(tx, 0, *vf->data, *vf->linesize);
      r.h = 32, r.w = 128, r.y = 32 * i, SDL_RenderCopy(sdl, tx, 0, &r);
      if (!i)
        continue;
      av_opt_get_int(band[i], "f", 1, buf);
      av_opt_get_int(band[i], "w", 1, buf + 1);
      r.x = 128 - pow(128, 1 - 1.0 * *buf / (want.freq / want.channels - 1)) -
            (r.w = 32 - pow(32, 1 - 1.0 * buf[1] / *buf)) / 2;
      r.w = FFMIN(r.w, 128 - r.x);
      SDL_SetRenderDrawColor(sdl, 0, 192, 255, 96), SDL_RenderFillRect(sdl, &r);
      av_opt_get(verb[i], "weights", 1, &opt);
      r.h = r.w = 32 / 8, r.x = 128 - r.w / 2 + 1.0 * atoi((char *)opt);
      r.y += 32 - r.h / 2 - volval[i] / 2.0 * 32;
      SDL_SetRenderDrawColor(sdl, 255, 136, 0, 255);
      SDL_RenderFillRect(sdl, &r), av_free(&opt);
    }
    TT.ss = af->pts * av_q2d(av_buffersink_get_time_base(sink));
    if (SDL_RenderPresent(sdl), !SDL_PollEvent(0)) {
      printf("%f\n", TT.ss);
      continue;
    }
    while (SDL_PollEvent(&ev))
      switch (ev.type) {
      case SDL_KEYDOWN:
        switch (ev.key.keysym.sym) {
        case SDLK_q:
          return 0;
        }
        continue;
      case SDL_MOUSEMOTION:
        if (ev.motion.x < 128 && (i = ev.motion.y / 32) &&
            (ev.motion.state & SDL_BUTTON_LMASK)) {
          snprintf(
              TT.buf, 6, "%ld",
              (*buf = (int)FFMAX(1, (1 - log(128 - ev.motion.x) / log(128)) *
                                        (want.freq / want.channels - 1))));
          snprintf(
              TT.buf + 6, 5, "%d",
              (int)FFMAX(1, (1 - log(32 - ev.motion.y % 32) / log(32)) * *buf));
          if (fltset(band[i], "f", TT.buf) || fltset(band[i], "w", TT.buf + 6))
            return printf("bad opt\n"), 1;
        } else if (ev.motion.x >= 128 && (i = ev.motion.y / 32) &&
                   (ev.motion.state & SDL_BUTTON_LMASK)) {
          ev.motion.x -= 128;
          snprintf(TT.buf, 6, "%d %d", ev.motion.x, 32 - ev.motion.x);
          snprintf(TT.buf + 6, 4, "%.1f",
                   (volval[i] = (32 - ev.motion.y % 32) * 2.0 / 32));
          if (fltset(verb[i], "weights", TT.buf) ||
              fltset(vol[i], "volume", TT.buf + 6))
            return printf("bad opt\n"), 1;
        }
        continue;
      case SDL_QUIT:
        return 0;
      }
  }
}
