#!/bin/bash

CLANG_FORMAT_ARGS="--dry-run -Werror"
if [ "$1" == "--fix" ]; then
	echo "--fix was specified, cleaning up formatting instead of checking it."
	CLANG_FORMAT_ARGS="-i"
fi

find . \
	-path ./thirdparty -prune -o \
	-path ./.git -prune -o \
	-path ./build -prune -o \
	-regex '.*\.\(c\|cpp\|h\)' -exec clang-format-11 -style=file $CLANG_FORMAT_ARGS {} \;

