#include "Sprite.h"
#include <cassert>
#include "Texture.h"
#include <d3d11_2.h>
#include <d2d1_2.h>
using namespace std;

Sprite::Sprite() {
}

Sprite::~Sprite() {
}

void Sprite::CreateBitmap() {
	assert(texture);
	D2D1_RECT_U rect = {0, 0, texture->w, texture->h};
	//bitmap->CopyFromMemory(texture->texture->QueryInterface()
}

void Sprite::Draw() {

}