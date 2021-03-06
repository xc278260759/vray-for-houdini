//
// Copyright (c) 2015-2018, Chaos Software Ltd
//
// V-Ray For Houdini Python IPR Module
//
// ACCESSIBLE SOURCE CODE WITHOUT DISTRIBUTION OF MODIFICATION LICENSE
//
// Full license text: https://github.com/ChaosGroup/vray-for-houdini/blob/master/LICENSE
//

#include "vfh_defines.h"
#include "vfh_log.h"
#include "vfh_ipr_viewer.h"
#include "vfh_ipr_imdisplay_viewer.h"

#include <QtCore>

#include <UT/UT_WritePipe.h>

using namespace VRayForHoudini;

/// A typedef for render elements array.
typedef std::vector<VRay::RenderElement> RenderElementsList;

static const QString colorPass("C");
static ImdisplayThread imdisplayThread;

static void addImages(VRay::VRayRenderer &renderer, VRay::VRayImage *image, int x, int y)
{
	vassert(image);

	int width = 0;
	int height = 0;
	if (!image->getSize(width, height))
		return;

	VRay::ImageRegion region(x, y);
	region.setWidth(width);
	region.setHeight(height);

	PlaneImages planes;

	TileImage *rgbaImage = new TileImage(image, colorPass);
	rgbaImage->setRegion(region);
	planes.append(rgbaImage);

	const VRay::RenderElements &reMan = renderer.getRenderElements();
	const RenderElementsList &reList = reMan.getAll(VRay::RenderElement::NONE);
	for (const VRay::RenderElement &re : reList) {
		TileImage *renderElementImage = new TileImage(re.getImage(&region), re.getName().c_str());
		renderElementImage->setRegion(region);

		planes.append(renderElementImage);
	}

	imdisplayThread.add(new TileImageMessage(planes));
}

void VRayForHoudini::onBucketReady(VRay::VRayRenderer &renderer, int x, int y, const char*, VRay::VRayImage *image, VRay::ImagePassType pass, double, void*)
{
	addImages(renderer, image, x, y);
}

void VRayForHoudini::onProgressiveImageUpdated(VRay::VRayRenderer &renderer,
                                               VRay::VRayImage *image,
                                               unsigned long long changeIndex,
                                               VRay::ImagePassType pass,
                                               double instant,
                                               void *userData)
{
	addImages(renderer, image, 0, 0);
}

void VRayForHoudini::onStateChanged(VRay::VRayRenderer &renderer, VRay::RendererState oldState, VRay::RendererState newState, double instant, void *userData)
{
	if (newState == VRay::IDLE_DONE) {
		addImages(renderer, renderer.getImage(), 0, 0);
	}
}

void VRayForHoudini::onProgress(VRay::VRayRenderer& /*renderer*/, const char *msg, int elementNumber, int elementsCount, double /*instant*/, void* /*data*/)
{
	const QString text(QString(msg).simplified());

	const int percentage = 100.0f * float(elementNumber) / float(elementsCount);

	TileProgressMessage *progMsg = new TileProgressMessage;
	progMsg->message = text;
	progMsg->percentage = percentage;

	imdisplayThread.add(progMsg);
}

ImdisplayThread & VRayForHoudini::getImdisplay()
{
	return imdisplayThread;
}

void VRayForHoudini::initImdisplay(VRay::VRayRenderer &renderer, const char *ropName)
{
	Log::getLog().debug("initImdisplay()");

	ImageHeaderMessage *imageHeaderMsg = new ImageHeaderMessage();
	renderer.getImageSize(imageHeaderMsg->imageWidth, imageHeaderMsg->imageHeight);
	imageHeaderMsg->planeNames.append(colorPass);
	imageHeaderMsg->ropName = ropName;

	const VRay::RenderElements &reMan = renderer.getRenderElements();
	for (const VRay::RenderElement &re : reMan.getAll(VRay::RenderElement::NONE)) {
		imageHeaderMsg->planeNames.append(re.getName().c_str());
	}

	imdisplayThread.add(imageHeaderMsg);
}
