
import FbxCommon

def display(node, indent):
  if not node: return

  print("%s%s" % (indent, node.GetNodeAttribute()))
  for i in range(node.GetChildCount()):
    child = node.GetChild(i)
    attr_type = child.GetNodeAttribute().GetAttributeType()

    if attr_type == FbxCommon.FbxNodeAttribute.eMesh:
      print(child)

    display(child, indent + "  ")


sdk_manager, scene = FbxCommon.InitializeSdkObjects()

if not FbxCommon.LoadScene(sdk_manager, scene, "cube.fbx"):
  print("error in LoadScene")

display(scene.GetRootNode(), "")

