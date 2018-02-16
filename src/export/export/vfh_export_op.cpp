//
// Copyright (c) 2015-2017, Chaos Software Ltd
//
// V-Ray For Houdini
//
// ACCESSIBLE SOURCE CODE WITHOUT DISTRIBUTION OF MODIFICATION LICENSE
//
// Full license text:
//   https://github.com/ChaosGroup/vray-for-houdini/blob/master/LICENSE
//

#include <QFile>

#include "vfh_exporter.h"
#include "vfh_attr_utils.h"

#include <COP2/COP2_Node.h>
#include <TIL/TIL_Raster.h>

using namespace VRayForHoudini;
using namespace Attrs;

/// Name format for autogenerated BitmapBuffer plugin.
static boost::format fmtHash("BitmapBuffer|%i");
static boost::format fmtPrefix("%s|%s");

/// Taken from: https://www.sidefx.com/docs/hdk/_h_d_k__data_flow__c_o_p.html
static TIL_Raster *getImageFromCop(COP2_Node &copNode, double time, const char *pname = "C")
{
	TIL_Raster *image = NULL;

	short key;
	const OP_ERROR err = copNode.open(key);

	if (err == UT_ERROR_NONE) {
		const TIL_Sequence *seq = copNode.getSequenceInfo();
		if (seq) {
			const TIL_Plane *plane = seq->getPlane(pname);

			int xres = 0;
			int yres = 0;
			seq->getRes(xres, yres);

			if (plane) {
				image = new TIL_Raster(PACK_RGBA, PXL_FLOAT32, xres, yres);

				if (seq->getImageIndex(time) == -1) {
					// out of frame range - black frame
					float black[4] = {0, 0, 0, 0};
					image->clearNormal(black);
				}
				else {
					OP_Context context(time);
					context.setXres(xres);
					context.setYres(yres);

					if (!copNode.cookToRaster(image, context, plane)) {
						delete image;
						image = NULL;
					}
				}
			}
		}
	}

	// Must be called even if open() failed.
	copNode.close(key);

	return image;
}

int VRayExporter::fillCopNodeBitmapBuffer(COP2_Node &copNode, Attrs::PluginDesc &rawBitmapBuffer)
{
	QScopedPointer<TIL_Raster> raster(getImageFromCop(copNode, getContext().getTime()));
	if (!raster)
		return 0;

	const int numPixels = raster->getNumPixels();
	if (!numPixels)
		return 0;

	const int numComponents = 4;
	const int bytesPerComponent = 4;
	const int pixelFormat = 1; // Float RGBA

	const int numPixelBytes = numPixels * numComponents * bytesPerComponent;
	const int numInts = numPixelBytes / sizeof(int);

	VRay::VUtils::IntRefList pixels(numInts);
	vutils_memcpy(pixels.get(), raster->getPixels(), numPixelBytes);

	rawBitmapBuffer.addAttribute(PluginAttr("pixels", pixels));
	rawBitmapBuffer.addAttribute(PluginAttr("pixels_type", pixelFormat));
	rawBitmapBuffer.addAttribute(PluginAttr("width", raster->getXres()));
	rawBitmapBuffer.addAttribute(PluginAttr("height", raster->getYres()));

	return 1;
}

VRay::Plugin VRayExporter::exportCopNodeBitmapBuffer(COP2_Node &copNode)
{
	VRay::Plugin res;

	Attrs::PluginDesc rawBitmapBuffer(getPluginName(&copNode, "RawBitmapBuffer"), "RawBitmapBuffer");
	if (fillCopNodeBitmapBuffer(copNode, rawBitmapBuffer)) {
		res = exportPlugin(rawBitmapBuffer);
	}

	return res;
}

void VRayExporter::fillDefaultMappingDesc(DefaultMappingType mappingType, Attrs::PluginDesc &uvwgenDesc)
{
	switch (mappingType) {
		case defaultMappingChannel: {
			uvwgenDesc.pluginID = "UVWGenChannel";
			uvwgenDesc.addAttribute(PluginAttr("uvw_channel", 0));
			break;
		}
		case defaultMappingChannelName: {
			uvwgenDesc.pluginID = "UVWGenMayaPlace2dTexture";
			uvwgenDesc.addAttribute(PluginAttr("uv_set_name", "uv"));
			break;
		}
		case defaultMappingSpherical: {
			VRay::Matrix uvwTm(1);
			VUtils::swap(uvwTm[1], uvwTm[2]);
			uvwTm[2].y = -uvwTm[2].y;

			uvwgenDesc.pluginID = "UVWGenEnvironment";
			uvwgenDesc.addAttribute(PluginAttr("mapping_type", "spherical"));
			uvwgenDesc.addAttribute(PluginAttr("uvw_matrix", uvwTm));
			break;
		}
		case defaultMappingTriPlanar: {
			uvwgenDesc.pluginID = "UVWGenProjection";
			uvwgenDesc.addAttribute(PluginAttr("type", 6));
			uvwgenDesc.addAttribute(PluginAttr("object_space", true));
			break;
		}
		default:
			break;
	}
}

VRay::Plugin VRayExporter::exportCopNodeWithDefaultMapping(COP2_Node &copNode, DefaultMappingType mappingType)
{
	VRay::Plugin res;

	const VRay::Plugin bitmapBuffer = exportCopNodeBitmapBuffer(copNode);
	if (bitmapBuffer) {
		Attrs::PluginDesc uvwgenDesc;
		uvwgenDesc.pluginName = str(fmtPrefix % "DefaultMapping" % bitmapBuffer.getName()),
		fillDefaultMappingDesc(mappingType, uvwgenDesc);

		// Bitmap data needs flipping for some reason.
		VRay::Matrix uvwTm(1);
		uvwTm.v1.set(0.0f, -1.0f, 0.0f);

		VRay::Plugin uvwgen;

		switch (mappingType) {
			case defaultMappingChannel: {
				uvwgenDesc.pluginName = getPluginName(&copNode, "UVWGenChannel");
				uvwgenDesc.addAttribute(PluginAttr("uvw_transform", VRay::Transform(uvwTm, VRay::Vector(0.0f, 0.0f, 0.0f))));
				uvwgen = exportPlugin(uvwgenDesc);
				break;
			}
			case defaultMappingChannelName: {
				uvwgenDesc.pluginName = getPluginName(&copNode, "UVWGenMayaPlace2dTexture");
				uvwgenDesc.addAttribute(PluginAttr("mirror_v", true));
				uvwgen = exportPlugin(uvwgenDesc);
				break;
			}
			case defaultMappingSpherical: {
				uvwgenDesc.pluginName = getPluginName(&copNode, "UVWGenEnvironment");
				uvwgenDesc.addAttribute(PluginAttr("uvw_matrix", uvwTm));
				uvwgen = exportPlugin(uvwgenDesc);
				break;
			}
			default:
				break;
		}

		if (uvwgen) {
			Attrs::PluginDesc texBitmapDesc(getPluginName(&copNode, "TexBitmap"),
											"TexBitmap");
			texBitmapDesc.addAttribute(PluginAttr("bitmap", bitmapBuffer));
			texBitmapDesc.addAttribute(PluginAttr("uvwgen", uvwgen));

			res = exportPlugin(texBitmapDesc);
		}
	}

	return res;
}

VRay::Plugin VRayExporter::exportFileTextureBitmapBuffer(const UT_String &filePath)
{
	Attrs::PluginDesc bitmapBufferDesc(str(fmtHash % VUtils::hashlittle(filePath.buffer(), filePath.length())),
									   "BitmapBuffer");
	bitmapBufferDesc.addAttribute(PluginAttr("color_space", 2));// 0 - linear, 1 - gamma corrected, 2 - sRGB
	bitmapBufferDesc.addAttribute(PluginAttr("file", filePath.toStdString()));

	return exportPlugin(bitmapBufferDesc);
}

VRay::Plugin VRayExporter::exportFileTextureWithDefaultMapping(const UT_String &filePath, DefaultMappingType mappingType)
{
	VRay::Plugin res;

	const VRay::Plugin bitmapBuffer = exportFileTextureBitmapBuffer(filePath);
	if (bitmapBuffer) {
		Attrs::PluginDesc uvwgenDesc;
		uvwgenDesc.pluginName = str(fmtPrefix % "DefaultMapping" % bitmapBuffer.getName()),
		fillDefaultMappingDesc(mappingType, uvwgenDesc);

		const VRay::Plugin uvwgen = exportPlugin(uvwgenDesc);
		if (uvwgen) {
			Attrs::PluginDesc texBitmapDesc(str(fmtPrefix % "TexBitmap" % bitmapBuffer.getName()),
											"TexBitmap");
			texBitmapDesc.addAttribute(PluginAttr("bitmap", bitmapBuffer));
			texBitmapDesc.addAttribute(PluginAttr("uvwgen", uvwgen));

			res = exportPlugin(texBitmapDesc);
		}
	}

	return res;
}

VRay::Plugin VRayExporter::exportNodeFromPathWithDefaultMapping(const UT_String &path, DefaultMappingType mappingType)
{
	VRay::Plugin res;

	if (path.startsWith(OPREF_PREFIX)) {
		OP_Node *opNode = getOpNodeFromPath(path, getContext().getTime());
		if (opNode) {
			COP2_Node *copNode = opNode->castToCOP2Node();
			VOP_Node *vopNode = opNode->castToVOPNode();
			if (copNode) {
				res = exportCopNodeWithDefaultMapping(*copNode, mappingType);
			}
			else if (vopNode) {
				res = exportVop(vopNode);
			}
		}
	}
	else {
		QFile fileChecker(path.buffer());
		if (fileChecker.exists()) {
			res = exportFileTextureWithDefaultMapping(path, mappingType);
		}
	}

	return res;
}

VRay::Plugin VRayExporter::exportNodeFromPath(const UT_String &path)
{
	return exportNodeFromPathWithDefaultMapping(path, defaultMappingTriPlanar);
}
