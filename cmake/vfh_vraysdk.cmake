#
# Copyright (c) 2015, Chaos Software Ltd
#
# V-Ray For Houdini
#
# Andrei Izrantcev <andrei.izrantcev@chaosgroup.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

set(_maya_versons "2017;2016;2015;2014")
foreach(_maya_version ${_maya_versons})
	if(WIN32)
		set(_vray_for_maya_root "C:/Program Files/Chaos Group/V-Ray/Maya ${_maya_version} for x64")
	elseif(APPLE)
		set(_vray_for_maya_root "/Applications/ChaosGroup/V-Ray/Maya${_maya_version}")
	else()
		set(_vray_for_maya_root "/usr/ChaosGroup/V-Ray/Maya${_maya_version}-x64")
	endif()

	if(EXISTS ${_vray_for_maya_root})
		if(WIN32)
			set(_vray_os_subdir       "x64")
			set(_vray_compiler_subdir "vc101")
			if (${_maya_version} GREATER 2014)
				set(_vray_compiler_subdir "vc11")
			endif()
		elseif(APPLE)
			if(${_maya_version} GREATER 2015)
				set(_vray_os_subdir   "mavericks_x64")
			else()
				set(_vray_os_subdir   "mountain_lion_x64")
			endif()
			set(_vray_compiler_subdir "gcc-4.2")
		else()
			set(_vray_os_subdir       "linux_x64")
			set(_vray_compiler_subdir "gcc-4.4")
		endif()

		set(vray_for_maya_incpaths
			${_vray_for_maya_root}/include
		)
		set(vray_for_maya_libpaths
			${_vray_for_maya_root}/lib/${_vray_os_subdir}
			${_vray_for_maya_root}/lib/${_vray_os_subdir}/${_vray_compiler_subdir}
		)
		break()
	endif()
endforeach()

set(CGR_VRAYSDK_INCPATH "" CACHE PATH "V-Ray SDK include path")
set(CGR_VRAYSDK_LIBPATH "" CACHE PATH "V-Ray SDK library path")

set(VRAYSDK_INCPATH "" CACHE PATH "")
set(VRAYSDK_LIBPATH "" CACHE PATH "")

if(NOT CGR_VRAYSDK_INCPATH STREQUAL "")
	set(VRAYSDK_INCPATH ${CGR_VRAYSDK_INCPATH} CACHE PATH "" FORCE)
else()
	set(VRAYSDK_INCPATH ${vray_for_maya_incpaths} CACHE PATH "" FORCE)
endif()

if(NOT CGR_VRAYSDK_LIBPATH STREQUAL "")
	set(VRAYSDK_LIBPATH ${CGR_VRAYSDK_LIBPATH} CACHE PATH "" FORCE)
else()
	set(VRAYSDK_LIBPATH ${vray_for_maya_libpaths} CACHE PATH "" FORCE)
endif()

macro(use_vray_sdk)
	if(WIN32)
		# Both V-Ray SDK and HDK defines some basic types,
		# tell V-Ray SDK not to define them
		add_definitions(
			-DVUTILS_NOT_DEFINE_INT8
			-DVUTILS_NOT_DEFINE_UINT8
		)
	endif()

	message(STATUS "Using V-Ray SDK include path: ${VRAYSDK_INCPATH}")
	message(STATUS "Using V-Ray SDK library path: ${VRAYSDK_LIBPATH}")

	include_directories(${VRAYSDK_INCPATH})
	link_directories(${VRAYSDK_LIBPATH})
endmacro()


macro(link_with_vray_sdk _name)
	set(VRAY_SDK_LIBS
		alembic_s
		bmputils_s
		meshes_s
		meshinfosubdivider_s
		osdCPU_s
		openexr_s
		pimglib_s
		plugman_s
		putils_s
		# treeparser_s
		# pll_s
		vutils_s
	)
	list(APPEND VRAY_SDK_LIBS
		jpeg_s
		libpng_s
		tiff_s
	)
	if(WIN32)
		list(APPEND VRAY_SDK_LIBS
			zlib_s
			QtCore4
		)
	endif()
	target_link_libraries(${_name} ${VRAY_SDK_LIBS})
endmacro()


macro(link_with_vray _name)
	set(VRAY_LIBS
		cgauth
		vray
	)
	target_link_libraries(${_name} ${VRAY_LIBS})
endmacro()
