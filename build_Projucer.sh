#!/bin/bash
set -e
parent_path=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
cd "$parent_path"
cd tracktion_engine/modules/juce/extras/Projucer/Builds/LinuxMakefile/
make
echo "Do you wish to install Projucer?"
select yn in "Yes" "No"; do
    case $yn in
        Yes ) sudo ln -sfn "$parent_path"/tracktion_engine/modules/juce/extras/Projucer/Builds/LinuxMakefile/build/Projucer /usr/bin/Projucer; break;;
        No ) exit;;
    esac
done

