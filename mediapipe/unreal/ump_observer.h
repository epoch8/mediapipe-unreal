#pragma once

#include "ump_object.h"

using UmpObserverBase = UmpObject<IUmpObserver>;

class UmpObserver : public UmpObserverBase
{
public:
	UmpObserver(const char* in_stream_name) : stream_name(in_stream_name) { log_d(strf("+UmpObserver %s", *stream_name)); }
	~UmpObserver() override { log_d(strf("~UmpObserver %s", *stream_name)); }

	absl::Status ObserveOutputStream(mediapipe::CalculatorGraph* graph)
	{
		log_i(strf("ObserveOutputStream: %s", *stream_name));

		std::string presence_name(stream_name);
		presence_name.append("_presence");

		graph->ObserveOutputStream(*presence_name, [this](const mediapipe::Packet& pk)
		{
			presence = pk.Get<bool>();

			if (callback)
				callback->OnUmpPresence(this, presence);

			return absl::OkStatus();
		});

		RET_CHECK_OK(graph->ObserveOutputStream(*stream_name, [this](const mediapipe::Packet& pk)
		{
			if (callback)
			{
				PROF_NAMED("observer_callback");

				raw_data = pk.GetRaw(); // requires patched mediapipe\framework\packet.h
				message_type = pk.GetTypeId();

				callback->OnUmpPacket(this);
			}

			return absl::OkStatus();
		}));

		return absl::OkStatus();
	}

	void SetPacketCallback(IUmpPacketCallback* in_callback) override { callback = in_callback; }
	virtual size_t GetMessageType() override { return message_type; }
	virtual const void* const GetData() override { return raw_data; }

protected:
	std::string stream_name;
	IUmpPacketCallback* callback = nullptr;
	const void* raw_data = nullptr;
	size_t message_type = 0;
	bool presence = false;
};
