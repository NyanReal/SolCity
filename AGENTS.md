# SolCity agent instructions

## Blender modeling

When a task asks for a Blender model, scene, building, prop, material, render, or FBX export:

1. Read `Docs/BlenderMCP_Modeling_Guide.md` before acting.
2. Use the configured `blender` MCP server (`djeada/blender-mcp-server`) to inspect and modify Blender. Do not stop after merely writing a Blender Python script.
3. Target Blender 5.2 LTS and metric units unless the user specifies otherwise.
4. Preserve any Blender process or scene that the user already has open. Use a separate work instance when ownership is uncertain.
5. Store reusable generation and verification scripts under `Tools/blender-mcp-server/scripts/`.
6. Store building deliverables under `Content/Art/Buildings/` with matching `.blend`, `.fbx`, and `_preview.png` names.
7. Validate every delivered model by inspecting its rendered preview and re-importing the exported FBX into a clean Blender 5.2 scene.
8. Report dimensions, mesh-object count, output paths, and any Unreal import recommendation.
