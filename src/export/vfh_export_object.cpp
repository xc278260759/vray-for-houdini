//
// Copyright (c) 2015-2018, Chaos Software Ltd
//
// V-Ray For Houdini
//
// ACCESSIBLE SOURCE CODE WITHOUT DISTRIBUTION OF MODIFICATION LICENSE
//
// Full license text: https://github.com/ChaosGroup/vray-for-houdini/blob/master/LICENSE
//

#include "sop/sop_vrayproxy.h"
#include "sop/sop_vrayscene.h"
#include "vop/vop_node_base.h"

#include "vfh_exporter.h"
#include "vfh_prm_templates.h"
#include "vfh_export_geom.h"
#include "vfh_attr_utils.h"
#include "vfh_log.h"

#include <PRM/PRM_Parm.h>
#include <OP/OP_Bundle.h>
#include <OP/OP_BundleList.h>
#include <GA/GA_IntrinsicMacros.h>
#include <OBJ/OBJ_Geometry.h>

using namespace VRayForHoudini;

VUtils::FastCriticalSection VRayExporter::csect;

OP_Node* VRayExporter::getObjMaterial(OBJ_Node *objNode, fpreal t)
{
	if (!objNode)
		return nullptr;
	return objNode ? objNode->getMaterialNode(t) : nullptr;
}

/// An OP_Node map for new objects in RT session.
typedef QMap<QString, OP_Node*> OpNodeMap;

/// A map for new objects in RT session.
static OpNodeMap rtNewObjQueue;

void newObjHandler(OP_Node *caller, void *callee, OP_EventType type, void *data)
{
	VRayExporter &exporter = *reinterpret_cast<VRayExporter*>(callee);
	vassert(!exporter.inSceneExport);

	const QString objPath(caller->getFullPath().nonNullBuffer());
	if (objPath.isEmpty())
		return;

	Log::getLog().debug("newObjHandler: \"%s\" -> '%s'",
	                    qPrintable(objPath), OPeventToString(type));

	// My tests are showing that this will be the last unique event after
	// object creation.
	if (type == OP_UI_CHANGED) {
		const OpNodeMap::iterator it = rtNewObjQueue.find(objPath);
		if (it != rtNewObjQueue.end()) {
			OP_Node *opNode = it.value();

			rtNewObjQueue.erase(it);

			exporter.delOpCallback(opNode, newObjHandler);
			exporter.exportObject(opNode);

			exporter.getRenderer().getVRay().commit();
		}
	}
}

void VRayExporter::rtCallbackObjNetwork(OP_Node *caller, void *callee, OP_EventType type, void *data)
{
	VRayExporter &exporter = *reinterpret_cast<VRayExporter*>(callee);
	if (exporter.inSceneExport)
		return;

	const QString objPath(caller->getFullPath().nonNullBuffer());
	if (objPath.isEmpty())
		return;

	Log::getLog().debug("rtCallbackObjNetwork: \"%s\" -> '%s'",
	                    qPrintable(objPath), OPeventToString(type));

	if (type == OP_CHILD_CREATED) {
		OP_Node *opNode = reinterpret_cast<OP_Node*>(data);
		if (opNode) {
			const QString newOpPath(opNode->getFullPath().nonNullBuffer());
			if (!objPath.isEmpty()) {
#if 0
				// Render opearator is not yet ready, register callback that will handle export.
				rtNewObjQueue.insert(newOpPath, opNode);
				exporter.addOpCallback(opNode, newObjHandler);
#endif
			}
		}
	}
}

void VRayExporter::RtCallbackOPDirector(OP_Node *caller, void *callee, OP_EventType type, void *data)
{
	VRayExporter &exporter = *reinterpret_cast<VRayExporter*>(callee);
	if (exporter.inSceneExport)
		return;

	Log::getLog().debug("RtCallbackOPDirector: \"%s\" -> '%s'",
	                    caller->getFullPath().nonNullBuffer(), OPeventToString(type));

	if (type == OP_UI_CURRENT_CHANGED) {
		if (data) {
			OBJ_Node* objNode = static_cast<OP_Node*>(data)->castToOBJNode();
			if (objNode) {
#if 0
				ObjectExporter &objExporter = exporter.getObjectExporter();

				// Clear caches, otherwise plugin will not be exported if it is deleted and recreated
				objExporter.clearOpPluginCache();
				objExporter.clearPrimPluginCache();

				// Update node
				objExporter.removeGenerated(*objNode);
				exporter.exportObject(objNode);
#endif
			}
		}
	}
}

void VRayExporter::RtCallbackOBJGeometry(OP_Node *caller, void *callee, OP_EventType type, void *data)
{
	VRayExporter &exporter = *reinterpret_cast<VRayExporter*>(callee);
	if (exporter.inSceneExport)
		return;

	if (!csect.tryEnter())
		return;

	Log::getLog().debug("RtCallbackOBJGeometry: %s from \"%s\"", OPeventToString(type), caller->getName().buffer());

	UT_ASSERT(caller->castToOBJNode());
	UT_ASSERT(caller->castToOBJNode()->castToOBJGeometry());

	OBJ_Node &objNode = *caller->castToOBJNode();
	OBJ_Geometry &objGeo = *objNode.castToOBJGeometry();

	int shouldReExport = false;
	switch (type) {
		case OP_PARM_CHANGED: {
			if (Parm::isParmSwitcher(*caller, reinterpret_cast<intptr_t>(data))) {
				break;
			}

			const PRM_Parm *prm = Parm::getParm(*caller, reinterpret_cast<intptr_t>(data));
			if (prm) {
				const UT_StringRef &prmToken = prm->getToken();
				const PRM_SpareData	*spare = prm->getSparePtr();

				Log::getLog().debug("  Parm: %s", prmToken.buffer());

				shouldReExport =
					prmToken.equal(objGeo.getMaterialParmToken()) ||
					prmToken.equal(VFH_ATTR_SHOP_MATERIAL_STYLESHEET) ||
					(spare && spare->getValue(OBJ_MATERIAL_SPARE_TAG));
			}
		}
		case OP_FLAG_CHANGED:
		case OP_INPUT_CHANGED:
		case OP_INPUT_REWIRED: {
			ObjectExporter &objExporter = exporter.getObjectExporter();

			// Otherwise we won't update plugin.
			objExporter.clearOpPluginCache();
			objExporter.clearPrimPluginCache();

			// Store current state
			const int geomExpState = objExporter.getExportGeometry();
			objExporter.setExportGeometry(shouldReExport);

			// Update node
			objExporter.removeGenerated(objNode);
			objExporter.exportObject(objNode);

			// Restore state
			objExporter.setExportGeometry(geomExpState);
			break;
		}
		case OP_NODE_PREDELETE: {
			exporter.delOpCallbacks(caller);

			ObjectExporter &objExporter = exporter.getObjectExporter();
			objExporter.removeObject(objNode);
			break;
		}
		default:
			break;
	}

	csect.leave();
}


void VRayExporter::RtCallbackSOPChanged(OP_Node *caller, void *callee, OP_EventType type, void *data)
{
	VRayExporter &exporter = *reinterpret_cast<VRayExporter*>(callee);
	if (exporter.inSceneExport)
		return;

	if (!csect.tryEnter())
		return;

	Log::getLog().debug("RtCallbackSOPChanged: %s from \"%s\"", OPeventToString(type), caller->getName().buffer());

	UT_ASSERT(caller->getCreator());
	UT_ASSERT(caller->getCreator()->castToOBJNode());
	UT_ASSERT(caller->getCreator()->castToOBJNode()->castToOBJGeometry());

	OBJ_Node &objNode = *caller->getCreator()->castToOBJNode();
	OBJ_Geometry &objGeo = *objNode.castToOBJGeometry();

	switch (type) {
		case OP_PARM_CHANGED: {
			if (Parm::isParmSwitcher(*caller, reinterpret_cast<intptr_t>(data))) {
				break;
			}
		}
		case OP_FLAG_CHANGED:
		case OP_INPUT_CHANGED:
		case OP_INPUT_REWIRED: {
			SOP_Node *geomNode = objGeo.getRenderSopPtr();
			if (geomNode) {
				ObjectExporter &objExporter = exporter.getObjectExporter();

				objExporter.clearPrimPluginCache();
				objExporter.removeGenerated(objNode);

				objExporter.exportObject(objNode);

				if (exporter.getSessionType() == VfhSessionType::rt) {
					exporter.addOpCallback(geomNode, RtCallbackSOPChanged);
				}
			}
			break;
		}
		case OP_NODE_PREDELETE: {
			exporter.delOpCallbacks(caller);
			break;
		}
		default:
			break;
	}

	csect.leave();
}


void VRayExporter::RtCallbackVRayClipper(OP_Node *caller, void *callee, OP_EventType type, void *data)
{
	VRayExporter &exporter = *reinterpret_cast<VRayExporter*>(callee);
	if (exporter.inSceneExport)
		return;

	if (!csect.tryEnter())
		return;

	OBJ_Node *clipperNode = caller->castToOBJNode();

	Log::getLog().debug("RtCallbackVRayClipper: %s from \"%s\"", OPeventToString(type), clipperNode->getName().buffer());

	switch (type) {
		case OP_PARM_CHANGED: {
			if (Parm::isParmSwitcher(*caller, reinterpret_cast<intptr_t>(data))) {
				break;
			}
		}
		case OP_FLAG_CHANGED:
		case OP_INPUT_CHANGED:
		case OP_INPUT_REWIRED:
		{
			exporter.exportVRayClipper(*clipperNode);
			break;
		}
		case OP_NODE_PREDELETE:
		{
			exporter.delOpCallbacks(caller);
			exporter.removePlugin(clipperNode);
			break;
		}
		default:
			break;
	}

	csect.leave();
}

template <typename ValueType>
static void dumpValues(const char *name, const UT_Options *options, bool readonly, GA_StorageClass storage, ValueType values, exint evalsize) {
	Log::getLog().debug("name = %s", name);
}

static void dumpType(OBJ_OBJECT_TYPE objType) {
	QString objTypeStr;
	if (objType & OBJ_WORLD) { objTypeStr += "OBJ_WORLD | "; }
	if (objType & OBJ_GEOMETRY) {  objTypeStr += "OBJ_GEOMETRY | ";  }
	if (objType & OBJ_CAMERA) { objTypeStr += "OBJ_CAMERA | "; }
	if (objType & OBJ_LIGHT)    { objTypeStr += "OBJ_LIGHT | "; }
	if (objType & OBJ_RENDERER) {  objTypeStr += "OBJ_RENDERER | ";  }
	if (objType & OBJ_FOG) {  objTypeStr += "OBJ_FOG | ";  }
	if (objType & OBJ_BONE) {  objTypeStr += "OBJ_BONE | ";  }
	if (objType & OBJ_HANDLE) { objTypeStr += "OBJ_HANDLE | "; }
	if (objType & OBJ_BLEND) { objTypeStr += "OBJ_BLEND | "; }
	if (objType & OBJ_FORCE) { objTypeStr += "OBJ_FORCE | "; }
	if (objType & OBJ_CAMSWITCH) { objTypeStr += "OBJ_CAMSWITCH | "; }
	if (objType & OBJ_SOUND) { objTypeStr += "OBJ_SOUND | "; }
	if (objType & OBJ_MICROPHONE) { objTypeStr += "OBJ_MICROPHONE | "; }
	if (objType & OBJ_SUBNET) { objTypeStr += "OBJ_SUBNET | "; }
	if (objType & OBJ_FETCH) { objTypeStr += "OBJ_FETCH | "; }
	if (objType & OBJ_NULL) {  objTypeStr += "OBJ_NULL | ";  }
	if (objType & OBJ_STICKY) { objTypeStr += "OBJ_STICKY | "; }
	if (objType & OBJ_DOPNET) { objTypeStr += "OBJ_DOPNET | "; }
	if (objType & OBJ_RIVET) { objTypeStr += "OBJ_RIVET | "; }
	if (objType & OBJ_MUSCLE) { objTypeStr += "OBJ_MUSCLE | "; }
	if (objType & OBJ_STD_LIGHT) { objTypeStr += "OBJ_STD_LIGHT | "; }
	if (objType & OBJ_STD_BONE) {  objTypeStr += "OBJ_STD_BONE | ";  }
	if (objType & OBJ_STD_HANDLE) { objTypeStr += "OBJ_STD_HANDLE | "; }
	if (objType & OBJ_STD_BLEND) { objTypeStr += "OBJ_STD_BLEND | "; }
	if (objType & OBJ_STD_FETCH) { objTypeStr += "OBJ_STD_FETCH | "; }
	if (objType & OBJ_STD_STICKY) { objTypeStr += "OBJ_STD_STICKY | "; }
	if (objType & OBJ_STD_RIVET) { objTypeStr += "OBJ_STD_RIVET | "; }
	if (objType & OBJ_STD_NULL) {  objTypeStr += "OBJ_STD_NULL | ";  }
	if (objType & OBJ_STD_MUSCLE) { objTypeStr += "OBJ_STD_MUSCLE | "; }
	if (objType & OBJ_STD_CAMSWITCH) { objTypeStr += "OBJ_STD_CAMSWITCH | "; }
	if (objType & OBJ_ALL) {  objTypeStr += "OBJ_ALL"; }
	Log::getLog().debug("OBJ_OBJECT_TYPE = %s", qPrintable(objTypeStr));
}

VRay::Plugin VRayExporter::exportObject(OP_Node *opNode)
{
	if (!opNode) {
		return VRay::Plugin();
	}

	const UT_String &objOpType = opNode->getOperator()->getName();
	if (objOpType.equal("guidegroom") ||
		objOpType.equal("guidedeform"))
	{
		return VRay::Plugin();
	}

	OBJ_Node *objNode = opNode->castToOBJNode();
	OBJ_Light *objLight = objNode->castToOBJLight();
	if (!objNode &&
		!objLight)
	{
		return VRay::Plugin();
	}

	OP_Node *renderOp = objNode->getRenderNodePtr();

	if (objOpType.equal("VRayNodeVRayClipper")) {
		return exportVRayClipper(*objNode);
	}

	if (!objLight && !renderOp) {
		Log::getLog().error("OBJ \"%s\": Render OP is not found!",
							opNode->getName().buffer());
	}
	else {
		VRay::Plugin plugin = objectExporter.exportObject(*objNode);
		if (plugin.isEmpty()) {
			Log::getLog().error("Error exporting OBJ: %s [%s]",
			                    opNode->getName().buffer(),
			                    objOpType.buffer());
			Log::getLog().error("  Render OP: %s:\"%s\"",
			                    renderOp->getName().buffer(),
			                    renderOp->getOperator()->getName().buffer());
		}
		else {
			if (objLight) {
				addOpCallback(opNode, RtCallbackLight);
			}
			else {
				if (sessionType == VfhSessionType::rt) {
					addOpCallback(opNode, RtCallbackOBJGeometry);
					addOpCallback(renderOp, RtCallbackSOPChanged);
				}
			}
		}

		return plugin;
	}

	return VRay::Plugin();
}


VRay::Plugin VRayExporter::exportVRayClipper(OBJ_Node &clipperNode)
{
	const fpreal t = getContext().getTime();

	Attrs::PluginDesc pluginDesc(getPluginName(clipperNode),
	                             SL("VRayClipper"));

	VRay::Plugin clipNodePlugin;

	// Find and export clipping geometry plugins.
	OP_Node *opNode = getOpNodeFromAttr(clipperNode, "clip_mesh", t);
	if (opNode &&
		opNode->getOpTypeID() == OBJ_OPTYPE_ID &&
		opNode->getUniqueId() != clipperNode.getUniqueId())
	{
		OBJ_Node *objNode = opNode->castToOBJNode();
		if (objNode && objNode->getObjectType() == OBJ_GEOMETRY) {
			clipNodePlugin = exportObject(objNode);
		}
	}

	pluginDesc.add(Attrs::PluginAttr("clip_mesh", clipNodePlugin));

	// Find and export excussion node plugins
	UT_String nodeMask;
	clipperNode.evalString(nodeMask, "exclusion_nodes", 0, 0, t);

	// Get a manager that contains objects
	OP_Network *objMan = OPgetDirector()->getManager("obj");

	UT_String bundle_name;
	OP_Bundle *bundle = OPgetDirector()->getBundles()->getPattern(bundle_name, objMan, objMan, nodeMask, "!!OBJ!!");

	// Get the node list for processing
	OP_NodeList nodeList;
	bundle->getMembers(nodeList);

	// Release the internal bundle created by getPattern()
	OPgetDirector()->getBundles()->deReferenceBundle(bundle_name);

	VRay::VUtils::ValueRefList nodePluginList(nodeList.size());
	int nodeIdx = 0;

	for (OP_Node *node : nodeList) {
		OBJ_Node *objNode = node->castToOBJNode();
		if (!objNode ||
		    !node->getVisible() ||
		    objNode->getObjectType() != OBJ_GEOMETRY)
		{
			continue;
		}

		const Attrs::PluginDesc nodePluginDesc(getPluginName(*objNode), SL("Node"));
		VRay::Plugin nodePlugin = exportPlugin(nodePluginDesc);
		if (nodePlugin.isEmpty()) {
			continue;
		}

		nodePluginList[nodeIdx++].setPlugin(nodePlugin);
	}

	pluginDesc.add(SL("exclusion_nodes"), nodePluginList);

	// transform
	pluginDesc.add(Attrs::PluginAttr("transform", getObjTransform(&clipperNode, m_context, clipNodePlugin.isEmpty())));

	// material
	const VRay::Plugin &mtlPlugin = exportMaterial(clipperNode.getMaterialNode(t));
	if (mtlPlugin.isNotEmpty()) {
		pluginDesc.add(Attrs::PluginAttr("material", mtlPlugin));
	}

	setAttrsFromOpNodePrms(pluginDesc, &clipperNode);

	addOpCallback(&clipperNode, RtCallbackVRayClipper);

	return exportPlugin(pluginDesc);
}
