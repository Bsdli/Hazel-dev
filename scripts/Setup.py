import os
import subprocess
import CheckPython

# Make sure everything we need is installed
CheckPython.ValidatePackages()

import Vulkan

# Change from Scripts directory to root
os.chdir('../')

if (not Vulkan.CheckVulkanSDK()):
    print("Vulkan SDK not installed.")
    
if (not Vulkan.CheckVulkanSDKDebugLibs()):
    print("Vulkan SDK debug libs not found.")

subprocess.call(["git", "lfs", "pull"])

print("Running premake...")
subprocess.call(["vendor/premake5.exe", "vs2022"])