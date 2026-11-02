# Game Content

Game assets live here (like Unreal's `Content/` folder).

| Extension | What it is | Loaded by |
| :--- | :--- | :--- |
| `.texaxe` | Encrypted Aurora texture | `M_Texture` |
| `.meshae` | Encrypted Aurora mesh | `M_Mesh` |
| `.mapae`  | Encrypted Aurora map / world | `M_World` |
| `.glb`, `.png`, ... | Raw source assets (unencrypted, for editing) | Packed into the formats above |

Suggested layout as content grows:

```
Content/
├── Textures/
├── Meshes/
└── Maps/
```
