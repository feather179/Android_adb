cmake_minimum_required(VERSION 3.21)

project(deployagent LANGUAGES Java)

find_package(Java REQUIRED)
include(UseJava)

set(CMAKE_JAVA_COMPILE_FLAGS --release 11)
set(DEPLOYAGENT_CLASSES deployagent_classes)
set(DEPLOYAGENT_CLASSES_JAR ${CMAKE_CURRENT_BINARY_DIR}/deployagent_classes.jar)
set(DEPLOYAGENT_JAR ${CMAKE_CURRENT_BINARY_DIR}/deployagent.jar)


file(GLOB FASTDEPLOY_SRCS "deployagent/src/com/android/fastdeploy/*.java")
file(GLOB FASTDEPLOY_PROTO_SRCS "proto/com/android/fastdeploy/*.java")

add_jar(${PROJECT_NAME}
    ${FASTDEPLOY_SRCS}
    ${FASTDEPLOY_PROTO_SRCS}

    INCLUDE_JARS
    ${ANDROID_JAR}
    protobuf-java-3.21.12.jar

    OUTPUT_NAME
    ${DEPLOYAGENT_CLASSES}
)

add_custom_target(deployagent_post_process ALL
    COMMAND ${ANDROID_D8} --min-api 26 --output ${DEPLOYAGENT_JAR} ${DEPLOYAGENT_CLASSES_JAR}
    COMMAND pwsh -File bin2c.ps1 -FilePath ${CMAKE_CURRENT_BINARY_DIR}/deployagent.jar -VariableName kDeployAgent -OutputFile ${CMAKE_CURRENT_BINARY_DIR}/deployagent.inc
    COMMAND pwsh -File bin2c.ps1 -FilePath deployagent/deployagent.sh -VariableName kDeployAgentScript -OutputFile ${CMAKE_CURRENT_BINARY_DIR}/deployagentscript.inc
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
