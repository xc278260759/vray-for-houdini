//
// Copyright (c) 2015-2018, Chaos Software Ltd
//
// V-Ray For Houdini
//
// ACCESSIBLE SOURCE CODE WITHOUT DISTRIBUTION OF MODIFICATION LICENSE
//
// Full license text: https://github.com/ChaosGroup/vray-for-houdini/blob/master/LICENSE
//

#ifndef VRAY_FOR_HOUDINI_VOP_NODE_ENVIRONMENT_H
#define VRAY_FOR_HOUDINI_VOP_NODE_ENVIRONMENT_H

#include "vop_node_base.h"


namespace VRayForHoudini {
namespace VOP {


class SettingsEnvironment:
		public NodeBase
{
public:
	SettingsEnvironment(OP_Network *parent, const char *name, OP_Operator *entry):NodeBase(parent, name, entry) {}
	virtual                  ~SettingsEnvironment() {}

	// From OP::VRayNode
	virtual PluginResult      asPluginDesc(Attrs::PluginDesc &pluginDesc, VRayExporter &exporter) VRAY_OVERRIDE;

protected:
	virtual void              setPluginType() VRAY_OVERRIDE;

};

} // namespace VOP
} // namespace VRayForHoudini

#endif // VRAY_FOR_HOUDINI_VOP_NODE_ENVIRONMENT_H
