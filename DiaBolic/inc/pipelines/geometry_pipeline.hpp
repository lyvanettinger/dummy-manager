#pragma once

#include "../../assets/shaders/constant_buffers.hlsli"

class Renderer;
struct Camera;

class GeometryPipeline
{
public:
	GeometryPipeline(Renderer& renderer, std::shared_ptr<Camera>& camera);
	~GeometryPipeline();

	void PopulateCommandlist(const Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>& commandList);
	void Update(float deltaTime);
private:
	Renderer& _renderer;
	std::shared_ptr<Camera> _camera;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineState{};

	// temporarily stored here
	Util::Buffer _positionBuffer{};
	Util::Buffer _normalBuffer{};
	Util::Buffer _uvBuffer{};
	Microsoft::WRL::ComPtr<ID3D12Resource> _indexBuffer{};
	D3D12_INDEX_BUFFER_VIEW _indexBufferView{};
	uint32_t _indexCount{};

	RenderResources _renderResources{};

	Util::Texture _albedoTexture{};

	void CreatePipeline();
	void InitializeAssets();
};