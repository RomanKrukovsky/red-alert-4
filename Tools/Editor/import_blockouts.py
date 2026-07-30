# Unreal Engine 5 Python Script: Import 142 FBX Blockouts into StaticMesh .uassets
import os
import unreal

def import_all_blockouts():
    print("=== RA4 Automatic FBX Blockout Importer Starting ===")
    
    project_dir = unreal.Paths.project_dir()
    blockout_root = os.path.join(project_dir, "Content", "RA4", "Art", "Blockout")
    
    if not os.path.exists(blockout_root):
        print(f"Blockout directory not found: {blockout_root}")
        return

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    
    # Configure FBX Import Options
    options = unreal.FbxImportUI()
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_as_skeletal", False)
    
    options.static_mesh_import_data.set_editor_property("combine_meshes", True)
    options.static_mesh_import_data.set_editor_property("generate_lightmap_u_vs", True)
    options.static_mesh_import_data.set_editor_property("auto_generate_collision", True)
    
    imported_count = 0
    
    for root, dirs, files in os.walk(blockout_root):
        for file in files:
            if file.endswith(".fbx"):
                fbx_path = os.path.join(root, file)
                rel_dir = os.path.relpath(root, os.path.join(project_dir, "Content"))
                destination_path = "/Game/" + rel_dir.replace("\\", "/")
                
                mesh_name = os.path.splitext(file)[0]
                uasset_path = destination_path + "/" + mesh_name + "." + mesh_name
                
                # Create task
                task = unreal.AssetImportTask()
                task.set_editor_property("filename", fbx_path)
                task.set_editor_property("destination_path", destination_path)
                task.set_editor_property("destination_name", mesh_name)
                task.set_editor_property("options", options)
                task.set_editor_property("automated", True)
                task.set_editor_property("save", True)
                task.set_editor_property("replace_existing", True)
                
                asset_tools.import_asset_tasks([task])
                imported_count += 1
                print(f"Imported [{imported_count}]: {destination_path}/{mesh_name}")

    print(f"=== Successfully imported {imported_count} FBX blockout assets ===")

if __name__ == "__main__":
    import_all_blockouts()
