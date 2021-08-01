#include "ump_pipeline.h"
#include "ump_observer.h"

#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"

#include "mediapipe/framework/port/opencv_imgproc_inc.h"
#include "mediapipe/framework/port/opencv_video_inc.h"
#include "mediapipe/framework/port/opencv_highgui_inc.h"

#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/file_helpers.h"
#include "mediapipe/util/resource_util.h"

UmpPipeline::UmpPipeline()
{
	log_d("+UmpPipeline");
}

UmpPipeline::~UmpPipeline()
{
	log_d("~UmpPipeline");
	Stop();
}

void UmpPipeline::SetGraphConfiguration(const char* filename)
{
	log_i(strf("SetGraphConfiguration: %s", filename));
	config_filename = filename;
}

void UmpPipeline::SetCaptureFromFile(const char* filename)
{
	log_i(strf("SetCaptureFromFile: %s", filename));
	input_filename = filename;
}

void UmpPipeline::SetCaptureParams(int in_cam_id, int in_cam_api, int in_cam_resx, int in_cam_resy, int in_cam_fps)
{
	log_i(strf("SetCaptureParams: cam=%d api=%d w=%d h=%d fps=%d", in_cam_id, in_cam_api, in_cam_resx, in_cam_resy, in_cam_fps));
	cam_id = in_cam_id;
	cam_api = in_cam_api;
	cam_resx = in_cam_resx;
	cam_resy = in_cam_resy;
	cam_fps = in_cam_fps;
}

void UmpPipeline::SetOverlay(bool in_overlay)
{
	log_i(strf("SetOverlay: %d", (in_overlay ? 1 : 0)));
	overlay = in_overlay;
}

IUmpObserver* UmpPipeline::CreateObserver(const char* stream_name)
{
	log_i(strf("CreateObserver: %s", stream_name));
	if (run_flag)
	{
		log_e("Invalid state: pipeline running");
		return nullptr;
	}
	auto* observer = new UmpObserver(stream_name);
	observer->AddRef();
	observers.emplace_back(observer);
	return observer;
}

bool UmpPipeline::Start()
{
	Stop();
	try
	{
		log_i("UmpPipeline::Start");
		this->run_flag = true;
		worker = std::make_unique<std::thread>([this]() { this->WorkerThread(); });
		log_i("UmpPipeline::Start OK");
		return true;
	}
	catch (const std::exception& ex)
	{
		log_e(ex.what());
	}
	return false;
}

void UmpPipeline::Stop()
{
	try
	{
		run_flag = false;
		if (worker)
		{
			log_i("UmpPipeline::Stop");
			worker->join();
			worker.reset();
			log_i("UmpPipeline::Stop OK");
		}
	}
	catch (const std::exception& ex)
	{
		log_e(ex.what());
	}
}

void UmpPipeline::WorkerThread()
{
	log_i("Enter WorkerThread");
	// RUN
	try
	{
		auto status = this->RunImpl();
		if (!status.ok())
		{
			std::string msg(status.message());
			log_e(msg);
		}
	}
	catch (const std::exception& ex)
	{
		log_e(ex.what());
	}
	// SHUTDOWN
	try
	{
		ShutdownImpl();
	}
	catch (const std::exception& ex)
	{
		log_e(ex.what());
	}
	log_i("Leave WorkerThread");
}

void UmpPipeline::ShutdownImpl()
{
	log_i("UmpPipeline::Shutdown");

	graph.reset();
	observers.clear();

	if (overlay)
		cv::destroyAllWindows();

	log_i("UmpPipeline::Shutdown OK");
}

absl::Status UmpPipeline::RunImpl()
{
	constexpr char kInputStream[] = "input_video";
	constexpr char kOutputStream[] = "output_video";
	constexpr char kWindowName[] = "MediaPipe";

	log_i("UmpPipeline::Run");

	// init mediapipe

	std::string config_str;
	RET_CHECK_OK(LoadGraphConfig(config_filename, config_str));

	log_i("ParseTextProto");
	mediapipe::CalculatorGraphConfig config;
	RET_CHECK(mediapipe::ParseTextProto<mediapipe::CalculatorGraphConfig>(config_str, &config));

	log_i("CalculatorGraph::Initialize");
	graph.reset(new mediapipe::CalculatorGraph());
	RET_CHECK_OK(graph->Initialize(config));

	for (auto& iter : observers)
	{
		RET_CHECK_OK(iter->ObserveOutputStream(graph.get()));
	}

	std::unique_ptr<mediapipe::OutputStreamPoller> output_poller;
	if (overlay)
	{
		//ASSIGN_OR_RETURN(mediapipe::OutputStreamPoller poller, graph->AddOutputStreamPoller(kOutputStream));
		auto output_poller_sop = graph->AddOutputStreamPoller(kOutputStream);
		RET_CHECK(output_poller_sop.ok());
		output_poller = std::make_unique<mediapipe::OutputStreamPoller>(std::move(output_poller_sop.value()));
	}

	// init opencv

	log_i("VideoCapture::open");
	cv::VideoCapture capture;
	use_camera = input_filename.empty();

	if (use_camera)
	{
		#if defined(_WIN32)
		if (cam_api == cv::CAP_ANY)
		{
			// CAP_MSMF is broken on windows! use CAP_DSHOW by default, also see: https://github.com/opencv/opencv/issues/17687
			cam_api = cv::CAP_DSHOW;
		}
		#endif

		capture.open(cam_id, cam_api);
	}
	else
	{
		capture.open(*input_filename);
	}

	RET_CHECK(capture.isOpened());

	if (use_camera)
	{
		if (cam_resx > 0 && cam_resy > 0)
		{
			capture.set(cv::CAP_PROP_FRAME_WIDTH, cam_resx);
			capture.set(cv::CAP_PROP_FRAME_HEIGHT, cam_resy);
		}

		if (cam_fps > 0)
			capture.set(cv::CAP_PROP_FPS, cam_fps);
	}

	if (overlay)
		cv::namedWindow(kWindowName, cv::WINDOW_AUTOSIZE);

	// start

	cv::Mat camera_frame_raw;
	cv::Mat camera_frame;

	log_i("CalculatorGraph::StartRun");
	RET_CHECK_OK(graph->StartRun({}));

	log_i("MAIN LOOP");
	while (run_flag)
	{
		PROF_NAMED("pipeline_tick");

		{
			PROF_NAMED("capture_frame");
			capture >> camera_frame_raw;
			if (camera_frame_raw.empty())
				continue;
		}

		{
			PROF_NAMED("process_frame");

			const size_t frame_timestamp_us = (double)cv::getTickCount() / (double)cv::getTickFrequency() * 1e6;

			cv::cvtColor(camera_frame_raw, camera_frame, cv::COLOR_BGR2RGB);
			if (use_camera)
				cv::flip(camera_frame, camera_frame, 1);

			auto input_frame = absl::make_unique<mediapipe::ImageFrame>(
				mediapipe::ImageFormat::SRGB, camera_frame.cols, camera_frame.rows,
				mediapipe::ImageFrame::kDefaultAlignmentBoundary);

			cv::Mat input_frame_mat = mediapipe::formats::MatView(input_frame.get());
			camera_frame.copyTo(input_frame_mat);

			RET_CHECK_OK(graph->AddPacketToInputStream(
				kInputStream,
				mediapipe::Adopt(input_frame.release())
				.At(mediapipe::Timestamp(frame_timestamp_us))));
		}

		if (overlay && output_poller)
		{
			mediapipe::Packet packet;
			if (!output_poller->Next(&packet))
			{
				log_e("OutputStreamPoller::Next failed");
				break;
			}

			{
				PROF_NAMED("draw_overlay");
				auto& output_frame = packet.Get<mediapipe::ImageFrame>();
				cv::Mat output_frame_mat = mediapipe::formats::MatView(&output_frame);
				cv::cvtColor(output_frame_mat, output_frame_mat, cv::COLOR_RGB2BGR);
				cv::imshow(kWindowName, output_frame_mat);
			}

			cv::waitKey(1); // required for cv::imshow
		}
	}

	log_i("CalculatorGraph::CloseInputStream");
	graph->CloseInputStream(kInputStream);
	graph->WaitUntilDone();

	return absl::OkStatus();
}

// allows multiple files separated by ';'
absl::Status UmpPipeline::LoadGraphConfig(const std::string& filename, std::string& out_str)
{
	log_i(strf("LoadGraphConfig: %s", filename.c_str()));

	out_str.clear();
	out_str.reserve(4096);

	std::string sub_str;
	sub_str.reserve(1024);

	std::stringstream filename_ss(filename);
	std::string sub_name;

	while(std::getline(filename_ss, sub_name, ';'))
	{
		sub_str.clear();
		RET_CHECK_OK(LoadResourceFile(sub_name, sub_str));
		out_str.append(sub_str);
	}

	return absl::OkStatus();
}

absl::Status UmpPipeline::LoadResourceFile(const std::string& filename, std::string& out_str)
{
	log_i(strf("LoadResourceFile: %s", filename.c_str()));

	out_str.clear();

	std::string path;
	ASSIGN_OR_RETURN(path, mediapipe::PathToResourceAsFile(filename));

	RET_CHECK_OK(mediapipe::file::GetContents(path, &out_str));

	return absl::OkStatus();
}
