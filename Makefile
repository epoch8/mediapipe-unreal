# Перед билдом в файле mediapipe/mediapipe/unreal/BUILD
# указать пути к графам стандартных моделей из медиапайпа, какие нужны
# В C:\\opencv\\build должен быть opencv, а в C://Python310//python.exe -- питон
# Далее запускаем эту команду для билда ump_shared:
unreal-build-cpu:
	bazel build -c opt --define MEDIAPIPE_DISABLE_GPU=1 --action_env PYTHON_BIN_PATH=C://Python310//python.exe mediapipe/unreal:ump_shared
