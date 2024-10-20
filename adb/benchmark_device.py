#!/usr/bin/env python3
#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os
import statistics
import subprocess
import sys
import tempfile
import time

transfer_size_mib = 100
num_runs = 10

# Make sure environment is setup, otherwise "adb" module is not available.
if os.getenv("ANDROID_BUILD_TOP") is None:
    print("Run source/lunch before running " + sys.argv[0])
    sys.exit()

import adb

def lock_min(device):
    device.shell_nocheck(["""
        for x in /sys/devices/system/cpu/cpu?/cpufreq; do
            echo userspace > $x/scaling_governor
            cat $x/scaling_min_freq > $x/scaling_setspeed
        done
    """])

def lock_max(device):
    device.shell_nocheck(["""
        for x in /sys/devices/system/cpu/cpu?/cpufreq; do
            echo userspace > $x/scaling_governor
            cat $x/scaling_max_freq > $x/scaling_setspeed
        done
    """])

def unlock(device):
    device.shell_nocheck(["""
        for x in /sys/devices/system/cpu/cpu?/cpufreq; do
            echo ondemand > $x/scaling_governor
            echo sched > $x/scaling_governor
            echo schedutil > $x/scaling_governor
        done
    """])

def harmonic_mean(xs):
    return 1.0 / statistics.mean([1.0 / x for x in xs])

def analyze(name, speeds):
    median = statistics.median(speeds)
    mean = harmonic_mean(speeds)
    stddev = statistics.stdev(speeds)
    msg = "%s: %d runs: median %.2f MiB/s, mean %.2f MiB/s, stddev: %.2f MiB/s"
    print(msg % (name, len(speeds), median, mean, stddev))

def benchmark_sink(device=None, size_mb=transfer_size_mib):
    if device == None:
        device = adb.get_device()

    speeds = list()
    cmd = device.adb_cmd + ["raw", "sink:%d" % (size_mb * 1024 * 1024)]

    with tempfile.TemporaryFile() as tmpfile:
        tmpfile.truncate(size_mb * 1024 * 1024)

        for _ in range(0, num_runs):
            tmpfile.seek(0)
            begin = time.time()
            subprocess.check_call(cmd, stdin=tmpfile)
            end = time.time()
            speeds.append(size_mb / float(end - begin))

    analyze("sink   %dMiB (write RAM)  " % size_mb, speeds)

def benchmark_source(device=None, size_mb=transfer_size_mib):
    if device == None:
        device = adb.get_device()

    speeds = list()
    cmd = device.adb_cmd + ["raw", "source:%d" % (size_mb * 1024 * 1024)]

    with open(os.devnull, 'w') as devnull:
        for _ in range(0, num_runs):
            begin = time.time()
            subprocess.check_call(cmd, stdout=devnull)
            end = time.time()
            speeds.append(size_mb / float(end - begin))

    analyze("source %dMiB (read RAM)   " % size_mb, speeds)

def benchmark_push(device=None, file_size_mb=transfer_size_mib):
    if device == None:
        device = adb.get_device()

    remote_path = "/data/local/tmp/adb_benchmark_push_tmp"
    local_path = "/tmp/adb_benchmark_temp"

    with open(local_path, "wb") as f:
        f.truncate(file_size_mb * 1024 * 1024)

    speeds = list()
    for _ in range(0, num_runs):
        begin = time.time()
        parameters = ['-Z'] # Disable compression since our file is full of 0s
        device.push(local=local_path, remote=remote_path, parameters=parameters)
        end = time.time()
        speeds.append(file_size_mb / float(end - begin))

    analyze("push   %dMiB (write flash)" % file_size_mb, speeds)

def benchmark_pull(device=None, file_size_mb=transfer_size_mib):
    if device == None:
        device = adb.get_device()

    remote_path = "/data/local/tmp/adb_benchmark_pull_temp"
    local_path = "/tmp/adb_benchmark_temp"

    device.shell(["dd", "if=/dev/zero", "of=" + remote_path, "bs=1m",
                  "count=" + str(file_size_mb)])
    speeds = list()
    for _ in range(0, num_runs):
        begin = time.time()
        device.pull(remote=remote_path, local=local_path)
        end = time.time()
        speeds.append(file_size_mb / float(end - begin))

    analyze("pull   %dMiB (read flash) " % file_size_mb, speeds)

def benchmark_device_dd(device=None, file_size_mb=transfer_size_mib):
    if device == None:
        device = adb.get_device()

    speeds = list()
    for _ in range(0, num_runs):
        begin = time.time()
        device.shell(["dd", "if=/dev/zero", "bs=1m",
                      "count=" + str(file_size_mb)])
        end = time.time()
        speeds.append(file_size_mb / float(end - begin))

    analyze("dd     %dMiB (write flash)" % file_size_mb, speeds)

def main():
    device = adb.get_device()
    unlock(device)

    benchmark_sink(device)
    benchmark_source(device)
    benchmark_push(device)
    benchmark_pull(device)
    benchmark_device_dd(device)

if __name__ == "__main__":
    main()
