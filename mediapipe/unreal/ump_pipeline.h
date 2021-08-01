#pragma once

#include "ump_object.h"

using UmpPipelineBase = UmpObject<IUmpPipeline>;

class UmpPipeline : public UmpPipelineBase
{
public:
	UmpPipeline();
	~UmpPipeline() override;

	void SetGraphConfiguration(const char* filename) override;
	void SetCaptureFromFile(const char* filename) override;
	void SetCaptureParams(int cam_id, int cam_api, int cam_resx, int cam_resy, int cam_fps) override;
	void SetOverlay(bool overlay) override;
	IUmpObserver* CreateObserver(const char* stream_name) override;
	bool Start() override;
	void Stop() override;

private:
	void WorkerThread();
	void ShutdownImpl();
	absl::Status RunImpl();
	absl::Status LoadGraphConfig(const std::string& filename, std::string& out_str);
	absl::Status LoadResourceFile(const std::string& filename, std::string& out_str);

private:
	std::string resource_dir;
	std::string config_filename;
	std::string input_filename;
	int cam_id = 0;
	int cam_api = 0;
	int cam_resx = 0;
	int cam_resy = 0;
	int cam_fps = 0;
	bool use_camera = false;
	bool overlay = false;

	using ObserverPtr = std::unique_ptr<class UmpObserver, IUmpObject::Dtor>;
	std::list<ObserverPtr> observers;

	std::shared_ptr<mediapipe::CalculatorGraph> graph;

	std::unique_ptr<std::thread> worker;
	std::atomic<bool> run_flag;
};
