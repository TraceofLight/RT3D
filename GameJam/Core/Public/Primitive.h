#pragma once

// static UINT PrimitiveNumber;

class UPrimitive
{
private:
    UINT ID;

public:
	virtual void Init() {}
	virtual void Destroy() {}

	// Special Member Function
    UPrimitive();
    virtual ~UPrimitive();
};
