#include "pch.h"
#include "GameJam/Core/Public/Primitive.h"

UPrimitive::UPrimitive()
{
    ++PrimitiveNumber;
    ID = PrimitiveNumber;
}

UPrimitive::~UPrimitive()
{
}

