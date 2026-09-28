#pragma once

namespace RE
{
	class BSNavmesh;
	class NiPoint3;

	class FindTriangleForLocationFilter
	{
	public:
		virtual ~FindTriangleForLocationFilter();  // 00

		// add
		virtual bool IsValidTri(NiPoint3* a_location, BSNavmesh* a_navMesh, std::uint16_t a_tri, NiPoint3* a_pointOnTri) = 0;
	};
	static_assert(sizeof(FindTriangleForLocationFilter) == 0x8);

	class FindTriangleForLocationFilterCheckDeltaZ : public FindTriangleForLocationFilter
	{
	public:
		~FindTriangleForLocationFilterCheckDeltaZ() override;  // 00

		// add
		bool IsValidTri(NiPoint3* a_location, BSNavmesh* a_navMesh, std::uint16_t a_tri, NiPoint3* a_pointOnTri) override;

		// members
		float maxDistAbove;  // 08
		float maxDistBelow;  // 0C
	};
	static_assert(sizeof(FindTriangleForLocationFilterCheckDeltaZ) == 0x10);
}