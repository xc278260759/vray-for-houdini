//
// Copyright (c) 2015-2017, Chaos Software Ltd
//
// V-Ray For Houdini
//
// ACCESSIBLE SOURCE CODE WITHOUT DISTRIBUTION OF MODIFICATION LICENSE
//
// Full license text: https://github.com/ChaosGroup/vray-for-houdini/blob/master/LICENSE
//

#include "vfh_vrayproxyutils.h"
#include "vfh_vray.h"
#include "vfh_log.h"
#include "vfh_hashes.h"

#include <GU/GU_Detail.h>
#include <GU/GU_DetailHandle.h>
#include <GU/GU_PrimPoly.h>

using namespace VRayForHoudini;

enum DataError {
	DE_INVALID_GEOM = 1,
	DE_NO_GEOM,
	DE_INVALID_FILE
};

struct MeshVoxelRAII
{
	MeshVoxelRAII(VUtils::MeshInterface *mi, int index)
		: mi(mi)
	{
		vassert(mi && "MeshInterface is NULL");

		if (index < 0 || index >= mi->getNumVoxels()) {
			vassert(false && "Invalid voxel index");
		}

		mv = mi->getVoxel(index);

		vassert(mv && "MeshVoxel is NULL");
	}

	~MeshVoxelRAII() {
		mi->releaseVoxel(mv);
	}

	bool isValid() const { return mv; }

	VUtils::MeshVoxel &getVoxel() const { return *mv; }

private:
	VUtils::MeshInterface *mi = nullptr;
	VUtils::MeshVoxel *mv = nullptr;
};

/// Appends mesh data to detail.
/// @param gdp Detail.
/// @param voxel Voxel of type MVF_GEOMETRY_VOXEL.
static int addMeshVoxelData(GU_Detail &gdp, VUtils::MeshVoxel &voxel)
{
	const VUtils::MeshChannel *vertChan = voxel.getChannel(VERT_GEOM_CHANNEL);
	const VUtils::MeshChannel *faceChan = voxel.getChannel(FACE_TOPO_CHANNEL);
	if (!vertChan || !faceChan) {
		gdp.addWarning(GU_WARNING_NO_METAOBJECTS, "Found invalid mesh voxel!");
		return false;
	}

	const VUtils::VertGeomData *verts = reinterpret_cast<VUtils::VertGeomData*>(vertChan->data);
	const VUtils::FaceTopoData *faces = reinterpret_cast<VUtils::FaceTopoData*>(faceChan->data);
	if (!verts || !faces) {
		gdp.addWarning(GU_WARNING_NO_METAOBJECTS, "Found invalid mesh voxel data!");
		return false;
	}

	const int numVerts = vertChan->numElements;
	const int numFaces = faceChan->numElements;

	const GA_Offset pointOffset = gdp.appendPointBlock(numVerts);
	for (int vertexIndex = 0; vertexIndex < numVerts; ++vertexIndex) {
		const VUtils::Vector &vert = verts[vertexIndex];
		gdp.setPos3(pointOffset + vertexIndex, UT_Vector3F(vert.x, vert.y, vert.z));
	}

	for (int faceIndex = 0; faceIndex < numFaces; ++faceIndex) {
		const VUtils::FaceTopoData &face = faces[faceIndex];

		GU_PrimPoly *poly = GU_PrimPoly::build(&gdp, 3, GU_POLY_CLOSED, 0);
		poly->setVertexPoint(0, pointOffset + face.v[0]);
		poly->setVertexPoint(1, pointOffset + face.v[1]);
		poly->setVertexPoint(2, pointOffset + face.v[2]);
		poly->reverse();
	}

	return true;
}

/// Appends hair data to detail.
/// @param gdp Detail.
/// @param voxel Voxel of type MVF_HAIR_GEOMETRY_VOXEL.
static int addHairVoxelData(GU_Detail &gdp, VUtils::MeshVoxel &voxel)
{
	VUtils::MeshChannel *vertChan = voxel.getChannel(HAIR_VERT_CHANNEL);
	VUtils::MeshChannel *strandChan = voxel.getChannel(HAIR_NUM_VERT_CHANNEL);
	if (!vertChan || !strandChan) {
		gdp.addWarning(GU_WARNING_NO_METAOBJECTS, "Found invalid hair voxel!");
		return false;
	}

	const VUtils::VertGeomData *verts   = reinterpret_cast<VUtils::VertGeomData*>(vertChan->data);
	const int                  *strands = reinterpret_cast<int*>(strandChan->data);
	if (!verts || !strands) {
		gdp.addWarning(GU_WARNING_NO_METAOBJECTS, "Found invalid hair voxel data!");
		return false;
	}

	const int numVerts   = vertChan->numElements;
	const int numStrands = strandChan->numElements;

	GA_Offset pointOffset = gdp.appendPointBlock(numVerts);
	for (int v = 0; v < numVerts; ++v) {
		const VUtils::Vector &vert = verts[v];
		gdp.setPos3(pointOffset + v, UT_Vector3F(vert.x, vert.y, vert.z));
	}

	for (int strandIndex = 0; strandIndex < numStrands; ++strandIndex) {
		const int vertsPerStrand = strands[strandIndex];

		GU_PrimPoly *poly = GU_PrimPoly::build(&gdp, vertsPerStrand, GU_POLY_OPEN, 0);
		for (int strandVertexIndex = 0; strandVertexIndex < vertsPerStrand; ++strandVertexIndex) {
			poly->setVertexPoint(strandVertexIndex, pointOffset + strandVertexIndex);
		}

		pointOffset += vertsPerStrand;
	}

	return true;
}

/// Appends voxel data to detail.
/// @param gdp Detail.
/// @param voxel Voxel.
static int addVoxelData(GU_Detail &gdp, VUtils::MeshVoxel &voxel, uint32 voxelFlags)
{
	if (voxelFlags & MVF_GEOMETRY_VOXEL ||
		voxelFlags & MVF_PREVIEW_VOXEL)
	{
		return addMeshVoxelData(gdp, voxel);
	}
	if (voxelFlags & MVF_HAIR_GEOMETRY_VOXEL) {
		return addHairVoxelData(gdp, voxel);
	}
	return false;
}

static int addVoxelData(GU_Detail &gdp, VUtils::MeshInterface &mi, int voxelIndex)
{
	MeshVoxelRAII voxelRaii(&mi, voxelIndex);
	if (!voxelRaii.isValid())
		return false;

	const uint32 voxelFlags = mi.getVoxelFlags(voxelIndex);

	return addVoxelData(gdp, voxelRaii.getVoxel(), voxelFlags);
}

static VUtils::Box getBoundingBox(VUtils::MeshInterface &mi)
{
	VUtils::Box bbox = mi.getBBox();

	if (bbox.isEmpty() || bbox.isInfinite()) {
		bbox.init();
		bbox += VUtils::Vector( 1.0f,  1.0f,  1.0f);
		bbox += VUtils::Vector(-1.0f, -1.0f, -1.0f);
	}

	return bbox;
}

static Hash::MHash getBoundingBoxHash(VUtils::MeshInterface &mi)
{
	const VUtils::Box &bbox = getBoundingBox(mi);

	Hash::MHash bboxHash = 0;
	Hash::MurmurHash3_x86_32(&bbox, sizeof(VUtils::Box), 42, &bboxHash);

	return bboxHash;
}

static Hash::MHash getVoxelHash(VUtils::MeshVoxel &voxel, uint32 voxelFlags)
{
	Hash::MHash hashKey = 0;

	VUtils::MeshChannel *channel = nullptr;

	if (voxelFlags & MVF_GEOMETRY_VOXEL ||
		voxelFlags & MVF_PREVIEW_VOXEL)
	{
		channel = voxel.getChannel(VERT_GEOM_CHANNEL);
	}
	else if (voxelFlags & MVF_HAIR_GEOMETRY_VOXEL) {
		channel = voxel.getChannel(HAIR_VERT_CHANNEL);
	}

	if (channel && channel->data) {
		const int dataSize = channel->elementSize * channel->numElements;
		const uint32 seed = reinterpret_cast<uintptr_t>(channel->data);

		Hash::MurmurHash3_x86_32(channel->data, dataSize, seed, &hashKey);
	}

	return hashKey;
}

static Hash::MHash getVoxelHash(VUtils::MeshInterface &mi, int voxelIndex)
{
	MeshVoxelRAII voxelRaii(&mi, voxelIndex);
	if (!voxelRaii.isValid())
		return false;

	const uint32 voxelFlags = mi.getVoxelFlags(voxelIndex);

	return getVoxelHash(voxelRaii.getVoxel(), voxelFlags);
}

/// Creates a box in detail.
/// @param gdp Detail.
/// @param mi MeshInterface to get bounding box from.
static int addBoundingBoxData(GU_Detail &gdp, VUtils::MeshInterface &mi)
{
	const VUtils::Box &bbox = getBoundingBox(mi);

	const VUtils::Vector &boxMin = bbox.pmin;
	const VUtils::Vector &boxMax = bbox.pmax;

	gdp.cube(boxMin[0], boxMax[0],
			 boxMin[1], boxMax[1],
			 boxMin[2], boxMax[2],
			 0, 0, 0, 0,
			 true);

	return true;
}

class VRayProxyCache
{
public:
	/// Geometry hash type.
	typedef Hash::MHash DetailKey;

	/// Detail cache entry.
	struct DetailEntry {
		DetailEntry() {
			bbox.initBounds();
		}

		/// Geometry bounding box.
		UT_BoundingBox bbox;

		/// Geometry preview type.
		LOD lod = LOD_PREVIEW;

		/// Geometry detail.
		GU_DetailHandle detail;
	};

	/// Maps geometry hash (currently vertex array hash) to GU_DetailHandle.
	typedef VUtils::HashMap<DetailKey, DetailEntry> DetailCache;

	/// Maps frame key to geometry hash.
	typedef VUtils::HashMap<int, DetailKey> FrameCache;

	VRayProxyCache() {}

	~VRayProxyCache() {
		freeMem();
	}

	/// Return the detail handle for a proxy packed primitive (based on frame, LOD)
	/// Caches the detail if not present in cache
	/// @param options[in] - proxy primitive options
	/// @return detail handle  if successful or emty one on error
	GU_DetailHandle getDetail(const VRayProxyRefKey &options);

	int open(const char *filepath);

private:
	/// Clears the caches and resets current .vrmesh file.
	void freeMem();

	/// Returns function to get the actual frame in the .vrmesh file based on
	/// proxy primitive options.
	int getFrame(const VRayProxyRefKey &options) const;
	
	GU_DetailHandle VRayProxyCache::addDetail(const VRayProxyRefKey &options);

	/// MeshFile instance. 
	VUtils::MeshFile *meshFile = nullptr;

	/// Frame key to geometry hash cache.
	FrameCache frameCache;

	/// Geometry hash to detail cache.
	DetailCache detailCache;
};

typedef VUtils::StringHashMap<VRayProxyCache> VRayProxyCacheMan;

static UT_Lock theLock;
static VRayProxyCacheMan theCacheMan;

GU_DetailHandle VRayForHoudini::getVRayProxyDetail(const VRayProxyRefKey &options)
{
	UT_AutoLock lock(theLock);

	if (options.filePath.empty())
		return GU_DetailHandle();

	const char *filePath = options.filePath.ptr();

	VRayProxyCacheMan::iterator it = theCacheMan.find(filePath);
	if (it != theCacheMan.end()) {
		return it.data().getDetail(options);
	}

	VRayProxyCache &cache = theCacheMan[filePath];
	if (!cache.open(filePath)) {
		theCacheMan.erase(filePath);
		return GU_DetailHandle();
	}

	return cache.getDetail(options);
}

bool VRayForHoudini::clearVRayProxyCache(const char *filepath)
{
	UT_AutoLock lock(theLock);
	return theCacheMan.erase(filepath);
}

bool VRayForHoudini::getVRayProxyBoundingBox(const VRayProxyRefKey &options, UT_BoundingBox &box)
{
	UT_AutoLock lock(theLock);

	if (options.filePath.empty())
		return false;

	// TODO
	return true;
}

int VRayProxyCache::open(const char *filepath)
{
	freeMem();

	VUtils::CharString path(filepath);
	if (!bmpCheckAssetPath(path, NULL, NULL, false)) {
		Log::getLog().error("VRayProxy: Can't find file \"%s\"", filepath);
		return false;
	}

	meshFile = VUtils::newDefaultMeshFile(path.ptr());
	if (!(meshFile && meshFile->init(path.ptr()))) {
		Log::getLog().error("VRayProxy: Can't open file \"%s\"", filepath);
		freeMem();
		return false;
	}

	return true;
}

void VRayProxyCache::freeMem()
{
	if (meshFile) {
		deleteDefaultMeshFile(meshFile);
		meshFile = nullptr;
	}

	frameCache.clear();
	detailCache.clear();
}

int VRayProxyCache::getFrame(const VRayProxyRefKey &options) const
{
	const int animStart  = options.animOverride ? options.animStart : 0;

	int animLength = options.animOverride ? options.animLength : 0;
	if (animLength <= 0) {
		animLength = VUtils::Max(meshFile->getNumFrames(), 1);
	}

	return VUtils::fast_ceil(calcFrameIndex(options.f,
	                                        static_cast<VUtils::MeshFileAnimType::Enum>(options.animType),
	                                        animStart,
	                                        animLength,
	                                        options.animOffset,
	                                        options.animSpeed));
}

GU_DetailHandle VRayProxyCache::addDetail(const VRayProxyRefKey &options)
{
	vassert(meshFile);

	GU_DetailHandle gdpHandle;

	const int frameIndex = getFrame(options);
	meshFile->setCurrentFrame(frameIndex);

	Hash::MHash detailHash;

	if (options.lod == LOD_BBOX) {
		detailHash = getBoundingBoxHash(*meshFile);
	}
	else {
		const int numHashVoxels = options.lod == LOD_PREVIEW ? 1 : meshFile->getNumVoxels();
		VUtils::Table<Hash::MHash> voxelsHash(numHashVoxels, 0);

		for (int voxelIndex = 0; voxelIndex < meshFile->getNumVoxels(); ++voxelIndex) {
			const uint32 voxelFlags = meshFile->getVoxelFlags(voxelIndex);
			if (options.lod == LOD_PREVIEW) {
				if (voxelFlags & MVF_PREVIEW_VOXEL) {
					voxelsHash[0] = getVoxelHash(*meshFile, voxelIndex);
					break;
				}
			}
			else if (options.lod == LOD_FULL) {
				voxelsHash[voxelIndex] = getVoxelHash(*meshFile, voxelIndex);
			}
		}

		Hash::MurmurHash3_x86_32(voxelsHash.elements, voxelsHash.count() * sizeof(Hash::MHash), 42, &detailHash);
	}

	const DetailCache::iterator detailIt = detailCache.find(detailHash);
	if (detailIt != detailCache.end()) {
		gdpHandle = detailIt.data().detail;
	}
	else {
		const FrameCache::iterator frameIt = frameCache.find(frameIndex);
		if (frameIt != frameCache.end()) {
			frameCache.erase(frameIt);
		}

		GU_Detail *gdp = new GU_Detail;

		if (options.lod == LOD_BBOX) {
			addBoundingBoxData(*gdp, *meshFile);
		}
		else {
			for (int voxelIndex = 0; voxelIndex < meshFile->getNumVoxels(); ++voxelIndex) {
				const uint32 voxelFlags = meshFile->getVoxelFlags(voxelIndex);
				if (options.lod == LOD_PREVIEW) {
					if (voxelFlags & MVF_PREVIEW_VOXEL) {
						addVoxelData(*gdp, *meshFile, voxelIndex); 
						break;
					}
				}
				else if (options.lod == LOD_FULL) {
					addVoxelData(*gdp, *meshFile, voxelIndex);
				}
			}
		}

		DetailEntry &entry = detailCache[detailHash];
		entry.lod = options.lod;
		entry.detail.allocateAndSet(gdp);

		gdpHandle = entry.detail;
	}

	frameCache.insert(frameIndex, detailHash);

	return gdpHandle;
}


GU_DetailHandle VRayProxyCache::getDetail(const VRayProxyRefKey &options)
{
	GU_DetailHandle gdpHandle;

	if (!meshFile)
		return gdpHandle;

	const int frameIndex = getFrame(options);

	const FrameCache::iterator frameIt = frameCache.find(frameIndex);
	if (frameIt == frameCache.end()) {
		gdpHandle = addDetail(options);
	}
	else {
		const DetailKey detailKey = frameIt.data();

		const DetailCache::iterator detailIt = detailCache.find(detailKey);
		if (detailIt == detailCache.end()) {
			gdpHandle = addDetail(options);
		}
		else {
			const DetailEntry detailEntry = detailIt.data();
			if (detailEntry.lod == options.lod) {
				gdpHandle = detailEntry.detail;
			}
			else {
				detailCache.erase(detailIt);
				gdpHandle = addDetail(options);
			}			
		}
	}

	return gdpHandle;
}
