//
// Copyright (c) 2015-2018, Chaos Software Ltd
//
// V-Ray For Houdini
//
// ACCESSIBLE SOURCE CODE WITHOUT DISTRIBUTION OF MODIFICATION LICENSE
//
// Full license text:
//  https://github.com/ChaosGroup/vray-for-houdini/blob/master/LICENSE
//

#include "vfh_defines.h"
#include "vfh_vray.h"
#include "vfh_log.h"
#include "vfh_vray_instances.h"
#include "vfh_hou_utils.h"
#include "vfh_includes.h"

#include <QSharedMemory>
#include <QApplication>
#include <QWidget>
#include <QProcess>

using namespace VRayForHoudini;

const QString &memKey = QString("VRayInstances%1").arg(QCoreApplication::applicationPid());
static const int maxInstances = 256;
static QRegExp cssRgbMatch("(\\d+)");

struct InstancesStorage {
	InstancesStorage()
		: vrayInit(nullptr)
	{}

	VRay::VRayInit* vrayInit;
	VRay::VRayRenderer* vrayInstances[maxInstances];
};

static const int memSize = sizeof(InstancesStorage);

static QSharedMemory vrayInstances(memKey);

static QColor getSelectionColor(const QString &styleSheet)
{
	QColor selectionColor;

	// Find the first occurence of "selection-background-color: rgb(184, 133, 32);"
	QStringList lines = styleSheet.split('\n');
	for (QString line : lines) {
		if (line.contains("selection-background-color")) {
			QStringList colors;
			int pos = 0;

			while ((pos = cssRgbMatch.indexIn(line, pos)) != -1) {
				colors << cssRgbMatch.cap(1);
				pos += cssRgbMatch.matchedLength();
			}

			selectionColor.setRed(colors[0].toInt());
			selectionColor.setGreen(colors[1].toInt());
			selectionColor.setBlue(colors[2].toInt());

			break;
		}
	}

	return selectionColor;
}

/// VFB will take colors from QApplication::palette(),
/// but Houdini's real palette is not stored there.
/// Must be called before VRay::VRayInit().
static void initVFBTheme()
{
#ifdef HDK_16_5
	const float uiScale = HOU::getUiScaling();
	if (uiScale > 1.0f) {
		qputenv("VRAY_VFB_UI_SCALE", QString("%1").arg(uiScale).toLocal8Bit());
	}
#endif

	QWidget *mainWindow = HOU::getMainQtWindow();
	if (mainWindow) {
		QPalette houdiniPalette = mainWindow->palette();

		// Pressed button color is incorrect in palette...
		QString styleSheet = mainWindow->styleSheet();

		houdiniPalette.setBrush(QPalette::Highlight,
								QBrush(getSelectionColor(styleSheet)));

		QApplication::setPalette(houdiniPalette);

		QApplication::setFont(mainWindow->font());
	}
}

static void clearSharedMemory()
{
	::memset(vrayInstances.data(), 0, memSize);
}

static int getCreateSharedMemory()
{
	if (vrayInstances.isAttached())
		return 1;

	if (vrayInstances.create(memSize)) {
		Log::getLog().debug("InstancesStorage: \"%s\"", qPrintable(memKey));
		clearSharedMemory();
	}
	else {
		Log::getLog().debug("InstancesStorage: %s",
							qPrintable(vrayInstances.errorString()));
		switch (vrayInstances.error()) {
			case QSharedMemory::NoError:
			case QSharedMemory::AlreadyExists: {
				vrayInstances.attach();
				break;
			}
			default: {
				Log::getLog().error("InstancesStorage error: %s",
									qPrintable(vrayInstances.errorString()));
				return 0;
			}
		}
	}

	return 1;
}

VRay::RendererOptions VRayForHoudini::getDefaultOptions(int initVFB)
{
	const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

	VRay::RendererOptions options;
	options.enableFrameBuffer = initVFB;
	options.showFrameBuffer = false;
	options.useDefaultVfbTheme = false;
	options.vfbDrawStyle = VRay::RendererOptions::ThemeStyleMaya;
	options.pluginLibraryPath = env.value("VRAY_FOR_HOUDINI_PLUGINS", "").toStdString();
	options.gpuPluginPath = env.value("VRAY_APPSDK", "").toStdString();
	options.gpuPluginPath.append("/bin");

	return options;
}

int VRayForHoudini::newVRayInit()
{
	Log::getLog().debug("newVRayInit()");

	getCreateSharedMemory();

	vassert(vrayInstances.isAttached());

	int addDummyRenderer = false;
	int vrayInitExist = false;

	const bool hasUI = HOU::isUIAvailable();

	if (vrayInstances.lock()) {
		InstancesStorage *is = reinterpret_cast<InstancesStorage*>(vrayInstances.data());
		if (is->vrayInit) {
			vrayInitExist = true;
		}
		else {
			if (hasUI) {
				initVFBTheme();
			}

			try {
				is->vrayInit = new VRay::VRayInit(hasUI);
			}
			catch (VRay::VRayException &e) {
				Log::getLog().error("Error initializing V-Ray AppSDK library:\n%s",
									e.what());
			}

			if (is->vrayInit) {
				addDummyRenderer = true;

				const VRay::APIVersion appSdkVer = VRay::getAPIVersion();

				Log::getLog().info("Using V-Ray AppSDK %u.%02u.%02u",
				                   appSdkVer.major, appSdkVer.minor, appSdkVer.revision);
			}
		}

		vrayInstances.unlock();
	}

	if (addDummyRenderer) {
		VRay::VRayRenderer *instance = nullptr;

		try {
			const VRay::RendererOptions options = getDefaultOptions(false);
			instance = newVRayRenderer(options);
		}
		catch (VRay::VRayException &e) {
			Log::getLog().error("Error initializing VRay::VRayRenderer instance:\n%s",
								e.what());
		}

		if (instance) {
			vrayInitExist = true;

			Log::getLog().info("Using V-Ray %s", VRay::getVRayVersionDetails());
		}
	}

	return vrayInitExist;
}

void VRayForHoudini::deleteVRayInit()
{
	Log::getLog().debug("deleteVRayInit()");

	if (!vrayInstances.isAttached()) {
		if (!vrayInstances.attach())
			return;
	}

	if (vrayInstances.lock()) {
		InstancesStorage *is = reinterpret_cast<InstancesStorage*>(vrayInstances.data());

		for (int i = 0; i < maxInstances; ++i) {
			VRay::VRayRenderer *vrayInstance = is->vrayInstances[i];
			if (vrayInstance) {
				Log::getLog().debug("Deleting VRayRenderer: 0x%X", vrayInstance);
				FreePtr(vrayInstance);
				is->vrayInstances[i] = nullptr;
			}
		}

		FreePtr(is->vrayInit);

		vrayInstances.unlock();
	}

	vrayInstances.detach();
}

VRay::VRayRenderer* VRayForHoudini::newVRayRenderer(const VRay::RendererOptions &options)
{
	Log::getLog().debug("newVRayRenderer()");

	vassert(vrayInstances.isAttached());

	VRay::VRayRenderer *instance = nullptr;

	if (vrayInstances.lock()) {
		InstancesStorage *is = reinterpret_cast<InstancesStorage*>(vrayInstances.data());

		try {
			instance = new VRay::VRayRenderer(options);
		}
		catch (VRay::VRayException &e) {
			Log::getLog().error("Error initializing VRay::VRayRenderer instance:\n%s",
								e.what());
		}

		if (instance) {
			instance->setAutoCommit(true);

			for (int i = 0; i < maxInstances; ++i) {
				VRay::VRayRenderer* &vrayInstance = is->vrayInstances[i];
				if (vrayInstance == nullptr) {
					vrayInstance = reinterpret_cast<VRay::VRayRenderer*>(instance);
					break;
				}
			}
		}

		vrayInstances.unlock();
	}

	return instance;
}

void VRayForHoudini::deleteVRayRenderer(VRay::VRayRenderer* &instance)
{
	Log::getLog().debug("deleteVRayRenderer()");

	if (!instance)
		return;

	if (!vrayInstances.isAttached()) {
		vrayInstances.attach();
	}

	vassert(vrayInstances.isAttached());

	if (vrayInstances.lock()) {
		InstancesStorage *is = reinterpret_cast<InstancesStorage*>(vrayInstances.data());

		for (int i = 0; i < maxInstances; ++i) {
			VRay::VRayRenderer* &vrayInstance = is->vrayInstances[i];
			if (vrayInstance == instance) {
				Log::getLog().debug("Deleting VRayRenderer: 0x%X", vrayInstance);
				FreePtr(vrayInstance);
				instance = nullptr;
				break;
			}
		}

		vrayInstances.unlock();
	}
}

void VRayForHoudini::dumpSharedMemory()
{
	Log::getLog().debug("dumpSharedMemory()");

	if (!vrayInstances.isAttached())
		return;

	if (vrayInstances.lock()) {
		InstancesStorage *is = reinterpret_cast<InstancesStorage*>(vrayInstances.data());

		Log::getLog().debug("VRayInit: 0x%X", is->vrayInit);

		for (int i = 0; i < maxInstances; ++i) {
			if (is->vrayInstances[i]) {
				Log::getLog().debug("  VRayRenderer: 0x%X", is->vrayInstances[i]);
			}
		}

		vrayInstances.unlock();
	}
}
