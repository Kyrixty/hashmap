if [[ ! -d src ]]; then
	echo "Cannot find src dir under cwd" >&2
	exit 1
fi

if [[ ! -d out ]]; then
	mkdir out
fi

includeFiles=""
if [[ -d include ]]; then
    includeFiles=$(ls include/*.lib include/*.dll)
fi

build_successful() {
	echo -e "\033[32mBuild success!"
	exit 0
}

build_failure() {
	code=2
	if (( $# > 0 )); then
		code=$1
	fi
	echo -e "\033[31mBuild failed!"
	exit $code
}

files=$(ls src/*.c) || build_failure $?
echo "Found files:"
ls -1 src/*.c
if [[ -d include ]]; then
    ls -1 include/*.lib include/*.dll include/*.h
fi

echo "Building..."
gcc -o out/main.exe $files && build_successful
build_failure $?
