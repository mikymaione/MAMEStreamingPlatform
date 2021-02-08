#pragma once

#ifndef SRC_STREAMINGSERVER_H
#define SRC_STREAMINGSERVER_H

#include "server_ws_impl.hpp"

#include <functional>

namespace webpp
{
	class StreamingServer : public webpp::ws_server
	{
	public:
		StreamingServer(std::function<void(StreamingServer *)> callBack)
			: callBack(callBack)
		{
		}

	protected:
		void accept()
		{
			callBack(this);
		}

	private:
		std::function<void(StreamingServer *)> callBack;
	};
} // namespace webpp

#endif //SRC_STREAMINGSERVER_H
