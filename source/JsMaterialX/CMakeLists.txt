set(OUTPUT_CORE ${CMAKE_CURRENT_SOURCE_DIR}/JsMaterialXCore/)
set(OUTPUT_FORMAT ${CMAKE_CURRENT_SOURCE_DIR}/JsMaterialXFormat/)
set(OUTPUT_GENSHADER ${CMAKE_CURRENT_SOURCE_DIR}/JsMaterialXGenShader/)
set(OUTPUT_GENESSL ${CMAKE_CURRENT_SOURCE_DIR}/JsMaterialXGenEssl/)
set(OUTPUT_LIBS ${PROJECT_BINARY_DIR}/source/)

set(SOURCE_FOLDER ${CMAKE_SOURCE_DIR}/source)

set(CORE_DEPS ${OUTPUT_CORE}JsTypes.cpp
    ${OUTPUT_CORE}JsMaterial.cpp
    ${OUTPUT_CORE}JsUnit.cpp
    ${OUTPUT_CORE}JsExceptions.cpp
    ${OUTPUT_CORE}JsUtil.cpp
    ${OUTPUT_CORE}JsElement.cpp
    ${OUTPUT_CORE}JsInterface.cpp
    ${OUTPUT_CORE}JsNode.cpp
    ${OUTPUT_CORE}JsDefinition.cpp
    ${OUTPUT_CORE}JsDocument.cpp
    ${OUTPUT_CORE}JsLook.cpp
    ${OUTPUT_CORE}JsProperty.cpp
    ${OUTPUT_CORE}JsGeom.cpp
    ${OUTPUT_CORE}JsVariant.cpp
    ${OUTPUT_CORE}JsValue.cpp
    ${OUTPUT_CORE}JsTraversal.cpp)

set(FORMAT_DEPS
    ${OUTPUT_FORMAT}JsUtil.cpp
    ${OUTPUT_FORMAT}JsXmlIo.cpp
    ${OUTPUT_FORMAT}JsXmlExport.cpp)

set(GENSHADER_DEPS ${OUTPUT_GENSHADER}JsGenContext.cpp
    ${OUTPUT_GENSHADER}JsShader.cpp
    ${OUTPUT_GENSHADER}JsShaderGenerator.cpp)

set(GENESSL_DEPS ${OUTPUT_GENESSL}JsEsslShaderGenerator.cpp)

# Linker flags
set(JS_LINK_FLAGS_CORE "-I${SOURCE_FOLDER} \
    -s WASM=1 \
    -s DISABLE_EXCEPTION_CATCHING=0 \
    -s MODULARIZE=1 \
    -s USE_ES6_IMPORT_META=0 \
    -s EXPORT_NAME='MaterialX' \
    -s MEMORY_GROWTH_LINEAR_STEP=32MB \
    -s 'EXPORTED_RUNTIME_METHODS=[\"FS\"]' \
    -s ALLOW_MEMORY_GROWTH=1 ")

# Add the post javascript core files 
string(APPEND JS_LINK_FLAGS_CORE "--post-js ${OUTPUT_CORE}post.js ")



if (EXTERNAL_LINK_FLAGS)
    string(APPEND JS_LINK_FLAGS_CORE "${EXTERNAL_LINK_FLAGS} ")
endif()

# Compiler flags
set(JS_COMPILE_FLAGS "")
if (EXTERNAL_COMPILE_FLAGS)
    string(APPEND JS_COMPILE_FLAGS "${EXTERNAL_COMPILE_FLAGS} ")
endif()

if (CMAKE_BUILD_TYPE MATCHES Debug)
    message("JS: Building Debug")
    string(APPEND JS_LINK_FLAGS_CORE "--bind ")
else()
    message("JS: Building Release")
    string(APPEND JS_LINK_FLAGS_CORE "-flto --bind ")
    string(APPEND JS_COMPILE_FLAGS "-flto ")
endif()


set(JS_LINK_FLAGS_GENSHADER "${JS_LINK_FLAGS_CORE} --preload-file ${CMAKE_CURRENT_SOURCE_DIR}/../../libraries@libraries ")

add_executable(JsMaterialXCore MaterialXLib.cpp
    ${CORE_DEPS}
    ${FORMAT_DEPS})

add_executable(JsMaterialXGenShader MaterialXLib.cpp
    ${CORE_DEPS}
    ${FORMAT_DEPS}
    ${GENSHADER_DEPS}
    ${GENESSL_DEPS})

set_target_properties(JsMaterialXCore
    PROPERTIES
    OUTPUT_NAME JsMaterialXCore
    COMPILE_FLAGS "${JS_COMPILE_FLAGS}"
    LINK_FLAGS "${JS_LINK_FLAGS_CORE}"
    SOVERSION "${MATERIALX_MAJOR_VERSION}")

set_target_properties(JsMaterialXGenShader
    PROPERTIES
    OUTPUT_NAME JsMaterialXGenShader
    COMPILE_FLAGS "${JS_COMPILE_FLAGS}"
    LINK_FLAGS "${JS_LINK_FLAGS_GENSHADER}"
    SOVERSION "${MATERIALX_MAJOR_VERSION}")

target_link_libraries(JsMaterialXCore
    PUBLIC MaterialXCore
    PUBLIC MaterialXFormat
    PRIVATE ${CMAKE_DL_LIBS})

target_link_libraries(JsMaterialXGenShader
    PUBLIC MaterialXCore
    PUBLIC MaterialXFormat
    PUBLIC MaterialXGenShader
    PUBLIC MaterialXGenEssl
    PRIVATE ${CMAKE_DL_LIBS})

# Install the JavaScript output
install(TARGETS JsMaterialXCore DESTINATION "${CMAKE_INSTALL_PREFIX}/JavaScript/MaterialX")
install(TARGETS JsMaterialXGenShader DESTINATION "${CMAKE_INSTALL_PREFIX}/JavaScript/MaterialX")

# Copy the wasm output
install(FILES ${CMAKE_BINARY_DIR}/bin/JsMaterialXCore.wasm DESTINATION ${CMAKE_INSTALL_PREFIX}/JavaScript/MaterialX/)
install(FILES ${CMAKE_BINARY_DIR}/bin/JsMaterialXGenShader.wasm ${CMAKE_BINARY_DIR}/bin/JsMaterialXGenShader.data DESTINATION ${CMAKE_INSTALL_PREFIX}/JavaScript/MaterialX/)

# Copy the JavaScript files with the version appended to them.
install(FILES ${CMAKE_BINARY_DIR}/bin/JsMaterialXCore.js DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ RENAME JsMaterialXCore-${MATERIALX_LIBRARY_VERSION}.js)
install(FILES ${CMAKE_INSTALL_PREFIX}/JavaScript/MaterialX/JsMaterialXCore.js DESTINATION ${CMAKE_INSTALL_PREFIX}/JavaScript/MaterialX/ RENAME JsMaterialXCore-${MATERIALX_LIBRARY_VERSION}.js)

install(FILES ${CMAKE_BINARY_DIR}/bin/JsMaterialXGenShader.js DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/ RENAME JsMaterialXGenShader-${MATERIALX_LIBRARY_VERSION}.js)
install(FILES ${CMAKE_INSTALL_PREFIX}/JavaScript/MaterialX/JsMaterialXGenShader.js DESTINATION ${CMAKE_INSTALL_PREFIX}/JavaScript/MaterialX/ RENAME JsMaterialXGenShader-${MATERIALX_LIBRARY_VERSION}.js)
