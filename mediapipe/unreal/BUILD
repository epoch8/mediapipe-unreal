licenses(["notice"])

package(default_visibility = [
	"//visibility:public",
])

cc_library(
	name = "headers",
	hdrs = glob(["*.h"]),
	visibility = ["//mediapipe/unreal:__pkg__"],
)

cc_library(
	name = "ump_core",
	srcs = ["ump_context.cc", "ump_pipeline.cc"],
	deps = [
		"@com_google_absl//absl/flags:flag",
		"@com_google_absl//absl/flags:parse",
		
		"//mediapipe/framework:calculator_framework",
		"//mediapipe/framework/formats:image_frame",
		"//mediapipe/framework/formats:image_frame_opencv",
		"//mediapipe/framework/port:opencv_imgproc",
		"//mediapipe/framework/port:opencv_video",
		"//mediapipe/framework/port:opencv_highgui",
		"//mediapipe/framework/port:parse_text_proto",
		"//mediapipe/framework/port:file_helpers",
		"//mediapipe/framework/port:status",
		
		"//mediapipe/graphs/face_mesh:desktop_live_calculators",
		"//mediapipe/modules/face_geometry:env_generator_calculator",
		"//mediapipe/modules/face_geometry:face_geometry_from_landmarks",
		"//mediapipe/graphs/hand_tracking:desktop_tflite_calculators",
		"//mediapipe/graphs/pose_tracking:pose_tracking_cpu_deps",
		"//mediapipe/graphs/holistic_tracking:holistic_tracking_cpu_graph_deps",
		"//mediapipe/graphs/iris_tracking:iris_tracking_cpu_video_input_deps",
		"//mediapipe/graphs/iris_tracking:iris_tracking_cpu_deps",
		"//mediapipe/graphs/iris_tracking:iris_depth_cpu_deps",

		"//mediapipe/calculators/core:mux_calculator",

		"//mediapipe/unreal:headers",
	],
)

cc_binary(
	name = "ump_shared",
	srcs = ["ump_dll.cc"],
	defines = ["UMP_EXPORTS"],
	linkshared = 1,
	deps = [
		":ump_core",
	],
)

cc_binary(
	name = "ump_app",
	srcs = ["ump_app.cc"],
	deps = [
		":ump_core"
	],
)
