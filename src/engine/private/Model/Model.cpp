#include <cassert>
#include <fstream>
#include <sstream>

#include <engine/public/Model/Model.h>
#include <engine/public/Model/ModelCommon.h>
#include <engine/public/renderer/ConstantBuffer.h>
#include <engine/public/renderer/VertexBuffer.h>

#include "engine/public/renderer/D3D12.h"

#ifdef _DEBUG
#include <imgui.h>
#endif

void Model::Init(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& fileName) {
	this->mModelCommon = modelCommon;

	mModelData = LoadObjFile(directoryPath, fileName);

	// 頂点バッファ modelData_を突っ込む
	mVertexBuffer = std::make_unique<VertexBuffer<Vertex>>(
		mModelCommon->GetD3D12()->GetDevice(),
		sizeof(Vertex) * mModelData.vertices.size(),
		mModelData.vertices.data()
	);

	// マテリアル定数バッファ
	mMaterialConstantBuffer = std::make_unique<ConstantBuffer>(mModelCommon->GetD3D12()->GetDevice(), sizeof(Material), "ModelMaterial");
	mMaterialData = mMaterialConstantBuffer->GetPtr<Material>();
	mMaterialData->color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 白
	mMaterialData->enableLighting = true;
	mMaterialData->uvTransform = Mat4::identity;
	mMaterialData->shininess = 128.0f;
	mMaterialData->specularColor = Vec3(0.25f, 0.25f, 0.25f); // 灰色

	//// テクスチャのファイルパスが空ではなかったら
	//if (!modelData_.material.textureFilePath.empty()) {
	//	// .objの参照しているテクスチャファイル読み込み
	//	TextureManager::GetInstance()->LoadTexture(modelData_.material.textureFilePath);
	//	// 読み込んだテクスチャの番号を取得
	//	modelData_.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(
	//		modelData_.material.textureFilePath
	//	);
	//}
}

void Model::ImGuiDraw() const {
#ifdef _DEBUG
	if (ImGui::Begin("Material Editor")) {
		ImGui::ColorEdit4("Color", &mMaterialData->color.x); // 色の編集
		ImGui::DragInt("Enable Lighting", &mMaterialData->enableLighting, 1, 0, 1); // ライティングの有効化
		ImGui::SliderFloat("Shininess", &mMaterialData->shininess, 0.0f, 128.0f); // シャイニネスの編集
		ImGui::ColorEdit3("Specular Color", &mMaterialData->specularColor.x); // スペキュラカラーの編集
	}
	ImGui::End();
#endif
}

void Model::Draw() const {
	// VertexBufferViewを設定
	D3D12_VERTEX_BUFFER_VIEW vbView = mVertexBuffer->View();
	mModelCommon->GetD3D12()->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);

	// マテリアルの定数バッファの設定
	mModelCommon->GetD3D12()->GetCommandList()->SetGraphicsRootConstantBufferView(
		0, mMaterialConstantBuffer->GetAddress()
	);

	//// SRVのDescriptorTableの先頭を設定
	//modelCommon_->GetD3D12()->GetCommandList()->SetGraphicsRootDescriptorTable(
	//	2, TextureManager::GetInstance()->GetSrvHandleGPU(modelData_.material.textureFilePath)
	//);

	// 描画
	mModelCommon->GetD3D12()->GetCommandList()->DrawInstanced(static_cast<UINT>(mModelData.vertices.size()), 1, 0, 0);
}

MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	MaterialData materialData; // 構築するMaterialData
	std::string line; // ファイルから読んだ1行を格納するもの
	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open()); // とりあえず開けなかったら止める

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 連結してファイルパスにする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}

	return materialData;
}

Model::ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData; // 構築するModelData
	std::vector<Vec4> positions; // 位置
	std::vector<Vec3> normals; // 法線
	std::vector<Vec2> texcoords; // テクスチャの座標
	std::string line;

	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier; // 先頭の識別子を読む

		if (identifier == "v") {
			Vec4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f;
			positions.emplace_back(position);
		} else if (identifier == "vt") {
			Vec2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.y = 1.0f - texcoord.y;
			texcoords.emplace_back(texcoord);
		} else if (identifier == "vn") {
			Vec3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normals.emplace_back(normal);
		} else if (identifier == "f") {
			Vertex triangle[3];

			// 面は三角形限定。その他は未対応
			for (auto& faceVertex : triangle) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				// 頂点の要素へのIndexは[位置/UV/法線] で格納されているので、分解してIndexを取得する
				std::istringstream v(vertexDefinition);
				std::string index;
				uint32_t elementIndices[3] = { 0, 0, 0 };
				int element = 0;

				while (std::getline(v, index, '/') && element < 3) {
					if (!index.empty()) {
						elementIndices[element] = std::stoi(index);
					}
					++element;
				}

				// 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vec4 position = positions[elementIndices[0] - 1];
				Vec2 texcoord = elementIndices[1] ? texcoords[elementIndices[1] - 1] : Vec2{ 0.0f, 0.0f };
				Vec3 normal = elementIndices[2] ? normals[elementIndices[2] - 1] : Vec3{ 0.0f, 0.0f, 0.0f };

				position.x *= -1.0f;
				normal.x *= -1.0f;
				faceVertex = { position, texcoord, normal };
			}
			// 頂点を逆順で登録することで、周り順を逆にする
			modelData.vertices.emplace_back(triangle[2]);
			modelData.vertices.emplace_back(triangle[1]);
			modelData.vertices.emplace_back(triangle[0]);
		} else if (identifier == "mtllib") {
			// materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}

	return modelData;
}
