#!/bin/sh
set -eu

baseline_source=${1:?Supply the baseline source directory.}
candidate_source=${2:?Supply the candidate source directory.}
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT

build_variant() {
	name=$1
	source_dir=$2
	build_dir="$work_dir/$name-build"

	cmake -S "$source_dir" -B "$build_dir" -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DLSL_UNITTESTS=OFF \
		-DLSL_FETCH_PUGIXML=OFF \
		-DLSL_OPTIMIZATIONS=ON >/dev/null
	cmake --build "$build_dir" -j4 >/dev/null
	g++ -std=c++14 -O3 -flto \
		-I"$source_dir/include" "$script_dir/benchmark.cpp" \
		-L"$build_dir" -llsl -pthread -Wl,-rpath,"$build_dir" \
		-o "$work_dir/benchmark-$name"
}

build_variant baseline "$baseline_source"
build_variant candidate "$candidate_source"

for run in \
	"baseline default" \
	"candidate default" \
	"candidate default" \
	"baseline default" \
	"baseline sync" \
	"candidate sync" \
	"candidate sync" \
	"baseline sync"
do
	set -- $run
	echo "$1-$2"
	"$work_dir/benchmark-$1" "$2" 2>/dev/null
done
