#!/bin/bash
set -e
parent_path=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "$parent_path"
./tracktion_engine/modules/juce/extras/Projucer/Builds/LinuxMakefile/build/Projucer ./Zero-Sampler.jucer &