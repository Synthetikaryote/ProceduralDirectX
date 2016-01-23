#include "Uber.h"
#include <string>
#include <vector>
#include <fstream>
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
	if (file.is_open()) {
		data.resize(static_cast<size_t>(file.tellg()));
		file.seekg(0, ios::beg);
		file.read(reinterpret_cast<char*>(&data[0]), data.size());
		file.close();
	}
	return data;
};

bool IsKeyDown(unsigned char vk) {
	return static_cast<bool>(Uber::I().keyboardState[vk] & 0x80);
}