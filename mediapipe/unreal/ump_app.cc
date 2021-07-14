#include "ump_pipeline.h"
#include "mediapipe/framework/port/opencv_video_inc.h"

#define UMP_UNIQ(_type) std::unique_ptr<_type, IUmpObject::Dtor>

int main(int argc, char* argv[])
{
	google::InitGoogleLogging(argv[0]);
	absl::ParseCommandLine(argc, argv);

	std::cout << "== INIT ==" << std::endl;

	UMP_UNIQ(IUmpContext) context(UmpCreateContext());
	context->SetResourceDir("");

	UMP_UNIQ(IUmpPipeline) pipe(context->CreatePipeline());
	pipe->SetCaptureParams(0, cv::CAP_DSHOW, 0, 0, 0); // CAP_MSMF is broken on windows!
	pipe->SetOverlay(true);

	pipe->SetGraphConfiguration("mediapipe/unreal/holistic_landmarks.pbtxt");

	std::vector<UMP_UNIQ(IUmpObserver)> observers;
	observers.emplace_back(pipe->CreateObserver("pose_landmarks"));
	observers.emplace_back(pipe->CreateObserver("face_landmarks"));
	observers.emplace_back(pipe->CreateObserver("left_hand_landmarks"));
	observers.emplace_back(pipe->CreateObserver("right_hand_landmarks"));

	pipe->Start();
	getchar();
	pipe->Stop();

	context->LogProfilerStats();

	getchar();
	return 0;
}
