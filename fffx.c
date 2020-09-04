/* 0BSD 2020 Denys Nykula <nykula@ukr.net> */
#include "libavfilter/buffersink.h"
#include "libavutil/opt.h"
#include <SDL.h>
int main(int argc, char **argv) {
  AVFilterContext *buf = 0, *sink = 0;
  AVFilterInOut *bufs[1] = {0}, *sinks[1] = {0};
  AVFilterGraph *flt;
  if (!(flt = avfilter_graph_alloc()) || !(*bufs = avfilter_inout_alloc()) ||
      !(*sinks = avfilter_inout_alloc()))
    return printf("bad mem\n"), 1;
  avfilter_graph_create_filter(&buf, avfilter_get_by_name("anullsrc"), "buf", 0,
                               0, flt);
  (*bufs)->filter_ctx = buf, (*bufs)->name = av_strdup("in");
  avfilter_graph_create_filter(&sink, avfilter_get_by_name("abuffersink"),
                               "sink", 0, 0, flt);
  (*sinks)->filter_ctx = sink, (*sinks)->name = av_strdup("out");
  if (avfilter_graph_parse_ptr(flt, "anull", sinks, bufs, 0) < 0 ||
      avfilter_graph_config(flt, 0) < 0)
    return printf("bad graph\n"), 1;
  printf("%s\n", argv[--argc]);
}
