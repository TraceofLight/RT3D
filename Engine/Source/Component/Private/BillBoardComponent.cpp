#include "pch.h"
#include "Component/Public/BillBoardComponent.h"

/**
 * @brief Level에서 각 Actor마다 가지고 있는 UUID를 출력해주기 위한 빌보드 클래스
 * Actor has a UBillBoardComponent
 */
UBillBoardComponent::UBillBoardComponent()
{
	Type = EPrimitiveType::BillBoard;
}
