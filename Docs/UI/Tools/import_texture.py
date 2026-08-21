# -*- coding: utf-8 -*-
"""Import a PNG into /Game/RA4UI/Art, replacing any asset of the same name.

Usage: UnrealEditor-Cmd <project> -run=pythonscript -script=import_texture.py
       with RA4_IMPORT_FILE and RA4_IMPORT_NAME set in the environment.
"""
import os
import unreal

source = os.environ['RA4_IMPORT_FILE']
name = os.environ['RA4_IMPORT_NAME']

task = unreal.AssetImportTask()
task.filename = source
task.destination_path = '/Game/RA4UI/Art'
task.destination_name = name
task.replace_existing = True
task.automated = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
unreal.EditorAssetLibrary.save_asset('/Game/RA4UI/Art/' + name, only_if_is_dirty=False)
unreal.log('RA4 imported texture: ' + name)
