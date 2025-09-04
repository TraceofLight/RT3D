#pragma once

class UIElement
{
protected:
	std::string name;
	FVector3 position;
	FVector3 size;
	FVector3 color;

public:
	virtual ~UIElement() = default;


	virtual void Update(float deltaTime) = 0;
	virtual void Render() = 0;

	virtual bool OnMouseClick(FVector3 pos) { return false; }

	void SetPosition(FVector3 pos) { position = pos; }
	void SetSize(FVector3 size) { this->size = size; }
	void SetColor(FVector3 col) { color = col; }

	FVector3 GetPosition() const { return position; }
	FVector3 GetSize() const { return size; }

	bool IsCursorInside(FVector3 pos)
	{
		float halfWidth = size.x / 2.0f;
		float halfHeight = size.y / 2.0f;

		float left = position.x - halfWidth;
		float right = position.x + halfWidth;
		float top = position.y - halfHeight;
		float bottom = position.y + halfHeight;

		return pos.x >= left && pos.x <= right && pos.y >= top && pos.y <= bottom;
	}
};
