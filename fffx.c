/* 0BSD 2020 Denys Nykula <nykula@ukr.net> */
#include "libavfilter/buffersink.h"
#include "libavutil/opt.h"
#include <SDL.h>
int main(int argc, char **argv) {
  AVFilterContext *buf[1] = {0}, *fx[1] = {0}, *sink[1] = {0};
  AVFrame *f;
  AVFilterGraph *flt;
  if (!(f = av_frame_alloc()) || !(flt = avfilter_graph_alloc()) ||
      !(*buf = avfilter_graph_alloc_filter(flt, avfilter_get_by_name("amovie"),
                                           0)) ||
      !(*fx = avfilter_graph_alloc_filter(flt, avfilter_get_by_name("anull"),
                                          0)) ||
      !(*sink = avfilter_graph_alloc_filter(
            flt, avfilter_get_by_name("abuffersink"), 0)))
    return printf("bad mem\n"), 1;

  av_opt_set(*buf, "filename", argv[argc - 1], 1);
  if (avfilter_init_str(*buf, 0) < 0 || avfilter_init_str(*fx, 0) < 0 ||
      avfilter_init_str(*sink, 0) < 0 || avfilter_link(*buf, 0, *fx, 0) < 0 ||
      avfilter_link(*fx, 0, *sink, 0) < 0 || avfilter_graph_config(flt, 0) < 0)
    return printf("bad graph\n"), 1;

  do {
    if (av_buffersink_get_frame(*sink, f) < 0)
      continue;
    printf("%s pts=%f\n", argv[argc - 1],
           f->pts * av_q2d(av_buffersink_get_time_base(*sink)));
  } while (fgetc(stdin) >= 0);
}
