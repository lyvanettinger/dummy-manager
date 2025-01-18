#include "pipelines/ui_pipeline.hpp"

UIPipeline::UIPipeline(Renderer& renderer) :
	_renderer(renderer)
{
	CreatePipeline();
}

UIPipeline::~UIPipeline()
{

}

void UIPipeline::PopulateCommandlist(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>& commandList)
{

}

void UIPipeline::Update(float deltaTime)
{

}

void UIPipeline::CreatePipeline()
{

}
