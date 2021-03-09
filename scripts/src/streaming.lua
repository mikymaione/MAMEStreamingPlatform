-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   streaming.lua
--
--   Rules for building MAME streaming server
--
---------------------------------------------------------------------------

project "streaming"
	uuid "2395a0a9-c8c2-4f64-950a-822e647a75ee"
	kind (LIBTYPE)

	links {	
		ext_lib("libavcodec"),
		ext_lib("libavutil"),
	}

	addprojectflags()

	includedirs {
		MAME_DIR .. "3rdparty",
		ext_includedir("ffmpeg"),
	}

	includedirs {		
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "src/lib/util/encoding/",		
	}
	
	files {
		MAME_DIR .. "src/lib/util/streaming_server.hpp",
	}

	files {
		MAME_DIR .. "src/lib/util/encoding/encode_to_mp4.h",
	}