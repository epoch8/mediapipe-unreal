#pragma once

#include "ump_object.h"

using UmpContextBase = UmpObject<IUmpContext>;

class UmpContext : public UmpContextBase
{
public:
	UmpContext();
	~UmpContext() override;

	void SetLog(class IUmpLog* log) override;
	void SetResourceDir(const char* resource_dir) override;
	class IUmpPipeline* CreatePipeline() override;
	void LogProfilerStats() override;
};
