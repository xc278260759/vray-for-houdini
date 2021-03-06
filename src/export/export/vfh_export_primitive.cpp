//
// Copyright (c) 2015-2018, Chaos Software Ltd
//
// V-Ray For Houdini
//
// ACCESSIBLE SOURCE CODE WITHOUT DISTRIBUTION OF MODIFICATION LICENSE
//
// Full license text: https://github.com/ChaosGroup/vray-for-houdini/blob/master/LICENSE
//

#include "vfh_export_primitive.h"
#include "vfh_exporter.h"
#include "vfh_op_utils.h"

#include "gu_volumegridref.h"

#include "vop/vop_node_base.h"
#include "vop/material/vop_PhoenixSim.h"

#include <GU/GU_PrimPacked.h>
#include <GU/GU_PrimVDB.h>
#include <SOP/SOP_Node.h>
#include <SHOP/SHOP_Node.h>
#include <GEO/GEO_Primitive.h>
#include <GU/GU_PrimVolume.h>
#include <GU/GU_Detail.h>
#include <GA/GA_SaveMap.h>

using namespace VRayForHoudini;

#ifdef CGR_HAS_AUR

namespace{

/// Wrapper over GEO_PrimVolume and GEO_PrimVDB providing common interface
/// this wrapper *can* be INVALID - it can be initialized with unsuported primitive
/// the wrapper *must* be checkked before use with its bool operator
/// methods called on INVALID wrapper will return default (0/1) initialized data
struct VolumeProxy {
	/// Create a proxy from a primitive, could be nyllptr or not supported primitive
	explicit VolumeProxy(const GEO_Primitive *prim)
		: m_vdb(nullptr)
		, m_vol(nullptr)
		, m_prim(prim)
	{
		if (!prim)
			return;

		if (prim->getTypeId() == GEO_PRIMVOLUME) {
			m_vol = static_cast<const GEO_PrimVolume*>(m_prim);
		}
		else if (prim->getTypeId() == GEO_PRIMVDB) {
			m_vdb = static_cast<const GEO_PrimVDB*>(m_prim);
		}
	};

	/// Get the resolution in voxels
	/// @res[out] - resolution in order x, y, z
	void getRes(int res[3]) const {
		if (m_vol) {
			m_vol->getRes(res[0], res[1], res[2]);
		}
		else if (m_vdb) {
			m_vdb->getRes(res[0], res[1], res[2]);
		}
	}

	/// Get total number of voxels in this volume
	/// @return - voxel count
	exint voxCount() const {
		int res[3] = {0, 0, 0};
		this->getRes(res);
		return res[0] * res[1] * res[2];
	}

	/// Copy the volume data to a PTrArray
	/// @T - the output data type - Color or float
	/// @F - the type of the accesor function for the elements of PtrArray
	/// @data - data destination
	/// @acc - function called for each element in @data, should return reference to which each voxel is assigned to
	///        if @T is color, @acc should return reference to either the red or green or blue channels - used to export velocities
	template <typename T, typename F>
	void copyTo(VRay::VUtils::PtrArray<T> &data, F acc) const {
		int res[3] = {0, 0, 0};
		getRes(res);

		auto GetCellIndex = [&res](int x, int y, int z) {
			return (x + y * res[0] + z * res[1] * res[0]);
		};

		if (m_vdb) {
			const auto fGrid = openvdb::gridConstPtrCast<openvdb::FloatGrid>(m_vdb->getGridPtr());
			if (!fGrid)
				return;

			auto readHandle = fGrid->getConstAccessor();
			for (int x = 0; x < res[0]; ++x) {
				for (int y = 0; y < res[1]; ++y) {
					for (int z = 0; z < res[2]; ++z) {
						const float &val = readHandle.getValue(openvdb::Coord(x, y, z));
						const int &idx = GetCellIndex(x, y, z);
						acc(data[idx]) = val;
					}
				}
			}
		}
		else if (m_vol) {
			const UT_VoxelArrayReadHandleF vh = m_vol->getVoxelHandle();
			for (int x = 0; x < res[0]; ++x) {
				for (int y = 0; y < res[1]; ++y) {
					for (int z = 0; z < res[2]; ++z) {
						const float &val = vh->getValue(x, y, z);
						const int &idx = GetCellIndex(x, y, z);
						acc(data[idx]) = val;
					}
				}
			}
		}
	}

	/// Get the bary center of the volume
	/// @return - UT_Vector3 in voxel space
	UT_Vector3 getBaryCenter() const {
		if (m_vol) {
			return m_vol->baryCenter();
		}
		if (m_vdb) {
			return m_vdb->baryCenter();
		}
		return UT_Vector3(0, 0, 0);
	}

	/// Get voxel space transform
	/// @return - 4d matrix
	UT_Matrix4D getTransform() const {
		UT_Matrix4D res(1);
		if (m_vol) {
			m_vol->getTransform4(res);
		}
		else if (m_vdb) {
			m_vdb->getSpaceTransform().getTransform4(res);
		}
		return res;
	}

	/// Check if this volume is PrimVDB
	bool isVDB() const {
		return m_vdb;
	}

	/// Check if this volume is PrimVolume
	bool isVOL() const {
		return m_vol;
	}

	/// Check if this is a valid volume
	operator bool() const {
		return m_prim && (m_vol || m_vdb);
	}

	const GEO_PrimVDB *m_vdb; ///< pointer to the VDB primitive in this is vdb
	const GEO_PrimVolume *m_vol; ///< pointer to the VOL primitive if this is vol
	const GEO_Primitive *m_prim; ///< pointer to the primitive passed to constructor
};
}

static VOP::PhxShaderSim *getPhoenixShaderSimNode(OP_Node *matNode)
{
	if (!matNode)
		return nullptr;
	if (matNode->getOperator()->getName().equal("VRayNodePhxShaderSim"))
		return static_cast<VOP::PhxShaderSim*>(matNode);
	return static_cast<VOP::PhxShaderSim*>(getVRayNodeFromOp(*matNode, "Simulation", "PhxShaderSim"));
}

void HoudiniVolumeExporter::exportPrimitive(const GA_Primitive &prim, const PrimMaterial &primMtl,
                                            PluginList &volumePlugins)
{
#if 0
	GA_ROAttributeRef ref_name = detail.findStringTuple(GA_ATTRIB_PRIMITIVE, "name");
	const GA_ROHandleS hnd_name(ref_name.getAttribute());
	if (hnd_name.isInvalid()) {
		SOP_Node *sop = m_object.getRenderSopPtr();
		Log::getLog().error("%s: \"name\" attribute not found! Can't export fluid data!",
		                    sop ? sop->getFullPath().buffer() : "UNKNOWN");
		return;
	}

	typedef std::map<QString, VRay::Plugin> CustomFluidData;
	CustomFluidData customFluidData;
	int res[3] = {0, 0, 0};

	// will hold resolution for velocity channels as it can be different
	int velocityRes[3] = {0, 0, 0};

	VRay::Transform nodeTm = VRayExporter::getObjTransform(&m_object, ctx);
	VRay::Transform phxTm;

	VRay::VUtils::ColorRefList vel;

	// get the largest velocity count, sometimes they are not the same sizes!!
	int velVoxCount = -1;
	bool missmatchedSizes = false;
	for (GA_Iterator offIt(detail.getPrimitiveRange()); !offIt.atEnd(); offIt.advance()) {
		const GA_Offset off = *offIt;
		const GEO_Primitive *prim = detail.getGEOPrimitive(off);
		VolumeProxy vol(prim);
		const QString texType = hnd_name.get(off);
		if (vol && (texType == "vel.x" || texType == "vel.y" || texType == "vel.z")) {
			int chRes[3];
			vol.getRes(res);
			const int voxCount = res[0] * res[1] * res[2];
			if (velVoxCount != -1 && velVoxCount != voxCount) {
				missmatchedSizes = true;
			}
			velVoxCount = std::max(voxCount, velVoxCount);
			// set max res of the 3 components
			for (int c = 0; c < 3; ++c) {
				velocityRes[c] = std::max(velocityRes[c], chRes[c]);
			}
		}
	}

	if (velVoxCount > 0) {
		vel = VRay::VUtils::ColorRefList(velVoxCount);
		if (missmatchedSizes) {
			memset(vel.get(), 0, vel.size() * sizeof(VRay::Color));
		}
	}

	UT_Matrix4 channelTm;

	// check all primitives if we can make PrimExporter for it and export it
	for (GA_Iterator offIt(detail.getPrimitiveRange()); !offIt.atEnd(); offIt.advance()) {
		const GA_Offset off = *offIt;
		const GEO_Primitive *prim = detail.getGEOPrimitive(off);

		VolumeProxy volume(prim);
		const QString texType = hnd_name.get(off);

		// unknown primitive, or unknow volume type
		if (!volume || texType.empty()) {
			continue;
		}

		volume.getRes(res);
		const int voxCount = res[0] * res[1] * res[2];

		UT_Matrix4D m4 = volume.getTransform();
		UT_Vector3 center = volume.getBaryCenter();

		// phxTm matrix to convert from voxel space to object local space
		// Voxel space is defined to be the 2-radius cube from (-1,-1,-1) to (1,1,1) centered at (0,0,0)
		// Need to scale uniformly by 2 as for TexMayaFluid seems to span from (0,0,0) to (1,1,1)
		phxTm = utMatrixToVRayTransform(m4);
		phxTm.offset.set(center.x(), center.y(), center.z());
		phxTm.matrix.v0.x *= 2.0f;
		phxTm.matrix.v1.y *= 2.0f;
		phxTm.matrix.v2.z *= 2.0f;

		// phxMatchTm matrix to convert from voxel space to world space
		// Needed for TexMayaFluidTransformed
		// Should match with transform for PhxShaderSim (?)
		VRay::Transform phxMatchTm = nodeTm * phxTm;

		Log::getLog().debug("Volume \"%s\": %i x %i x %i",
		                    texType, res[0], res[1], res[2]);

		// extract data
		if (texType == "vel.x") {
			volume.copyTo(vel, std::bind(&VRay::Color::r, std::placeholders::_1));
			continue;
		}
		else if (texType == "vel.y") {
			volume.copyTo(vel, std::bind(&VRay::Color::g, std::placeholders::_1));
			continue;
		}
		else if (texType == "vel.z") {
			volume.copyTo(vel, std::bind(&VRay::Color::b, std::placeholders::_1));
			continue;
		}

		VRay::VUtils::FloatRefList values(voxCount);
		volume.copyTo(values, [](float &c) -> float& { return c; });

		const QString primPluginNamePrefix = texType + "|";

		Attrs::PluginDesc fluidTex(VRayExporter::getPluginName(&m_object, primPluginNamePrefix), "TexMayaFluid");
		fluidTex.add("size_x", res[0]));
		fluidTex.add("size_y", res[1]));
		fluidTex.add("size_z", res[2]));
		fluidTex.add("values", values));

		Attrs::PluginDesc fluidTexTm(VRayExporter::getPluginName(&m_object, primPluginNamePrefix + "Tm"),
		                             "TexMayaFluidTransformed");
		fluidTexTm.add("fluid_tex", m_exporter.exportPlugin(fluidTex)));
		fluidTexTm.add("fluid_value_scale", 1.0f));
		fluidTexTm.add("object_to_world", phxMatchTm));

		VRay::Plugin fluidTexPlugin = m_exporter.exportPlugin(fluidTexTm);

		if (texType == "density") {
			Attrs::PluginDesc fluidTexAlpha(VRayExporter::getPluginName(&m_object, primPluginNamePrefix + "Alpha"),
			                                "PhxShaderTexAlpha");
			fluidTexAlpha.add("ttex", fluidTexPlugin));

			fluidTexPlugin = m_exporter.exportPlugin(fluidTexAlpha);
		}

		customFluidData[texType] = fluidTexPlugin;
	}

	// write velocities if we have any
	if (vel.count()) {
		Attrs::PluginDesc velTexDesc(VRayExporter::getPluginName(&m_object, "vel"), "TexMayaFluid");
		velTexDesc.add("size_x", velocityRes[0]));
		velTexDesc.add("size_y", velocityRes[1]));
		velTexDesc.add("size_z", velocityRes[2]));
		velTexDesc.add("color_values", vel));

		Attrs::PluginDesc velTexTmDesc(VRayExporter::getPluginName(&m_object, "Vel@Tm@"), "TexMayaFluidTransformed");
		velTexTmDesc.add("fluid_tex", m_exporter.exportPlugin(velTexDesc)));
		velTexTmDesc.add("fluid_value_scale", 1.0f));

		VRay::Plugin velTmTex = m_exporter.exportPlugin(velTexTmDesc);

		velTmTex = m_exporter.exportPlugin(velTexTmDesc);

		customFluidData["velocity"] = velTmTex;
	}

	Attrs::PluginDesc phxShaderCacheDesc(VRayExporter::getPluginName(&m_object), "PhxShaderCache");
	phxShaderCacheDesc.add("grid_size_x", (float)res[0]));
	phxShaderCacheDesc.add("grid_size_y", (float)res[1]));
	phxShaderCacheDesc.add("grid_size_z", (float)res[2]));

	// Skip "cache_path" exporting - we don't have chache but texture plugins
	phxShaderCacheDesc.add("cache_path", Attrs::AttrTypeIgnore));

	VRay::Plugin phxShaderCache = m_exporter.exportPlugin(phxShaderCacheDesc);

	nodeTm = nodeTm * phxTm;
	Attrs::PluginAttrs overrides;
	overrides.push_back("node_transform", nodeTm));
	overrides.push_back("cache", phxShaderCache));

	if (customFluidData.size()) {
		if (customFluidData.count("heat")) {
			overrides.push_back("darg", 4)); // 4 == Texture
			overrides.push_back("dtex", customFluidData["heat"]));
		}
		if (customFluidData.count("density")) {
			overrides.push_back("targ", 4)); // 4 == Texture
			overrides.push_back("ttex", customFluidData["density"]));
		}
		if (customFluidData.count("temperature")) {
			overrides.push_back("earg", 4)); // 4 == Texture
			overrides.push_back("etex", customFluidData["temperature"]));
		}
		if (customFluidData.count("velocity")) {
			overrides.push_back("varg", 2)); // 2 == Texture
			overrides.push_back("vtex", customFluidData["velocity"]));
		}
	}

	OP_Node *matNode = m_exporter.getObjMaterial(&m_object, ctx.getTime());;

	OP_Node *phxSimNode = getPhoenixShaderSimNode(matNode);
	if (!phxSimNode) {
		Log::getLog().error("Can't find shop node for %s", phxShaderCache.getName());
	}
	else {
		exportSim(*phxSimNode, overrides, phxShaderCache.getName());
	}
#endif
}

VRay::Plugin VolumeExporter::exportVRayVolumeGridRef(OBJ_Node &objNode, const GU_PrimPacked &prim) const
{
	UT_Options opts;
	prim.saveOptions(opts, GA_SaveMap(prim.getDetail(), nullptr));

	const VRayVolumeGridRef *vrayVolumeGridRef = UTverify_cast<const VRayVolumeGridRef*>(prim.implementation());
	const int primID = vrayVolumeGridRef->getOptions().hash();

	Attrs::PluginDesc phxCache(SL("PhxShaderCache|") % QString::number(primID) % SL("@") % objNode.getName().buffer(),
	                           SL("PhxShaderCache"));
	if (pluginExporter.setAttrsFromUTOptions(phxCache, opts)) {
		return pluginExporter.exportPlugin(phxCache);
	}

	return VRay::Plugin();
}

void VolumeExporter::exportPrimitive(const GA_Primitive &prim, const PrimMaterial &primMtl, PluginList &volumePlugins)
{
	const GU_PrimPacked *primPacked = UTverify_cast<const GU_PrimPacked*>(&prim);
	vassert(primPacked && "PhxShaderCache plugin is not GU_PrimPacked!");

	VRay::Plugin cachePlugin = exportVRayVolumeGridRef(objNode, *primPacked);
	if (cachePlugin.isEmpty()) {
		Log::getLog().error("PhxShaderCache plugin is not exported for volume %s",
		                    objNode.getName().buffer());
		return;
	}

	OP_Node *matNode = primMtl.matNode;
	if (!matNode) {
		matNode = VRayExporter::getObjMaterial(&objNode, ctx.getTime());
	}

	VOP::PhxShaderSim *phxShaderSim = getPhoenixShaderSimNode(matNode);
	if (!phxShaderSim) {
		Log::getLog().error("Volume shader is not found for volume %s",
		                    objNode.getName().buffer());
		return;
	}

#pragma pack(push, 1)
	const struct PhxShaderSimKey {
		uint32 hash;
		exint detailID;
		GA_Offset primOffset;
	} phxShaderSimKey = {
		objNode.getFullPath().hash(),
		pluginExporter.getObjectExporter().getDetailID(),
		prim.getMapOffset()
	};
#pragma pack(pop)

	const uint32 phxShaderSimID = Hash::hashLittle(phxShaderSimKey);

	const QString phxShaderSimName =
		QString::asprintf("PhxShaderSim|%X", phxShaderSimID);

	Attrs::PluginDesc phxSim(phxShaderSimName,
	                         SL("PhxShaderSim"));
	phxShaderSim->asPluginDesc(phxSim, pluginExporter);

	pluginExporter.getShaderExporter().exportConnectedSockets(*phxShaderSim, phxSim);

	phxSim.add(SL("node_transform"), tm);
	phxSim.add(SL("cache"), cachePlugin);

	const VOP::PhxShaderSim::RenderMode rendMode = Phoenix::getRenderMode(*phxShaderSim);

	if (rendMode == VOP::PhxShaderSim::RenderMode::Volumetric) {
		phxSim.add(SL("renderAsVolumetric"), true);
	}

	const VRay::Plugin overwriteSim = pluginExporter.exportPlugin(phxSim);
	vassert(overwriteSim.isNotEmpty());

	if (rendMode == VOP::PhxShaderSim::RenderMode::Volumetric) {
		volumePlugins.append(overwriteSim);

		const Attrs::PluginDesc simVol(SL("__PhxShaderGlobalVolume__"),
		                               SL("PhxShaderSimVol"));
		pluginExporter.exportPlugin(simVol);
	}
	else {
		const bool isMesh = rendMode == VOP::PhxShaderSim::RenderMode::Mesh;

		const QString &phxWrapperPluginID = isMesh ? SL("PhxShaderSimMesh") : SL("PhxShaderSimGeom");

		Attrs::PluginDesc phxWrapper(SL("Wrapper@") % phxShaderSimName,
		                             phxWrapperPluginID);

		// Need to export as List().
		VRay::VUtils::ValueRefList phoenixSimParamValue(1);
		phoenixSimParamValue[0].setPlugin(overwriteSim);

		phxWrapper.add(SL("phoenix_sim"), phoenixSimParamValue);

		if (isMesh) {
			const int dynamicGeometry = Phoenix::getDynamicGeometry(*phxShaderSim);

			Attrs::PluginDesc staticMesh(SL("Geom@") % phxShaderSimName,
			                             SL("GeomStaticMesh"));
			staticMesh.add(SL("dynamic_geometry"), dynamicGeometry);

			phxWrapper.add(SL("static_mesh"), pluginExporter.exportPlugin(staticMesh));
		}

		const VRay::Plugin phxWrapperPlugin = pluginExporter.exportPlugin(phxWrapper);

		Attrs::PluginDesc node(SL("Node@") % phxShaderSimName,
		                       SL("Node"));
		node.add(SL("geometry"), phxWrapperPlugin);
		node.add(SL("visible"), true);
		node.add(SL("transform"), tm);
		node.add(SL("material"), pluginExporter.exportDefaultMaterial());

		const VRay::Plugin phxNodePlugin = pluginExporter.exportPlugin(node);
		volumePlugins.append(phxNodePlugin);
	}
}

#endif // CGR_HAS_AUR
