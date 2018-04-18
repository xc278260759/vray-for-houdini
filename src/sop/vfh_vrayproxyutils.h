//
// Copyright (c) 2015-2018, Chaos Software Ltd
//
// V-Ray For Houdini
//
// ACCESSIBLE SOURCE CODE WITHOUT DISTRIBUTION OF MODIFICATION LICENSE
//
// Full license text: https://github.com/ChaosGroup/vray-for-houdini/blob/master/LICENSE
//

#ifndef VRAY_FOR_HOUDINI_VRAYPROXYCACHE_H
#define VRAY_FOR_HOUDINI_VRAYPROXYCACHE_H

#include "vfh_hashes.h"
#include "vfh_defines.h"
#include "vfh_vray.h"

#include <GU/GU_DetailHandle.h>
#include <UT/UT_BoundingBox.h>
#include <UT/UT_String.h>

namespace VRayForHoudini {

/// Level of detail types for viewport display of .vrmesh geometry
/// Proxy cache manager will load and cache only the data needed
/// to display the corresponding level of detail
enum class VRayProxyPreviewType {
	bbox = 0, ///< Load and display bbox of the geometry
	preview, ///< Load and display geometry in the preview voxel
	full, ///< Load and display full geometry
};

/// Proxy cache key.
struct VRayProxyRefKey {
	VUtils::CharString filePath;
	VRayProxyPreviewType lod = VRayProxyPreviewType::preview;
	fpreal f = 0;
	int animType = 0;
	fpreal64 animOffset = 0;
	fpreal64 animSpeed = 1.0;
	bool animOverride = false;
	int animStart = 0;
	int animLength = 100;
	int previewFaces = 10000;

	bool operator ==(const VRayProxyRefKey &other) const {
		return filePath == other.filePath &&
		       lod == other.lod &&
		       MemberFloatEq(f) &&
		       animType == other.animType &&
		       MemberFloatEq(animOffset) &&
		       MemberFloatEq(animSpeed) &&
		       animOverride == other.animOverride &&
		       animStart == other.animStart &&
		       animLength == other.animLength &&
		       previewFaces == other.previewFaces;
	}

	bool operator !=(const VRayProxyRefKey &other) const {
		return !(*this == other);
	}

	bool operator <(const VRayProxyRefKey &other) const {
		return *this != other;
	}

	Hash::MHash getHash() const {
#pragma pack(push, 1)
		struct SettingsKey {
			int lod;
			int frame;
			int animType;
			int animOffset;
			int animSpeed;
			int animOverride;
			int animLength;
			int numPreviewFaces;
		} settingsKey = {
			static_cast<int>(lod),
			VUtils::fast_floor(f * 100.0),
			animType,
			VUtils::fast_floor(animOffset * 100.0),
			VUtils::fast_floor(animSpeed * 100.0),
			animOverride,
			animLength,
			previewFaces
		};
#pragma pack(pop)

		Hash::MHash data;
		Hash::MurmurHash3_x86_32(&settingsKey, sizeof(SettingsKey), 42, &data);

		return data;
	}

	/// Checks for difference between two instances,
	/// without taking frame into account
	bool differingSettings(const VRayProxyRefKey &other) const {
		return !(filePath == other.filePath &&
			lod == other.lod &&
			animType == other.animType &&
			MemberFloatEq(animOffset) &&
			MemberFloatEq(animSpeed) &&
			animOverride == other.animOverride &&
			animStart == other.animStart &&
			animLength == other.animLength &&
			previewFaces == other.previewFaces);
	}
};

/// Get detail handle for a proxy packed primitive.
/// @note internally this will go through the proxy cache manager
/// @param[in] options - UT_Options for proxy packed primitive
/// @retval the detail handle for the primitive (based on file,
///         frame, LOD in options)
GU_DetailHandle getVRayProxyDetail(const VRayProxyRefKey &options);

/// Get the bounding box for a proxy packed primitive.
/// @note internally this will go through the proxy cache manager
///       bbox is cached on first read access for detail handle
/// @param[in] options - UT_Options for proxy packed primitive
/// @param[out] box - the bounding box for all geometry
/// @retval true if successful i.e this data is found cached
bool getVRayProxyBoundingBox(const VRayProxyRefKey &options, UT_BoundingBox &box);

/// Clear cached data for a given file.
/// @param filepath File path.
/// @retval true if cached file is deleted.
bool clearVRayProxyCache(const char *filepath);
} // namespace VRayForHoudini

#endif // VRAY_FOR_HOUDINI_VRAYPROXYCACHE_H
