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

# arg1: ${CMAKE_CURRENT_SOURCE_DIR}
# arg2: ${CMAKE_BINARY_DIR}

# Generate compile_commands.json for clang-tidy test
python3 $1/tools/code_check/compile_db_remove_test.py $2

# Run clang-tidy test
run-clang-tidy-10 -header-filter=$1/src/.* -p $2/Testing/Temporary $1/src
