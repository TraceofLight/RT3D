#include "pch.h"
#include "LineComponent.h"

void ULineComponent::GetWorldLineData(TArray<FVector>& OutStartPoints, TArray<FVector>& OutEndPoints, TArray<FVector4>& OutColors) const
{
    if (!bLinesVisible || Lines.empty())
    {
        OutStartPoints.clear();
        OutEndPoints.clear();
        OutColors.clear();
        return;
    }
    
    FMatrix worldMatrix = GetWorldMatrix();
    size_t lineCount = Lines.size();
    
    for (const ULine* Line : Lines)
    {
        if (Line)
        {
            FVector worldStart, worldEnd;
            Line->GetWorldPoints(worldMatrix, worldStart, worldEnd);
            
            OutStartPoints.push_back(worldStart);
            OutEndPoints.push_back(worldEnd);
            OutColors.push_back(Line->GetColor());
        }
    }
    return;
}

ULineComponent::ULineComponent()
{
    bLinesVisible = true;
}

ULineComponent::~ULineComponent()
{
    ClearLines();
}

ULine* ULineComponent::AddLine(const FVector& StartPoint, const FVector& EndPoint, const FVector4& Color)
{
    ULine* NewLine = NewObject<ULine>();
    NewLine->SetLine(StartPoint, EndPoint);
    NewLine->SetColor(Color);
    
    Lines.push_back(NewLine);
    
    return NewLine;
}

void ULineComponent::RemoveLine(ULine* Line)
{
    if (!Line) return;
    
    auto it = std::find(Lines.begin(), Lines.end(), Line);
    if (it != Lines.end())
    {
        DeleteObject(*it);
        Lines.erase(it);
    }
}

void ULineComponent::ClearLines()
{
    for (ULine* Line : Lines)
    {
        if (Line)
        {
            DeleteObject(Line);
        }
    }
    Lines.Empty();
}

void ULineComponent::Render(URHIDevice* RHI, const FMatrix& ViewMatrix, const FMatrix& ProjectionMatrix)
{
    if (!HasVisibleLines() || !RHI)
        return;

    // TODO: RHI를 통한 라인 렌더링은 DebugPass에서 처리
    // 현재는 빈 구현으로 두고, DebugPass에서 GetWorldLineData()를 호출하여 처리
    // 이 함수는 향후 제거되거나 DebugPass로 통합될 예정
}

