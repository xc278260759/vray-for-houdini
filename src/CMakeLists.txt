#
# Copyright (c) 2015-2018, Chaos Software Ltd
#
# V-Ray For Houdini
#
# ACCESSIBLE SOURCE CODE WITHOUT DISTRIBUTION OF MODIFICATION LICENSE
#
# Full license text: https://github.com/ChaosGroup/vray-for-houdini/blob/master/LICENSE
#

set(SDK_PATH "" CACHE STRING "SDK root qpath")

include_directories(.)
include_directories(io)
include_directories(op)
include_directories(gu)
include_directories(gu/options)
include_directories(sop)
include_directories(vop)
include_directories(rop)
include_directories(lib)
include_directories(vop/brdf)
include_directories(vop/material)
include_directories(vop/texture)
include_directories(vop/uvwgen)
include_directories(vop/meta)
include_directories(export)
include_directories(export/utils)
include_directories(export/export)
include_directories(utils)

set(EXPORTER_SOURCES
	export/export/vfh_export_vop.cpp
	export/export/vfh_export_geom.cpp
	export/export/vfh_export_hair.cpp
	export/export/vfh_export_mesh.cpp
	export/export/vfh_export_primitive.cpp
	export/export/vfh_export_vrayproxy.cpp
	export/export/vfh_export_hdk_shaders.cpp
	export/export/vfh_export_op.cpp
	export/export/vfh_export_texture.cpp
	export/export/vfh_material_override.cpp
	gu/vfh_gu_base.cpp
	gu/gu_volumegridref.cpp
	gu/gu_vrayproxyref.cpp
	gu/gu_vraysceneref.cpp
	gu/gu_geomplaneref.cpp
	gu/gu_pgyeti.cpp
	export/utils/vfh_geoutils.cpp
	export/utils/vfh_tex_utils.cpp
	export/utils/vfh_utils_vrmesh.cpp
	export/vfh_export_context.cpp
	export/vfh_export_light.cpp
	export/vfh_export_material.cpp
	export/vfh_export_object.cpp
	export/vfh_export_view.cpp
	export/vfh_exporter.cpp
	export/vfh_plugin_attrs.cpp
	export/vfh_plugin_exporter.cpp
	export/vfh_vray_cloud.cpp
	utils/vfh_gpu_device_select.cpp
	utils/vfh_error.cpp
	utils/vfh_hashes.cpp
	utils/vfh_hou_utils.cpp
	utils/vfh_log.cpp
	utils/vfh_lru_cache.hpp
	utils/vfh_op_utils.cpp
	utils/vfh_phx_utils.cpp
	utils/vfh_plugin_info.cpp
	utils/vfh_prm_def.cpp
	utils/vfh_prm_defaults.cpp
	utils/vfh_prm_globals.cpp
	utils/vfh_prm_templates.cpp
	utils/vfh_sys_utils.cpp
	utils/vfh_attr_utils.cpp
	lib/vfh_vray_instances.cpp
	op/op_node_base.cpp
	rop/vfh_rop.cpp
)

set(EXPORTER_HEADERS
	export/export/vfh_export_vop.h
	export/export/vfh_export_geom.h
	export/export/vfh_export_hair.h
	export/export/vfh_export_mesh.h
	export/export/vfh_export_primitive.h
	export/export/vfh_export_vrayproxy.h
	export/export/vfh_material_override.h
	export/utils/vfh_geoutils.h
	export/utils/vfh_tex_utils.h
	export/utils/vfh_utils_vrmesh.h
	export/vfh_export_context.h
	export/vfh_export_view.h
	export/vfh_exporter.h
	export/vfh_plugin_attrs.h
	export/vfh_plugin_exporter.h
	export/vfh_vray_cloud.h
	export/vfh_vray.h
	gu/vfh_gu_base.h
	gu/gu_volumegridref.h
	gu/gu_vrayproxyref.h
	gu/gu_vraysceneref.h
	gu/gu_geomplaneref.h
	gu/gu_pgyeti.h
	gu/vfh_gu_cache.h
	utils/vfh_gpu_device_select.h
	utils/vfh_attr_utils.h
	utils/vfh_error.h
	utils/vfh_hashes.h
	utils/vfh_hou_utils.h
	utils/vfh_log.h
	utils/vfh_op_utils.h
	utils/vfh_phx_utils.h
	utils/vfh_plugin_info.h
	utils/vfh_prm_def.h
	utils/vfh_prm_defaults.h
	utils/vfh_prm_globals.h
	utils/vfh_prm_templates.h
	utils/vfh_sys_utils.h
	lib/vfh_vray_instances.h
	op/op_node_base.h
	rop/vfh_rop.h
)

set(SOURCES
	cmd/vfh_cmd_register.cpp
	cmd/vfh_cmd_vrayproxy.cpp
	io/io_vrmesh.cpp
	io/io_vrscene.cpp
	obj/obj_node_base.cpp
	rop/rop_vrayproxyrop.cpp
	sop/sop_node_base.cpp
	sop/sop_GeomPlane.cpp
	sop/sop_PhoenixCache.cpp
	sop/sop_vrayproxy.cpp
	sop/sop_vrayscene.cpp
	sop/sop_pgyeti.cpp
	sop/vfh_vrayproxyutils.cpp
	vfh_register.cpp
	vop/brdf/vop_BRDFLayered.cpp
	vop/brdf/vop_BRDFScanned.cpp
	vop/brdf/vop_BRDFVRayMtl.cpp
	vop/brdf/vop_BRDFToonMtl.cpp
	vop/env/vop_SettingsEnvironment.cpp
	vop/material/vop_MaterialOutput.cpp
	vop/material/vop_MtlMulti.cpp
	vop/material/vop_MtlVRayMesh.cpp
	vop/material/vop_PhoenixSim.cpp
	vop/meta/vop_meta_image_file.cpp
	vop/rc/vop_rc_container.cpp
	vop/texture/vop_TexFalloff.cpp
	vop/texture/vop_TexGradRamp.cpp
	vop/texture/vop_TexLayeredMax.cpp
	vop/texture/vop_TexRemap.cpp
	vop/texture/vop_texture_output.cpp
	vop/texture/vfh_TexMulti.cpp
	vop/vop_context.cpp
	vop/vop_node_base.cpp
)

set(HEADERS
	cmd/vfh_cmd_register.h
	cmd/vfh_cmd_vrayproxy.h
	io/io_vrmesh.h
	io/io_vrscene.h
	obj/obj_node_base.h
	obj/obj_node_def.h
	rop/rop_vrayproxyrop.h
	sop/sop_GeomPlane.h
	sop/sop_node_base.h
	sop/sop_node_def.h
	sop/sop_PhoenixCache.h
	sop/sop_vrayproxy.h
	sop/sop_vrayscene.h
	sop/sop_pgyeti.h
	sop/vfh_vrayproxyutils.h
	vfh_class_utils.h
	vfh_defines.h
	vfh_includes.h
	vfh_typedefs.h
	vfh_dso_version.h
	vop/brdf/vop_brdf_def.h
	vop/brdf/vop_BRDFLayered.h
	vop/brdf/vop_BRDFScanned.h
	vop/brdf/vop_BRDFVRayMtl.h
	vop/brdf/vop_BRDFToonMtl.h
	vop/env/vop_env_def.h
	vop/env/vop_SettingsEnvironment.h
	vop/material/vop_MaterialOutput.h
	vop/material/vop_mtl_def.h
	vop/material/vop_MtlMulti.h
	vop/material/vop_MtlVRayMesh.h
	vop/material/vop_PhoenixSim.h
	vop/meta/vop_meta_def.h
	vop/meta/vop_meta_image_file.h
	vop/rc/vop_rc_container.h
	vop/rc/vop_rc_def.h
	vop/texture/vop_TexFalloff.h
	vop/texture/vop_TexGradRamp.h
	vop/texture/vop_TexLayeredMax.h
	vop/texture/vop_TexRemap.h
	vop/texture/vop_texture_def.h
	vop/texture/vop_texture_output.h
	vop/texture/vfh_TexMulti.h
	vop/uvwgen/vop_uvwgen_def.h
	vop/vop_context.h
	vop/vop_node_base.h
)

if (VRayOSL_FOUND)
	list(APPEND SOURCES vop/vop_node_osl.cpp)
	list(APPEND HEADERS vop/vop_node_osl.h)
endif()

if (WIN32)
	function(GroupSources curdir)
		file(GLOB children RELATIVE ${PROJECT_SOURCE_DIR}/${curdir} ${PROJECT_SOURCE_DIR}/${curdir}/*)
		foreach(child ${children})
			if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${curdir}/${child})
				GroupSources(${curdir}/${child})
			else()
				string(REPLACE "/" "\\" groupname ${curdir})
				string(REPLACE "src\\" "" groupname ${groupname})
				source_group(${groupname} FILES ${PROJECT_SOURCE_DIR}/${curdir}/${child})
			endif()
		endforeach()
	endfunction()
	GroupSources(src)
endif()

# *.aur preview
if (Phoenix_FOUND)
	# this will enable compilation for the code that integrates phx
	add_definitions(-DCGR_HAS_AUR)
	include_directories(${Phoenix_INCLUDES})
	link_directories(${Phoenix_LIBRARIES})
endif()

houdini_library(vray_for_houdini_exporter "${EXPORTER_SOURCES};${EXPORTER_HEADERS}")

vfh_make_moc(TARGET vray_for_houdini_exporter
	FILE_IN       ${CMAKE_CURRENT_SOURCE_DIR}/export/vfh_vray_cloud.cpp
	FILE_OUT_NAME vfh_vray_cloud.cpp.moc
)

vfh_make_moc(TARGET vray_for_houdini_exporter
	FILE_IN       ${CMAKE_CURRENT_SOURCE_DIR}/utils/vfh_gpu_device_select.cpp
	FILE_OUT_NAME vfh_gpu_device_select.cpp.moc
)

houdini_plugin(${PROJECT_NAME} "${SOURCES};${HEADERS}")

# Generate GU intrinsics.
#
function(vfh_generate_gu_options _libTarget _dsoTarget)
	set(FILE_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
	set(GU_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/gu/options/vfh_gu_options.py)

	file(GLOB JSON_INPUT "gu/options/json/*.json")

	foreach(jsonFile ${JSON_INPUT})
		get_filename_component(CLASS_NAME ${jsonFile} NAME_WE)
		set(FILE_OUT_NAME "vfh_${CLASS_NAME}Base.cpp")
		set(FILE_OUT_NAME_H "vfh_${CLASS_NAME}Base.h")

		execute_process(
			COMMAND
				${PYTHON_BIN} ${GU_SCRIPT} --hdk=${HOUDINI_VERSION} --jsonFile=${jsonFile} --outDir=${FILE_OUT_DIR}
			WORKING_DIRECTORY
				${FILE_OUT_DIR}
		)

		add_custom_command(
			OUTPUT
				${FILE_OUT_DIR}/${FILE_OUT_NAME}
			COMMAND
				${PYTHON_BIN} ${GU_SCRIPT} --hdk=${HOUDINI_VERSION} --jsonFile=${jsonFile} --outDir=${FILE_OUT_DIR}
			DEPENDS
				${GU_SCRIPT}
				${jsonFile}
			WORKING_DIRECTORY
				${FILE_OUT_DIR}
			COMMENT
				"Generating options for ${CLASS_NAME}..."
			VERBATIM
		)

		target_sources(${_libTarget}
			PRIVATE
				${FILE_OUT_DIR}/${FILE_OUT_NAME}
				${FILE_OUT_DIR}/${FILE_OUT_NAME_H}
				${jsonFile}
		)

		source_group("gu\\options\\json\\cpp"
			FILES
				${FILE_OUT_DIR}/${FILE_OUT_NAME}
				${FILE_OUT_DIR}/${FILE_OUT_NAME_H}
		)
	endforeach()

	target_include_directories(${_libTarget} PRIVATE ${FILE_OUT_DIR})
	target_include_directories(${_dsoTarget} PRIVATE ${FILE_OUT_DIR})
endfunction()

vfh_generate_gu_options(vray_for_houdini_exporter vray_for_houdini)

target_link_libraries(${PROJECT_NAME} vray_for_houdini_exporter)


link_with_vray_sdk(${PROJECT_NAME})
link_with_vray(${PROJECT_NAME})
link_with_vray_appsdk(${PROJECT_NAME})
link_with_boost(${PROJECT_NAME})
link_with_vray_osl(${PROJECT_NAME})

if (Phoenix_FOUND)
	target_link_libraries(${PROJECT_NAME} ${Phoenix_LIBS})
endif()

add_definitions(-DCGR_GIT_HASH=${SRC_GIT_HASH})

function(vfh_install_common _location)
	# Copy Phoenix cache file loader library
	if (Phoenix_FOUND)
		install(
			FILES
				${CGR_PHOENIX_SHARED}
				${CGR_PHOENIX_SHARED_VDB}
				${CGR_PHOENIX_SHARED_F3D}
			DESTINATION
				${_location}/vfh_home/libs
			PERMISSIONS
				OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	endif()

	# Copy plugin
	cgr_install_runtime(${PROJECT_NAME} ${_location}/vfh_home/dso)

	# Copy config
	install(DIRECTORY ${CMAKE_SOURCE_DIR}/deploy/vfh_home
			DESTINATION ${_location}
			PATTERN "otls/*" EXCLUDE
			PATTERN "otls" EXCLUDE
			PATTERN "*.pyc" EXCLUDE
	)

	if (WITH_SOHO)
		install(DIRECTORY ${CMAKE_SOURCE_DIR}/deploy/vfh_home/otls
				DESTINATION ${_location}/vfh_home
				PATTERN "backup/*" EXCLUDE
				PATTERN "backup" EXCLUDE
		)
	endif()
endfunction(vfh_install_common)

function(vfh_update_houdini_env)
	set(HOUDINI_ENV_FILEPATH ${HOUDINI_HOME_PATH}/houdini.env)

	set(VFH_ENV_MATCH "# V-Ray for Houdini Begin(.*)# V-Ray for Houdini End")
	set(VFH_ENV "# V-Ray for Houdini Begin
VFH_BUILD_ROOT=\"${CMAKE_BINARY_DIR}\"
VFH_SDK_ROOT=\"${SDK_PATH}\"
VFH_SRC_ROOT=\"${CMAKE_SOURCE_DIR}\"

VRAY_APPSDK=\"${_appsdk_root}\"
VRAY_OSL_PATH=\"$\{VRAY_APPSDK\}/bin\"
VRAY_UI_DS_PATH=\"$\{VFH_SRC_ROOT\}/deploy/ui\"
VRAY_FOR_HOUDINI_AURA_LOADERS=\"$\{VFH_SDK_ROOT\}/phxsdk/qt5/vc14/lib\"

VFH_PYTHONPATH=\"$\{VFH_BUILD_ROOT\}/python;&\"
VFH_PATH=\"$\{VFH_BUILD_ROOT\}/bin;$\{VRAY_APPSDK\}/bin;$\{VRAY_FOR_HOUDINI_AURA_LOADERS\}\"
VFH_HOUDINI_PATH=\"$\{VFH_SRC_ROOT\}/deploy/vfh_home\"
VFH_HOUDINI_DSO_PATH=\"$\{VFH_BUILD_ROOT\}/dso\"

PYTHONPATH=\"$\{VFH_PYTHONPATH\};$\{PYTHONPATH\};&\"
PATH=\"$\{VFH_PATH\};$\{PATH\}\"

HOUDINI_PATH=\"$\{VFH_HOUDINI_PATH\};&\"
HOUDINI_DSO_PATH=\"$\{VFH_HOUDINI_DSO_PATH\};$\{HOUDINI_DSO_PATH\};&\"

HOUDINI13_VOLUME_COMPATIBILITY=1
HDF5_DISABLE_VERSION_CHECK=1
# V-Ray for Houdini End"
)
	if(EXISTS ${HOUDINI_ENV_FILEPATH})
		file(READ ${HOUDINI_ENV_FILEPATH} HOUDINI_ENV)
	endif()

	if(NOT HOUDINI_ENV)
		file(WRITE ${HOUDINI_ENV_FILEPATH} ${VFH_ENV})
	else()
		string(REGEX MATCH ${VFH_ENV_MATCH} HAS_VFH_ENV "${HOUDINI_ENV}")
		if(NOT HAS_VFH_ENV)
			file(APPEND ${HOUDINI_ENV_FILEPATH} "${VFH_ENV}")
		else()
			string(REGEX REPLACE ${VFH_ENV_MATCH}
			       "${VFH_ENV}"
			       VFH_ENV_NEW
			       "${HOUDINI_ENV}"
			)

			if(NOT WIN32)
				string(REPLACE ";" ":" VFH_ENV_NEW "${VFH_ENV_NEW}")
			endif()

			file(WRITE ${HOUDINI_ENV_FILEPATH} "${VFH_ENV_NEW}")
		endif()
	endif()
endfunction()

if(INSTALL_RELEASE)
	vfh_install_common(${INSTALL_RELEASE_PATH})

	# Generate launcher file
	vfh_generate_launcher(RELEASE BATCH DESTINATION ${INSTALL_RELEASE_PATH})
	vfh_generate_launcher(RELEASE       DESTINATION ${INSTALL_RELEASE_PATH})

	# Copy ui
	install(DIRECTORY   ${CMAKE_SOURCE_DIR}/deploy/ui
			DESTINATION ${INSTALL_RELEASE_PATH})

	# Copy AppSDK
	if(HOUDINI_QT_VERSION VERSION_EQUAL 4)
		set(EXCLUDE_LIBVRAY "*${CMAKE_SHARED_LIBRARY_PREFIX}vray${CMAKE_SHARED_LIBRARY_SUFFIX}")
	else()
		set(EXCLUDE_LIBVRAY "*${CMAKE_SHARED_LIBRARY_PREFIX}vray_qt4${CMAKE_SHARED_LIBRARY_SUFFIX}")
	endif()

	install(DIRECTORY ${APPSDK_ROOT}/bin
			DESTINATION ${INSTALL_RELEASE_PATH}/appsdk
			PATTERN ${EXCLUDE_LIBVRAY} EXCLUDE
	)

	# Example houdini.env file
	install(FILES       ${CMAKE_SOURCE_DIR}/deploy/houdini.env
			DESTINATION ${INSTALL_RELEASE_PATH})

	# Default libvray.dll is Qt5
	if(HOUDINI_QT_VERSION VERSION_EQUAL 4)
		set(VRAYDSO_QT4 ${APPSDK_ROOT}/bin/${CMAKE_SHARED_LIBRARY_PREFIX}vray_qt4${CMAKE_SHARED_LIBRARY_SUFFIX})
		# If doesn't exist - it's already Qt 4
		if(EXISTS ${VRAYDSO_QT4})
			install(FILES ${VRAYDSO_QT4}
				DESTINATION
					${INSTALL_RELEASE_PATH}/appsdk/bin
				RENAME
					${CMAKE_SHARED_LIBRARY_PREFIX}vray${CMAKE_SHARED_LIBRARY_SUFFIX}
			)
		endif()
	endif()

	# Archive the installation directory
	if(INSTALL_RELEASE_PACK)
		set(VFH_PACK ${CMAKE_BINARY_DIR}/vfh_pack.cmake)
		configure_file(${CMAKE_SOURCE_DIR}/cmake/vfh_pack.cmake.in ${VFH_PACK})
		install(SCRIPT ${VFH_PACK})
	endif()
else()
	cgr_install_runtime(${PROJECT_NAME} ${VFH_DSO_DIRPATH})
	vfh_update_houdini_env()
endif()
