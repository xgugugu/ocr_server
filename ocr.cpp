
#include <iostream>
#include <cstdio>
#include <thread>
#include "httplib.h"
#include <windows.h>

using namespace std;
using namespace httplib;

static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
static inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}
std::string base64_decode(const std::string& encoded_string) {
	size_t in_len = encoded_string.size();
	int i = 0, j = 0, in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;
	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_];
		in_++;
		if (i == 4) {
			for (i = 0; i < 4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]) & 0xff;
			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}
	if (i) {
		for (j = 0; j < i; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]) & 0xff;
		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}
	return ret;
}

typedef void (* __stdcall VOID_FUNC)();
typedef char* (* __stdcall OCR_FUNC)(const char*, int, int*);

HMODULE hModule = LoadLibrary("c_ocr.dll");
VOID_FUNC init = (VOID_FUNC)GetProcAddress(hModule, "init");
OCR_FUNC ocr = (OCR_FUNC)GetProcAddress(hModule, "ocr");
VOID_FUNC cleanup = (VOID_FUNC)GetProcAddress(hModule, "cleanup");

void tc() {
	Sleep(1000);
	cleanup();
	exit(0);
}

int main() {
	init();
	Server svr;
	svr.Options("/captcha", [&](const Request & req, Response & res) {
		res.set_header("Access-Control-Allow-Origin", "*");
	});
	svr.Post("/captcha", [&](const Request & req, Response & res) {
		string buffer = base64_decode(req.body);
		int rsize = 0;
		char* ret = ocr(buffer.c_str(), buffer.size(), &rsize);
		char txt[rsize + 1] = {};
		memcpy(txt, ret, rsize);
		free(ret);
		res.set_header("Access-Control-Allow-Origin", "*");
		res.set_content(txt, "text/plain");
		cout << "成功处理了一次请求，结果为：\"" << txt << "\"\n";
	});
	svr.Get("/exit", [&](const Request & req, Response & res) {
		res.set_header("Access-Control-Allow-Origin", "*");
		res.set_content("ok", "text/plain");
		thread th(tc);
		th.detach();
	});
	svr.listen("127.0.0.1", 11451);
	return 0;
}
