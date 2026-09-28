#pragma once

#include "RE/N/NiPlane.h"
#include "RE/N/NiRefObject.h"
#include "RE/N/NiSmartPointer.h"

namespace RE
{
	class BSCullingProcess;
	class BSCubeMapCamera;
	class BSShaderAccumulator;
	class BSWaterShaderMaterial;

	class TESWaterReflections : public NiRefObject
	{
	public:
		inline static constexpr auto RTTI = RTTI_TESWaterReflections;
		inline static constexpr auto VTABLE = VTABLE_TESWaterReflections;

		struct CubeMapSide
		{
			std::uint32_t idx;         // 00
			float         renderTime;  // 04
		};
		static_assert(sizeof(CubeMapSide) == 0x8);

		virtual ~TESWaterReflections();  // 00

		// members

		std::uint16_t                  flags;                  // 10
		std::uint16_t                  pad12;                  // 12
		NiPlane                        reflectPlane;           // 14
		std::uint32_t                  pad24;                  // 24
		NiPointer<BSCubeMapCamera>     cubeMapCamera;          // 28
		NiPointer<BSShaderAccumulator> sorter;                 // 30
		BSCullingProcess*              cullingProcess;         // 38
		BSWaterShaderMaterial*         waterMaterial;          // 40
		float                          reflectionUpdateTimer;  // 48
		std::uint32_t                  currentCubeMapFace;     // 4C
		CubeMapSide                    cubeMapSides[6];        // 50
		bool                           activeThisFrame;        // 80
		std::uint8_t                   pad81;                  // 81
		std::uint16_t                  pad82;                  // 82
		std::uint32_t                  pad84;                  // 84
	};
	static_assert(sizeof(TESWaterReflections) == 0x88);
}
