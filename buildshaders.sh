(set -o igncr) 2>/dev/null && set -o igncr; # comment is needed on Windows to ignore this lines trailing \r

if [ "$(uname)" == "Linux" ]; then
	exit 0
elif [ "$(uname)" != "Darwin" ]; then
	# Windows has a number of compatibility layers with different uname results.
	EXE=.exe
fi

FOLDER=$(dirname $0)
$FOLDER/Rendering/ShaderCompiler/ShaderCompiler$EXE /P $FOLDER/shaders glsl glsl3 glsles glsles3
