#!/bin/bash
# Copyright 2020 Amazon.com, Inc. and its affiliates. All Rights Reserved.
# SPDX-License-Identifier: LicenseRef-.amazon.com.-AmznSL-1.0
# Licensed under the Amazon Software License (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
# http://aws.amazon.com/asl/
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.

set -euo pipefail

CLEAN=false
REPLACE=false

parse_args() {
    while [ "$#" -gt 0 ]; do
        case $1 in
        --clean)
            CLEAN=true
            ;;
        --replace)
            REPLACE=true
            ;;
        --help)
            echo "Usage: $0 [OPTION]"
            echo "  --clean    Only clean-up"
            echo "  --replace  Clean and replace, but don't rename"
            exit 0
            ;;
        esac
        shift
    done
}

parse_args "$@"

# revert changes
git reset --hard HEAD > /dev/null
git clean -fd > /dev/null

if ${CLEAN}; then
    exit 0
fi

# Perform replacements
find . \
    -type f \
    -not -path "*.git/*" \
    -not -path "./.gitmodules" \
    -not -path "./build*" \
    -not -path "./ke-acee-interfaces*" \
    -not -path "./schemas*" \
    -not -path "./interfaces*" \
    -not -path "./tools/magic-scripts*" \
    -not -path "./tools/renaming*" \
    -not -path "./tools/customer_release/*-cloud/*-2021-06-17.json" \
    -exec sed -E -i -f tools/renaming/rules.sed {} +

if ${REPLACE}; then
    exit 0
fi

# Perform file renaming
if true; then
    mv model-manifests/generators/can-network/Kaleidoscope_can_traffic_generator.py                        model-manifests/generators/can-network/IoTFleetWise_can_traffic_generator.py
    mv model-manifests/parsers/can-dbc-parser/Kaleidoscope_dbc_parser.py                                   model-manifests/parsers/can-dbc-parser/IoTFleetWise_dbc_parser.py
    mv src/executionmanagement/include/KaleidoscopeConfig.h                                                src/executionmanagement/include/IoTFleetWiseConfig.h
    mv src/executionmanagement/include/KaleidoscopeEngine.h                                                src/executionmanagement/include/IoTFleetWiseEngine.h
    mv src/executionmanagement/include/KaleidoscopeVersion.h                                               src/executionmanagement/include/IoTFleetWiseVersion.h
    mv src/executionmanagement/src/KaleidoscopeConfig.cpp                                                  src/executionmanagement/src/IoTFleetWiseConfig.cpp
    mv src/executionmanagement/src/KaleidoscopeEdge.cpp                                                    src/executionmanagement/src/IoTFleetWiseEdge.cpp
    mv src/executionmanagement/src/KaleidoscopeEngine.cpp                                                  src/executionmanagement/src/IoTFleetWiseEngine.cpp
    mv src/executionmanagement/src/KaleidoscopeVersion.cpp.in                                              src/executionmanagement/src/IoTFleetWiseVersion.cpp.in
    mv src/executionmanagement/test/KaleidoscopeConfigTest.cpp                                             src/executionmanagement/test/IoTFleetWiseConfigTest.cpp
    mv src/executionmanagement/test/KaleidoscopeEngineTest.cpp                                             src/executionmanagement/test/IoTFleetWiseEngineTest.cpp
    mv tools/customer_release/kaleidoscope-cloud                                                           tools/customer_release/aws-iot-fleetwise-cloud
    mv tools/customer_release/kaleidoscope-edge/tools/deploy/ke@.service                                   tools/customer_release/kaleidoscope-edge/tools/deploy/fwe@.service
    mv tools/customer_release/kaleidoscope-edge/tools/deploy/start-and-enable-ke.sh                        tools/customer_release/kaleidoscope-edge/tools/deploy/start-and-enable-fwe.sh
    mv tools/customer_release/kaleidoscope-edge/tools/deploy/stop-ke.sh                                    tools/customer_release/kaleidoscope-edge/tools/deploy/stop-fwe.sh
    mv tools/customer_release/kaleidoscope-edge/tools/yocto/sources/meta-kaleidoscope                      tools/customer_release/kaleidoscope-edge/tools/yocto/sources/meta-aws-iot-fleetwise
    mv tools/customer_release/kaleidoscope-edge/tools/build-ke-cross.sh                                    tools/customer_release/kaleidoscope-edge/tools/build-fwe-cross.sh
    mv tools/customer_release/kaleidoscope-edge/tools/build-ke-native.sh                                   tools/customer_release/kaleidoscope-edge/tools/build-fwe-native.sh
    mv tools/customer_release/kaleidoscope-edge/tools/configure-ke.sh                                      tools/customer_release/kaleidoscope-edge/tools/configure-fwe.sh
    mv tools/customer_release/kaleidoscope-edge/tools/install-ke.sh                                        tools/customer_release/kaleidoscope-edge/tools/install-fwe.sh
    mv tools/customer_release/kaleidoscope-edge                                                            tools/customer_release/aws-iot-fleetwise-edge
    mv tools/customer_release/cfn-templates/kdev.yml                                                       tools/customer_release/cfn-templates/fwdev.yml
    mv tools/customer_release/cfn-templates/kdemo.yml                                                      tools/customer_release/cfn-templates/fwdemo.yml
fi
