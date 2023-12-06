Scene: Scene Name
Environment:
  AssetPath: assets\env\pink_sunrise_4k.hdr
  Light:
    Direction: [0.0529999994, 0.795000017, 0]
    Radiance: [1, 0.999989986, 0.999989986]
    Multiplier: 0.651000023
Entities:
  - Entity: 17359415144565549859
    TagComponent:
      Tag: Directional Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1.00000322, 1.00000238]
    DirectionalLightComponent:
      Radiance: [1, 1, 1]
      CastShadows: true
      SoftShadows: true
      LightSize: 0.5
  - Entity: 15213035846546605980
    TagComponent:
      Tag: Sponza
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [0.100000001, 0.100000001, 0.100000001]
    MeshComponent:
      AssetPath: assets\meshes\sponza\sponza.obj
  - Entity: 7172337265010841038
    TagComponent:
      Tag: Sky Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0, 0, 0]
      Scale: [1, 1, 1]
    SkyLightComponent:
      EnvironmentAssetPath: assets\env\pink_sunrise_4k.hdr
      Intensity: 0.460000008
      Angle: 0
PhysicsLayers:
  []