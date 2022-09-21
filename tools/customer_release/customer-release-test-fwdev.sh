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

# Install deps, run cansim and build edge
mkdir -p ~/aws-iot-fleetwise-edge && cd ~/aws-iot-fleetwise-edge
aws s3 cp s3://kaleidoscope-demo/${CI_COMMIT_SHORT_SHA}/aws-iot-fleetwise-edge.zip .
unzip -q -o aws-iot-fleetwise-edge.zip
sudo -H ./tools/install-deps-native.sh
sudo -H ./tools/install-socketcan.sh
sudo -H ./tools/install-cansim.sh
./tools/build-fwe-native.sh

# Provision, configure, install and run edge
sudo mkdir -p /etc/aws-iot-fleetwise
sudo ./tools/provision.sh \
  --vehicle-name ${PROJECT_NAME} \
  --certificate-pem-outfile /etc/aws-iot-fleetwise/certificate.pem \
  --private-key-outfile /etc/aws-iot-fleetwise/private-key.key \
  --endpoint-url-outfile /etc/aws-iot-fleetwise/endpoint.txt \
  --vehicle-name-outfile /etc/aws-iot-fleetwise/vehicle-name.txt
sudo ./tools/configure-fwe.sh \
  --input-config-file configuration/static-config.json \
  --output-config-file /etc/aws-iot-fleetwise/config-0.json \
  --vehicle-name `cat /etc/aws-iot-fleetwise/vehicle-name.txt` \
  --endpoint-url `cat /etc/aws-iot-fleetwise/endpoint.txt` \
  --can-bus0 vcan0 \
  --topic-prefix \$aws/iotfleetwise/gamma-us-east-1/
sudo ./tools/install-fwe.sh

# Run the cloud demo script
cd tools/cloud
sudo -H ./install-deps.sh
./demo.sh --vehicle-name ${PROJECT_NAME} --endpoint-url https://controlplane.us-east-1.gamma.kaleidoscope.iot.aws.dev --clean-up
./demo.sh --vehicle-name ${PROJECT_NAME} --endpoint-url https://controlplane.us-east-1.gamma.kaleidoscope.iot.aws.dev --campaign-file campaign-obd-heartbeat.json --clean-up

# TODO: Ideally the HIL would now be used to test the ARM64 binary on an S32G with cansim running on a physical CAN bus
