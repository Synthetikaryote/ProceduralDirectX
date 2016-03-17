#include "Uber.h"
#include <string>
#include <vector>
#include <fstream>
#include "Shlwapi.h"
#include "HeightTree.h"
using namespace std;

void ThrowIfFailed(HRESULT hr) {
	if (FAILED(hr))
		throw _com_error(hr);
}

chrono::high_resolution_clock::time_point timeStart = chrono::high_resolution_clock::now();
float time() {
	auto sinceStart = chrono::high_resolution_clock::now() - timeStart;
	return ((float)sinceStart.count()) * chrono::high_resolution_clock::period::num / chrono::high_resolution_clock::period::den;
}

vector<uint8_t> Read(string path) {
	vector<uint8_t> data;
	fstream file(path, ios::in | ios::ate | ios::binary);
	
	vector<wchar_t> pathBuf;
	DWORD copied = 0;
	do {
		pathBuf.resize(pathBuf.size() + MAX_PATH);
		copied = GetModuleFileName(0, &pathBuf.at(0), pathBuf.size());
	} while (copied >= pathBuf.size());
	pathBuf.resize(copied);
	PathRemoveFileSpec(&pathBuf.at(0));
	wstring exePath(pathBuf.begin(), pathBuf.end());
	exePath += wstring(L"\\");

	if (file.is_open()) {
		data.resize(static_cast<size_t>(file.tellg()));
		file.seekg(0, ios::beg);
		file.read(reinterpret_cast<char*>(&data[0]), data.size());
		file.close();
	}
	return data;
};

bool IsKeyDown(unsigned char vk) {
	return (Uber::I().keyboardState[vk] & 0x80) ? true : false;
}

float GetHeight(float yaw, float pitch) {
	while (yaw < 0.0f) yaw += TWOPI;
	while (yaw >= TWOPI) yaw -= TWOPI;
	if (pitch < 0.0f) pitch = 0.0f;
	if (pitch > PI) pitch = PI;
	unsigned x = static_cast<unsigned>(yaw / TWOPI * heightSMax) % heightSMax;
	unsigned y = static_cast<unsigned>(pitch / PI * heightSMax) % heightSMax;
	return Uber::I().heights->GetHeight(x, y, Uber::I().zoomStep);
}

void SetHeight(float yaw, float pitch, float value) {
	while (yaw < 0.0f) yaw += TWOPI;
	while (yaw >= TWOPI) yaw -= TWOPI;
	if (pitch < 0.0f) pitch = 0.0f;
	if (pitch > PI) pitch = PI;
	unsigned x = static_cast<unsigned>(yaw / TWOPI * heightSMax) % heightSMax;
	unsigned y = static_cast<unsigned>(pitch / PI * heightSMax) % heightSMax;
	return Uber::I().heights->SetHeight(x, y, Uber::I().zoomStep, value);
}