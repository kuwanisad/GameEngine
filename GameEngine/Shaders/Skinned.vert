// ----------------------------------------------------------------
// 스켈레탈 메시용 버텍스 셰이더. Phong.vert와 동일한 조명 파이프라인을
// 쓰되(fragTexCoord/fragNormal/fragWorldPos 이름/의미 동일, Phong.frag 그대로 재사용),
// 정점을 uWorldTransform으로 보내기 전에 매트릭스 팔레트로 먼저 스키닝한다.
// ----------------------------------------------------------------

#version 330

// 매트릭스 팔레트: 본별 (InverseBindPose * CurrentGlobal) 최종 행렬.
// USkeletalMeshComponent::Draw()에서 매 프레임 업로드.
uniform mat4 uMatrixPalette[96];

// Uniforms for world transform and view-proj (Phong.vert와 동일)
uniform mat4 uWorldTransform;
uniform mat4 uViewProj;

// VertexContainer::PosNormSkinTex 레이아웃과 정확히 대응
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in uvec4 inBoneIndices;   // GL_UNSIGNED_BYTE x4, 정수 그대로 (비정규화)
layout(location = 3) in vec4 inBoneWeights;    // GL_UNSIGNED_BYTE x4, 0~1로 정규화되어 들어옴
layout(location = 4) in vec2 inTexCoord;

// Any vertex outputs (other than position) - Phong.frag와 이름/의미 동일
out vec2 fragTexCoord;
out vec3 fragNormal;
out vec3 fragWorldPos;

void main()
{
	// 본 인덱스 4개로 팔레트에서 행렬을 조회해 가중치대로 블렌딩
	mat4 SkinMatrix =
		uMatrixPalette[inBoneIndices.x] * inBoneWeights.x +
		uMatrixPalette[inBoneIndices.y] * inBoneWeights.y +
		uMatrixPalette[inBoneIndices.z] * inBoneWeights.z +
		uMatrixPalette[inBoneIndices.w] * inBoneWeights.w;

	// 모델 공간 안에서 먼저 스키닝 적용
	vec4 pos = vec4(inPosition, 1.0);
	pos = pos * SkinMatrix;
	// 월드 공간으로
	pos = pos * uWorldTransform;
	fragWorldPos = pos.xyz;
	// 클립 공간으로
	gl_Position = pos * uViewProj;

	// 노멀도 동일하게 스키닝 + 월드 변환 (w = 0, 이동 성분 제외)
	fragNormal = (vec4(inNormal, 0.0f) * SkinMatrix * uWorldTransform).xyz;

	fragTexCoord = inTexCoord;
}
