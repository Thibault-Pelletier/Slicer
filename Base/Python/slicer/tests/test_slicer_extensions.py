import sys
import unittest
import uuid
from pathlib import Path

import qt
import slicer
import slicer.extensions
from slicer.ScriptedLoadableModule import ScriptedLoadableModule


_TEMPLATE_MODULE = """
from slicer.ScriptedLoadableModule import ScriptedLoadableModule, ScriptedLoadableModuleWidget


class {name}(ScriptedLoadableModule):
    def __init__(self, parent):
        ScriptedLoadableModule.__init__(self, parent)
        self.parent.title = "{name}"
        self.parent.categories = []
        self.parent.dependencies = []


class {name}Widget(ScriptedLoadableModuleWidget):
    def setup(self):
        ScriptedLoadableModuleWidget.setup(self)
"""


class ScriptedExtensionMechanismTest(unittest.TestCase):
    def setUp(self):
        self._tmpDir = qt.QTemporaryDir()
        self._tmpDir.setAutoRemove(True)
        self._root = Path(self._tmpDir.path())

        self._tmpPath = []
        self._addToPath(self._root)
        self.addCleanup(self._cleanupTmpPaths)

    def _cleanupTmpPaths(self):
        for path in self._tmpPath:
            if path in sys.path:
                sys.path.remove(path)
        self._tmpPath = []

    def _addToPath(self, path):
        sys.path.append(str(path))
        self._tmpPath.append(str(path))

    @staticmethod
    def _uniqueName(prefix):
        return prefix + uuid.uuid4().hex[:8]

    def _createModule(self, moduleName, innerDir):
        packageName = f"slicer_ext_{moduleName}"
        moduleRoot = self._root / innerDir
        packageDir = moduleRoot / packageName
        packageDir.mkdir(parents=True)
        (packageDir / "__init__.py").write_text(f"from .{moduleName} import {moduleName}\n", encoding="utf-8")
        implementationPath = packageDir / f"{moduleName}.py"
        implementationPath.write_text(_TEMPLATE_MODULE.replace("{name}", moduleName), encoding="utf-8")
        modulesDir = moduleRoot / "slicer_ext" / "modules"
        modulesDir.mkdir(parents=True)
        (modulesDir / f"{moduleName}.py").write_text(f"from {packageName} import {moduleName}\n", encoding="utf-8")
        self._addToPath(moduleRoot)

    def test_can_load_two_installed_wheels_in_inner_python_paths(self):
        nameA = self._uniqueName("CoexistModuleA")
        nameB = self._uniqueName("CoexistModuleB")
        self._createModule(nameA, "_inner_1")
        self._createModule(nameB, "_inner_2")

        # Trigger registration
        slicer.extensions.loadInstalledModules()

        # Check that the modules have been registered
        moduleNames = dir(slicer.moduleNames)
        self.assertIn(nameA, moduleNames)
        self.assertIn(nameB, moduleNames)
        assert isinstance(getattr(slicer.modules, nameA.lower()), ScriptedLoadableModule)
        assert isinstance(getattr(slicer.modules, nameB.lower()), ScriptedLoadableModule)

