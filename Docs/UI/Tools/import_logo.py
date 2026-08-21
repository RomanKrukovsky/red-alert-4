import unreal
task = unreal.AssetImportTask()
task.filename = '/tmp/T_RA4_Logo_source.png'
task.destination_path = '/Game/RA4UI/Art'
task.destination_name = 'T_RA4_Logo'
task.replace_existing = True
task.automated = True
task.save = True
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
for p in task.get_editor_property('imported_object_paths'):
    unreal.log('RA4 imported: ' + p)
unreal.EditorAssetLibrary.save_asset('/Game/RA4UI/Art/T_RA4_Logo', only_if_is_dirty=False)
unreal.log('RA4 logo import done')
