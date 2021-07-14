#include "ump_context.h"
#include "ump_pipeline.h"

ABSL_DECLARE_FLAG(std::string, resource_root_dir);

class UmpStdoutLog : public IUmpLog
{
public:
	void Println(EUmpVerbosity verbosity, const char* msg) const override { std::cout << msg << std::endl; }
	static IUmpLog* GetInstance() { static UmpStdoutLog log; return &log; }
};

IUmpLog* _ump_log = UmpStdoutLog::GetInstance();

UmpContext::UmpContext() {
	log_d("+UmpContext");
}

UmpContext::~UmpContext() {
	log_d("~UmpContext");
}

void UmpContext::SetLog(IUmpLog* log) {
	_ump_log = log;
}

void UmpContext::SetResourceDir(const char* resource_dir) {
	log_i(strf("SetResourceDir: %s", resource_dir));
	absl::SetFlag(&FLAGS_resource_root_dir, resource_dir);
}

IUmpPipeline* UmpContext::CreatePipeline() {
	return new UmpPipeline();
}

void UmpContext::LogProfilerStats() {
	log_d(std::string(PROF_SUMMARY).c_str());
}

IUmpContext* UmpCreateContext() {
	return new UmpContext();
}
