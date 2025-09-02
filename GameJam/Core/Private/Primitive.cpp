#include "pch.h"
#include "Core/Public/Primitive.h"

// Global variable definition
UINT PrimitiveNumber = 0;

UPrimitive::UPrimitive()
{
    ++PrimitiveNumber;
    ID = PrimitiveNumber;
}

UPrimitive::~UPrimitive()
{
}

