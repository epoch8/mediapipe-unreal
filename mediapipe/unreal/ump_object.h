#pragma once

#include "ump_shared.h"

class IExtDummy {};

template<typename TBase, typename TExt = IExtDummy>
class UmpObject : public TBase, public TExt
{
public:
	UmpObject(const UmpObject&) = delete;
	UmpObject(UmpObject&&) = delete;

	UmpObject() : ref_count(1) {}
	~UmpObject() override {}

	void Release() override
	{
		if (ref_count.fetch_sub(1) == 1) // TODO: better memory_order?
		{
			delete this;
		}
	}

	void AddRef() override
	{
		ref_count.fetch_add(1); // TODO: better memory_order?
	}

	inline void log(EUmpVerbosity verbosity, const char* msg) const { if (_ump_log) { _ump_log->Println(verbosity, msg); } }
	inline void log_e(const char* msg) const { log(EUmpVerbosity::Error, msg); }
	inline void log_w(const char* msg) const { log(EUmpVerbosity::Warning, msg); }
	inline void log_i(const char* msg) const { log(EUmpVerbosity::Info, msg); }
	inline void log_d(const char* msg) const { log(EUmpVerbosity::Debug, msg); }
	inline void log_e(const std::string& msg) const { log(EUmpVerbosity::Error, *msg); }
	inline void log_w(const std::string& msg) const { log(EUmpVerbosity::Warning, *msg); }
	inline void log_i(const std::string& msg) const { log(EUmpVerbosity::Info, *msg); }
	inline void log_d(const std::string& msg) const { log(EUmpVerbosity::Debug, *msg); }

protected:
	std::atomic<int> ref_count;
};
