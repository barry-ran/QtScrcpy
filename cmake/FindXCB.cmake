# SPDX-FileCopyrightText: 2011 Fredrik Höglund <fredrik@kde.org>
# SPDX-FileCopyrightText: 2013 Martin Gräßlin <mgraesslin@kde.org>
# SPDX-FileCopyrightText: 2014-2015 Alex Merry <alex.merry@kde.org>
#
# SPDX-License-Identifier: BSD-3-Clause

#[=======================================================================[.rst:
FindXCB
-------

Try to find XCB.

This is a component-based find module, which makes use of the COMPONENTS and
OPTIONAL_COMPONENTS arguments to find_module.  The following components are
available::

  XCB
  ATOM         AUX          COMPOSITE    CURSOR       DAMAGE
  DPMS         DRI2         DRI3         EVENT        EWMH
  GLX          ICCCM        IMAGE        KEYSYMS      PRESENT
  RANDR        RECORD       RENDER       RENDERUTIL   RES
  SCREENSAVER  SHAPE        SHM          SYNC         UTIL
  XEVIE        XF86DRI      XFIXES       XINERAMA     XINPUT
  XKB          XPRINT       XTEST        XV           XVMC

If no components are specified, this module will act as though all components
except XINPUT (which is considered unstable) were passed to
OPTIONAL_COMPONENTS.

This module will define the following variables, independently of the
components searched for or found:

``XCB_FOUND``
    True if (the requestion version of) xcb is available
``XCB_VERSION``
    Found xcb version
``XCB_TARGETS``
    A list of all targets imported by this module (note that there may be more
    than the components that were requested)
``XCB_LIBRARIES``
    This can be passed to target_link_libraries() instead of the imported
    targets
``XCB_INCLUDE_DIRS``
    This should be passed to target_include_directories() if the targets are
    not used for linking
``XCB_DEFINITIONS``
    This should be passed to target_compile_options() if the targets are not
    used for linking

For each searched-for components, ``XCB_<component>_FOUND`` will be set to
true if the corresponding xcb library was found, and false otherwise.  If
``XCB_<component>_FOUND`` is true, the imported target ``XCB::<component>``
will be defined.  This module will also attempt to determine
``XCB_*_VERSION`` variables for each imported target, although
``XCB_VERSION`` should normally be sufficient.

In general we recommend using the imported targets, as they are easier to use
and provide more control.  Bear in mind, however, that if any target is in the
link interface of an exported library, it must be made available by the
package config file.

Since pre-1.0.0.
#]=======================================================================]

include(${CMAKE_CURRENT_LIST_DIR}/ECMFindModuleHelpersStub.cmake)

ecm_find_package_version_check(XCB)

# Note that this list needs to be ordered such that any component
# appears after its dependencies
set(XCB_known_components
    XCB
    RENDER
    SHAPE
    XFIXES
    SHM
    ATOM
    AUX
    COMPOSITE
    CURSOR
    DAMAGE
    DPMS
    DRI2
    DRI3
    EVENT
    EWMH
    GLX
    ICCCM
    IMAGE
    KEYSYMS
    PRESENT
    RANDR
    RECORD
    RENDERUTIL
    RES
    SCREENSAVER
    SYNC
    UTIL
    XEVIE
    XF86DRI
    XINERAMA
    XINPUT
    XKB
    XPRINT
    XTEST
    XV
    XVMC
)

# XINPUT is unstable; do not include it by default
set(XCB_default_components ${XCB_known_components})
list(REMOVE_ITEM XCB_default_components "XINPUT")

# default component info: xcb components have fairly predictable
# header files, library names and pkg-config names
foreach(_comp ${XCB_known_components})
    string(TOLOWER "${_comp}" _lc_comp)
    set(XCB_${_comp}_component_deps XCB)
    set(XCB_${_comp}_pkg_config "xcb-${_lc_comp}")
    set(XCB_${_comp}_lib "xcb-${_lc_comp}")
    set(XCB_${_comp}_header "xcb/${_lc_comp}.h")
endforeach()
# exceptions
set(XCB_XCB_component_deps)
set(XCB_COMPOSITE_component_deps XCB XFIXES)
set(XCB_DAMAGE_component_deps XCB XFIXES)
set(XCB_IMAGE_component_deps XCB SHM)
set(XCB_RENDERUTIL_component_deps XCB RENDER)
set(XCB_XFIXES_component_deps XCB RENDER SHAPE)
set(XCB_XVMC_component_deps XCB XV)
set(XCB_XV_component_deps XCB SHM)
set(XCB_XCB_pkg_config "xcb")
set(XCB_XCB_lib "xcb")
set(XCB_ATOM_header "xcb/xcb_atom.h")
set(XCB_ATOM_lib "xcb-util")
set(XCB_AUX_header "xcb/xcb_aux.h")
set(XCB_AUX_lib "xcb-util")
set(XCB_CURSOR_header "xcb/xcb_cursor.h")
set(XCB_EVENT_header "xcb/xcb_event.h")
set(XCB_EVENT_lib "xcb-util")
set(XCB_EWMH_header "xcb/xcb_ewmh.h")
set(XCB_ICCCM_header "xcb/xcb_icccm.h")
set(XCB_IMAGE_header "xcb/xcb_image.h")
set(XCB_KEYSYMS_header "xcb/xcb_keysyms.h")
set(XCB_PIXEL_header "xcb/xcb_pixel.h")
set(XCB_RENDERUTIL_header "xcb/xcb_renderutil.h")
set(XCB_RENDERUTIL_lib "xcb-render-util")
set(XCB_UTIL_header "xcb/xcb_util.h")

ecm_find_package_parse_components(XCB
    RESULT_VAR XCB_components
    KNOWN_COMPONENTS ${XCB_known_components}
    DEFAULT_COMPONENTS ${XCB_default_components}
)

list(FIND XCB_components "XINPUT" _XCB_XINPUT_index)
if (NOT _XCB_XINPUT_index EQUAL -1)
    message(AUTHOR_WARNING "XINPUT from XCB was requested: this is EXPERIMENTAL and is likely to unavailable on many systems!")
endif()

ecm_find_package_handle_library_components(XCB
    COMPONENTS ${XCB_components}
)

find_package_handle_standard_args(XCB
    FOUND_VAR
        XCB_FOUND
    REQUIRED_VARS
        XCB_LIBRARIES
    VERSION_VAR
        XCB_VERSION
    HANDLE_COMPONENTS
)

include(FeatureSummary)
set_package_properties(XCB PROPERTIES
    URL "https://xcb.freedesktop.org/"
    DESCRIPTION "X protocol C-language Binding"
)
