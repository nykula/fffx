fffx: fffx.c
	${CC} ${CFLAGS} -Wall -Werror -Wextra -funsigned-char `pkg-config --cflags --libs libavfilter libavutil sdl2` fffx.c ${LDFLAGS} -o fffx

commit:
	git diff
	git diff --stat
	git commit -F- -a

format:
	clang-format -i fffx.c
