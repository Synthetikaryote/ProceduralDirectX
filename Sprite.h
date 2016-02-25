#pragma once
class Texture;
class Transform;
class ID2D1Bitmap;
class Sprite {
public:
	Texture* texture;
	Transform* transform;
	ID2D1Bitmap* bitmap;

	void CreateBitmap();
	void Draw();

	Sprite();
	~Sprite();
};

