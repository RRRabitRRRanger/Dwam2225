# Lecture SEULE : exporte un asset en texte (T3D) dans Saved/Automation/, pour lire ses nœuds et leurs liens.
import unreal, os

ASSET = "/Game/AVOLABO/Shaders/MF_Light2"
asset = unreal.load_asset(ASSET)
out_dir = os.path.join(unreal.Paths.project_saved_dir(), "Automation")
os.makedirs(out_dir, exist_ok=True)
for ext in ("t3d", "copy"):
    task = unreal.AssetExportTask()
    task.object = asset
    task.filename = os.path.join(out_dir, os.path.basename(ASSET) + "." + ext)
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    ok = unreal.Exporter.run_asset_export_task(task)
    unreal.log(f"CLAUDE: export {ext} -> {ok} {task.filename}")
