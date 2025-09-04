#include "pch.h"
#include "Core/Public/Primitive.h"

static UINT PrimitiveNumber = 0;

UPrimitive::UPrimitive()
{
    ++PrimitiveNumber;
    ID = PrimitiveNumber;
}

UPrimitive::~UPrimitive()
{
}
