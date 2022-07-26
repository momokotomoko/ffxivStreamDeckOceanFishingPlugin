//==============================================================================
/**
@file       ImageUtils.h
@brief      utilities for image processing
@copyright  (c) 2020, Momoko Tomoko
**/
//==============================================================================

#pragma once
#include <string>

namespace imageutils
{
	static const std::string BASE64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	/**
		@brief converts loaded png into base64. Function obtained from lodepng example documentation

		@param[out] out the output base 64
		@param[in] in the input image
	**/
	template<typename T, typename U> //T and U can be std::string or std::vector<unsigned char>
	void pngToBase64(T& out, const U& in) {
		for (size_t i = 0; i < in.size(); i += 3) {
			int v = 65536 * in[i];
			if (i + 1 < in.size()) v += 256 * in[i + 1];
			if (i + 2 < in.size()) v += in[i + 2];
			out.push_back(BASE64[(v >> 18) & 0x3f]);
			out.push_back(BASE64[(v >> 12) & 0x3f]);
			if (i + 1 < in.size()) out.push_back(BASE64[(v >> 6) & 0x3f]);
			else out.push_back('=');
			if (i + 2 < in.size()) out.push_back(BASE64[(v >> 0) & 0x3f]);
			else out.push_back('=');
		}
	}
}