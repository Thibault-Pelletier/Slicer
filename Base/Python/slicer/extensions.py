import importlib
import logging
import pkgutil
import qt
import slicer


def registerInstalledModules(factoryManager=None):
    """
    Looks for scripted modules registered in slicer_ext.modules across all registered python path and registers
    them to the qSlicerModuleFactoryManager instance.
    """
    from slicer.ScriptedLoadableModule import ScriptedLoadableModule

    factoryManager = _resolveFactoryManager(factoryManager)
    if not factoryManager:
        return []

    namespaceName = "slicer_ext.modules"
    try:
        namespace = importlib.import_module(namespaceName)
    except ModuleNotFoundError:
        return []

    # Iterate on all module files present in the slicer_ext.modules folder(s)
    registered = []
    for moduleInfo in pkgutil.iter_modules(namespace.__path__):
        name = moduleInfo.name

        # Skip internal / private / already registered modules
        if factoryManager.isRegistered(name) or name.startswith("_"):
            continue

        # Import the extension module located in slicer_ext.modules
        # The module is expected to import the actual scripted module classes from their python package
        entryName = f"{namespaceName}.{name}"
        try:
            entry = importlib.import_module(entryName)
        except ImportError as e:
            _errorMsg = f"Failed to import scripted module entry point {entryName}: {e}"
            logging.error(_errorMsg)
            continue

        # List all scripted modules from the module and try to register each
        moduleClasses = [c for c in dir(entry) if type(c) == type and issubclass(c, ScriptedLoadableModule)]

        # For each scripted module, let the factory try to register it from the package path
        for moduleClass in moduleClasses:
            filePath = importlib.import_module(moduleClass.__module__).__file__
            factoryManager.registerModule(qt.QFileInfo(filePath))
            if factoryManager.isRegistered(name):
                registered.append(name)

    return registered


def loadInstalledModules(factoryManager=None) -> list[str]:
    """
    Iterate on extensions registered in slicer_ext.modules and load them.

    :param factoryManager: optional instance to the factory manager to use during loading
    :returns list of loaded modules names
    """
    factoryManager = _resolveFactoryManager(factoryManager)
    if not factoryManager:
        return []

    moduleNames = registerInstalledModules(factoryManager)
    factoryManager.loadModules(moduleNames)
    for name in moduleNames:
        if factoryManager.isLoaded(name):
            setattr(slicer.moduleNames, name, name)
            setattr(slicer.modules, name.lower(), factoryManager.moduleInstance(name))

    return moduleNames


def _resolveFactoryManager(factoryManager):
    return factoryManager or slicer.app.moduleManager().factoryManager()
