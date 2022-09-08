#!/bin/bash

echo "Running CoreScriptConverter"
cd "$1"
./CoreScriptConverter --rscloc "../rsc.config" --verbose

cd "$1/../../../App/script/"

if [[ -e "LuaGenCS.inl" ]];
then
    file1=$(md5 -q LuaGenCSNew.inl)
    file2=$(md5 -q LuaGenCS.inl)

    echo $file1
    echo $file2

    if [ $file1 == $file2 ]
    then
        echo "No CoreScript changes detected"
        rm "LuaGenCSNew.inl"
    else
        echo "CoreScript changes detected, will require linking"
        rm "LuaGenCS.inl"
        mv "LuaGenCSNew.inl" "LuaGenCS.inl"
    fi
else
    echo "CoreScript changes detected, will require linking"
    mv "LuaGenCSNew.inl" "LuaGenCS.inl"
fi