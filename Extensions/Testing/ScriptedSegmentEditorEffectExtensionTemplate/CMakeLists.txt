cmake_minimum_required(VERSION 3.20.6...3.22.6 FATAL_ERROR)

project(ScriptedSegmentEditorEffectExtensionTemplate)

#-----------------------------------------------------------------------------
# Extension meta-information
set(EXTENSION_HOMEPAGE "https://www.slicer.org/wiki/Documentation/Nightly/Extensions/ScriptedSegmentEditorEffectExtensionTemplate")
set(EXTENSION_CONTRIBUTORS "John Doe (AnyWare Corp.)")
set(EXTENSION_DESCRIPTION "This is an example of a simple extension")
set(EXTENSION_ICONURL "https://www.example.com/Slicer/Extensions/ScriptedSegmentEditorEffectExtensionTemplate.png")
set(EXTENSION_SCREENSHOTURLS "https://www.example.com/Slicer/Extensions/ScriptedSegmentEditorEffectExtensionTemplate/Screenshots/1.png")
set(EXTENSION_DEPENDS "NA") # Specified as a list or "NA" if no dependencies

#-----------------------------------------------------------------------------
# Extension dependencies
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})

#-----------------------------------------------------------------------------
# Extension modules
add_subdirectory(ScriptedSegmentEditorEffectModuleTemplate)
## NEXT_MODULE

#-----------------------------------------------------------------------------
include(${Slicer_EXTENSION_GENERATE_CONFIG})
include(${Slicer_EXTENSION_CPACK})
