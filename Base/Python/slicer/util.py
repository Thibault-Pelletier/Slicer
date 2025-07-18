#
# General
#

EXIT_SUCCESS = 0
EXIT_FAILURE = 1

TESTING_DATA_URL = "https://github.com/Slicer/SlicerTestingData/releases/download/"
"""Base URL for downloading testing data.

Datasets can be downloaded using URL of the form ``TESTING_DATA_URL + "SHA256/" + sha256ofDataSet``
"""

DATA_STORE_URL = "https://github.com/Slicer/SlicerDataStore/releases/download/"
"""Base URL for downloading data from Slicer Data Store. Data store contains atlases,
registration case library images, and various sample data sets.

Datasets can be downloaded using URL of the form ``DATA_STORE_URL + "SHA256/" + sha256ofDataSet``
"""


def quit():
    exit(EXIT_SUCCESS)


def exit(status=EXIT_SUCCESS):
    """Exits the application with the specified exit code.

    The method does not stop the process immediately but lets
    pending events to be processed.
    If exit() is called again while processing pending events,
    the error code will be overwritten.

    To make the application exit immediately, this code can be used.

    ..warning::

      Forcing the application to exit may result in
      improperly released files and other resources.

    .. code-block:: python

      import sys
      sys.exit(status)

    """

    from slicer import app

    # Prevent automatic application exit (for example, triggered by starting Slicer
    # with "--testing" argument) from overwriting the exit code that we set now.
    app.commandOptions().runPythonAndExit = False
    app.exit(status)


def restart():
    """Restart the application.

    No confirmation popup is displayed.
    """

    from slicer import app

    app.restart()


def _readCMakeCache(var):
    import os
    from slicer import app

    prefix = var + ":"

    try:
        with open(os.path.join(app.slicerHome, "CMakeCache.txt")) as cache:
            for line in cache:
                if line.startswith(prefix):
                    return line.split("=", 1)[1].rstrip()

    except:
        pass

    return None


def sourceDir():
    """Location of the Slicer source directory.

    :type: :class:`str` or ``None``

    This provides the location of the Slicer source directory, if Slicer is being
    run from a CMake build directory. If the Slicer home directory does not
    contain a ``CMakeCache.txt`` (e.g. for an installed Slicer), the property
    will have the value ``None``.
    """
    return _readCMakeCache("Slicer_SOURCE_DIR")


def startupEnvironment():
    """Returns the environment without the Slicer specific values.

    Path environment variables like `PATH`, `LD_LIBRARY_PATH` or `PYTHONPATH`
    will not contain values found in the launcher settings.

    Similarly `key=value` environment variables also found in the launcher
    settings are excluded.

    The function excludes both the Slicer launcher settings and the revision
    specific launcher settings.

    .. warning::

      If a value was associated with a key prior
      starting Slicer, it will not be set in the environment returned by this
      function.
    """
    import slicer

    startupEnv = slicer.app.startupEnvironment()
    import os

    # "if varname" is added to reject empty key (it is invalid)
    if os.name == "nt":
        # On Windows, subprocess functions expect environment to contain strings
        # and Qt provide us unicode strings, so we need to convert them.
        return {str(varname): str(startupEnv.value(varname)) for varname in list(startupEnv.keys()) if varname}
    else:
        return {varname: startupEnv.value(varname) for varname in list(startupEnv.keys()) if varname}


#
# Custom Import
#


def importVTKClassesFromDirectory(directory, dest_module_name, filematch="*"):
    from vtk import vtkObjectBase

    importClassesFromDirectory(directory, dest_module_name, vtkObjectBase, filematch)


def importQtClassesFromDirectory(directory, dest_module_name, filematch="*"):
    importClassesFromDirectory(directory, dest_module_name, "PythonQtClassWrapper", filematch)


# To avoid globbing multiple times the same directory, successful
# call to ``importClassesFromDirectory()`` will be indicated by
# adding an entry to the ``__import_classes_cache`` set.
#
# Each entry is a tuple of form (directory, dest_module_name, type_info, filematch)
__import_classes_cache = set()


def importClassesFromDirectory(directory, dest_module_name, type_info, filematch="*"):
    # Create entry for __import_classes_cache
    cache_key = ",".join([str(arg) for arg in [directory, dest_module_name, type_info, filematch]])
    # Check if function has already been called with this set of parameters
    if cache_key in __import_classes_cache:
        return

    import glob, os, re, fnmatch

    re_filematch = re.compile(fnmatch.translate(filematch))
    for fname in glob.glob(os.path.join(directory, filematch)):
        if not re_filematch.match(os.path.basename(fname)):
            continue
        try:
            from_module_name = os.path.splitext(os.path.basename(fname))[0]
            importModuleObjects(from_module_name, dest_module_name, type_info)
        except ImportError as detail:
            import sys

            print(detail, file=sys.stderr)

    __import_classes_cache.add(cache_key)


def importModuleObjects(from_module_name, dest_module_name, type_info):
    """Import object of type 'type_info' (str or type) from module identified
    by 'from_module_name' into the module identified by 'dest_module_name'.
    """

    # Obtain a reference to the module identified by 'dest_module_name'
    import sys

    dest_module = sys.modules[dest_module_name]

    # Skip if module has already been loaded
    if from_module_name in dir(dest_module):
        return

    # Obtain a reference to the module identified by 'from_module_name'
    import importlib

    module = importlib.import_module(from_module_name)

    # Loop over content of the python module associated with the given python library
    for item_name in dir(module):
        # Obtain a reference associated with the current object
        item = getattr(module, item_name)

        # Check type match by type or type name
        match = False
        if isinstance(type_info, type):
            try:
                match = issubclass(item, type_info)
            except TypeError as e:
                pass
        else:
            match = type(item).__name__ == type_info

        if match:
            setattr(dest_module, item_name, item)


#
# UI
#


def lookupTopLevelWidget(objectName):
    """Loop over all top level widget associated with 'slicer.app' and
    return the one matching 'objectName'

    :raises RuntimeError: if no top-level widget is found by that name
    """
    from slicer import app

    for w in app.topLevelWidgets():
        if hasattr(w, "objectName"):
            if w.objectName == objectName:
                return w
    # not found
    raise RuntimeError("Failed to obtain reference to '%s'" % objectName)


def mainWindow():
    """Get main window widget (qSlicerMainWindow object)

    :return: main window widget, or ``None`` if there is no main window
    """
    try:
        mw = lookupTopLevelWidget("qSlicerMainWindow")
    except RuntimeError:
        # main window not found, return None
        # Note: we do not raise an exception so that this function can be conveniently used
        # in expressions such as `parent if parent else mainWindow()`
        mw = None
    return mw


def pythonShell():
    """Get Python console widget (ctkPythonConsole object)

    :raises RuntimeError: if not found
    """
    from slicer import app

    console = app.pythonConsole()
    if not console:
        raise RuntimeError("Failed to obtain reference to python shell")
    return console


def showStatusMessage(message, duration=0):
    """Display ``message`` in the status bar."""
    mw = mainWindow()
    if not mw or not mw.statusBar():
        return False
    mw.statusBar().showMessage(message, duration)
    return True


def findChildren(widget=None, name="", text="", title="", className=""):
    """Return a list of child widgets that meet all the given criteria.

    If no criteria are provided, the function will return all widgets descendants.
    If no widget is provided, slicer.util.mainWindow() is used.

    :param widget: parent widget where the widgets will be searched
    :param name: name attribute of the widget
    :param text: text attribute of the widget
    :param title: title attribute of the widget
    :param className: className() attribute of the widget
    :return: list with all the widgets that meet all the given criteria.
    """
    # TODO: figure out why the native QWidget.findChildren method does not seem to work from PythonQt
    import fnmatch

    if not widget:
        widget = mainWindow()
    if not widget:
        return []
    children = []
    parents = [widget]
    kwargs = {"name": name, "text": text, "title": title, "className": className}
    expected_matches = []
    for kwarg_key, kwarg_value in kwargs.items():
        if kwarg_value:
            expected_matches.append(kwarg_key)
    while parents:
        p = parents.pop()
        # sometimes, p is null, f.e. when using --python-script or --python-code
        if not p:
            continue
        if not hasattr(p, "children"):
            continue
        parents += p.children()
        matched_filter_criteria = 0
        for attribute in expected_matches:
            if hasattr(p, attribute):
                attr_name = getattr(p, attribute)
                if attribute == "className":
                    # className is a method, not a direct attribute. Invoke the method
                    attr_name = attr_name()
                # Objects may have text attributes with non-string value (for example,
                # QUndoStack objects have text attribute of 'builtin_qt_slot' type.
                # We only consider string type attributes.
                if isinstance(attr_name, str):
                    if fnmatch.fnmatchcase(attr_name, kwargs[attribute]):
                        matched_filter_criteria = matched_filter_criteria + 1
        if matched_filter_criteria == len(expected_matches):
            children.append(p)
    return children


def findChild(widget, name):
    """Convenience method to access a widget by its ``name``.

    :raises RuntimeError: if the widget with the given ``name`` does not exist.
    """
    errorMessage = "Widget named " + str(name) + " does not exist."
    child = None
    try:
        child = findChildren(widget, name=name)[0]
        if not child:
            raise RuntimeError(errorMessage)
    except IndexError:
        raise RuntimeError(errorMessage)
    return child


def loadUI(path):
    """Load UI file ``path`` and return the corresponding widget.

    :raises RuntimeError: if the UI file is not found or if no
     widget was instantiated.
    """
    import qt

    qfile = qt.QFile(path)
    if not qfile.exists():
        errorMessage = "Could not load UI file: file not found " + str(path) + "\n\n"
        raise RuntimeError(errorMessage)
    qfile.open(qt.QFile.ReadOnly)
    loader = qt.QUiLoader()
    widget = loader.load(qfile)
    if not widget:
        errorMessage = "Could not load UI file: " + str(path) + "\n\n"
        raise RuntimeError(errorMessage)
    return widget


def startQtDesigner(args=None):
    """Start Qt Designer application to allow editing UI files."""
    import slicer

    cmdLineArguments = []
    if args is not None:
        if isinstance(args, str):
            cmdLineArguments.append(args)
        else:
            cmdLineArguments.extend(args)
    return slicer.app.launchDesigner(cmdLineArguments)


def childWidgetVariables(widget):
    """Get child widgets as attributes of an object.

    Each named child widget is accessible as an attribute of the returned object,
    with the attribute name matching the child widget name.
    This function provides convenient access to widgets in a loaded UI file.

    Example::

      uiWidget = slicer.util.loadUI(myUiFilePath)
      self.ui = slicer.util.childWidgetVariables(uiWidget)
      self.ui.inputSelector.setMRMLScene(slicer.mrmlScene)
      self.ui.outputSelector.setMRMLScene(slicer.mrmlScene)

    """
    ui = type("", (), {})()  # empty object
    childWidgets = findChildren(widget)
    for childWidget in childWidgets:
        if hasattr(childWidget, "name"):
            setattr(ui, childWidget.name, childWidget)
    return ui


def addParameterEditWidgetConnections(parameterEditWidgets, updateParameterNodeFromGUI):
    """Add connections to get notification of a widget change.

    The function is useful for calling updateParameterNodeFromGUI method in scripted module widgets.

    .. note:: Not all widget classes are supported yet. Report any missing classes at https://discourse.slicer.org.

    Example::

      class SurfaceToolboxWidget(ScriptedLoadableModuleWidget, VTKObservationMixin):
        ...
        def setup(self):
          ...
          self.parameterEditWidgets = [
            (self.ui.inputModelSelector, "inputModel"),
            (self.ui.outputModelSelector, "outputModel"),
            (self.ui.decimationButton, "decimation"),
            ...]
          slicer.util.addParameterEditWidgetConnections(self.parameterEditWidgets, self.updateParameterNodeFromGUI)

        def updateGUIFromParameterNode(self, caller=None, event=None):
          if self._parameterNode is None or self._updatingGUIFromParameterNode:
            return
          self._updatingGUIFromParameterNode = True
          slicer.util.updateParameterEditWidgetsFromNode(self.parameterEditWidgets, self._parameterNode)
          self._updatingGUIFromParameterNode = False

        def updateParameterNodeFromGUI(self, caller=None, event=None):
          if self._parameterNode is None or self._updatingGUIFromParameterNode:
            return
          wasModified = self._parameterNode.StartModify()  # Modify all properties in a single batch
          slicer.util.updateNodeFromParameterEditWidgets(self.parameterEditWidgets, self._parameterNode)
          self._parameterNode.EndModify(wasModified)
    """

    for widget, parameterName in parameterEditWidgets:
        widgetClassName = widget.className()
        if widgetClassName == "QSpinBox":
            widget.connect("valueChanged(int)", updateParameterNodeFromGUI)
        elif widgetClassName == "QCheckBox":
            widget.connect("clicked()", updateParameterNodeFromGUI)
        elif widgetClassName == "QPushButton":
            widget.connect("toggled(bool)", updateParameterNodeFromGUI)
        elif widgetClassName == "qMRMLNodeComboBox":
            widget.connect("currentNodeIDChanged(QString)", updateParameterNodeFromGUI)
        elif widgetClassName == "QComboBox":
            widget.connect("currentIndexChanged(int)", updateParameterNodeFromGUI)
        elif widgetClassName == "ctkSliderWidget":
            widget.connect("valueChanged(double)", updateParameterNodeFromGUI)


def removeParameterEditWidgetConnections(parameterEditWidgets, updateParameterNodeFromGUI):
    """Remove connections created by :py:meth:`addParameterEditWidgetConnections`."""

    for widget, parameterName in parameterEditWidgets:
        widgetClassName = widget.className()
        if widgetClassName == "QSpinBox":
            widget.disconnect("valueChanged(int)", updateParameterNodeFromGUI)
        elif widgetClassName == "QPushButton":
            widget.disconnect("toggled(bool)", updateParameterNodeFromGUI)
        elif widgetClassName == "qMRMLNodeComboBox":
            widget.disconnect("currentNodeIDChanged(QString)", updateParameterNodeFromGUI)
        elif widgetClassName == "QComboBox":
            widget.disconnect("currentIndexChanged(int)", updateParameterNodeFromGUI)
        elif widgetClassName == "ctkSliderWidget":
            widget.disconnect("valueChanged(double)", updateParameterNodeFromGUI)


def updateParameterEditWidgetsFromNode(parameterEditWidgets, parameterNode):
    """Update widgets from values stored in a vtkMRMLScriptedModuleNode.

    The function is useful for implementing updateGUIFromParameterNode.

    See example in :py:meth:`addParameterEditWidgetConnections` documentation.

    .. note::

      Only a few widget classes are supported now. More will be added later.
      Report any missing classes at https://discourse.slicer.org.
    """

    for widget, parameterName in parameterEditWidgets:
        widgetClassName = widget.className()
        parameterValue = parameterNode.GetParameter(parameterName)
        if widgetClassName == "QSpinBox":
            if parameterValue:
                widget.value = int(float(parameterValue))
            else:
                widget.value = 0
        if widgetClassName == "ctkSliderWidget":
            if parameterValue:
                widget.value = float(parameterValue)
            else:
                widget.value = 0.0
        elif widgetClassName == "QCheckBox" or widgetClassName == "QPushButton":
            widget.checked = parameterValue == "true"
        elif widgetClassName == "QComboBox":
            widget.setCurrentText(parameterValue)
        elif widgetClassName == "qMRMLNodeComboBox":
            widget.currentNodeID = parameterNode.GetNodeReferenceID(parameterName)


def updateNodeFromParameterEditWidgets(parameterEditWidgets, parameterNode):
    """Update vtkMRMLScriptedModuleNode from widgets.

    The function is useful for implementing updateParameterNodeFromGUI.

    .. note::

      Only a few widget classes are supported now. More will be added later.
      Report any missing classes at https://discourse.slicer.org.

    See example in :py:meth:`addParameterEditWidgetConnections` documentation.
    """

    for widget, parameterName in parameterEditWidgets:
        widgetClassName = widget.className()
        if widgetClassName == "QSpinBox" or widgetClassName == "ctkSliderWidget":
            parameterNode.SetParameter(parameterName, str(widget.value))
        elif widgetClassName == "QCheckBox" or widgetClassName == "QPushButton":
            parameterNode.SetParameter(parameterName, "true" if widget.checked else "false")
        elif widgetClassName == "QComboBox":
            parameterNode.SetParameter(parameterName, widget.currentText)
        elif widgetClassName == "qMRMLNodeComboBox":
            parameterNode.SetNodeReferenceID(parameterName, widget.currentNodeID)


def setSliceViewerLayers(background="keep-current", foreground="keep-current", label="keep-current",
                         foregroundOpacity=None, labelOpacity=None, fit=False, rotateToVolumePlane=False):
    """Set the slice views with the given nodes.

    If node ID is not specified (or value is 'keep-current') then the layer will not be modified.

    :param background: node or node ID to be used for the background layer
    :param foreground: node or node ID to be used for the foreground layer
    :param label: node or node ID to be used for the label layer
    :param foregroundOpacity: opacity of the foreground layer
    :param labelOpacity: opacity of the label layer
    :param rotateToVolumePlane: rotate views to closest axis of the selected background, foreground, or label volume
    :param fit: fit slice views to their content (position&zoom to show all visible layers)
    """
    import slicer

    def _nodeID(nodeOrID):
        nodeID = nodeOrID
        if isinstance(nodeOrID, slicer.vtkMRMLNode):
            nodeID = nodeOrID.GetID()
        return nodeID

    num = slicer.mrmlScene.GetNumberOfNodesByClass("vtkMRMLSliceCompositeNode")
    for i in range(num):
        sliceViewer = slicer.mrmlScene.GetNthNodeByClass(i, "vtkMRMLSliceCompositeNode")
        if background != "keep-current":
            sliceViewer.SetBackgroundVolumeID(_nodeID(background))
        if foreground != "keep-current":
            sliceViewer.SetForegroundVolumeID(_nodeID(foreground))
        if foregroundOpacity is not None:
            sliceViewer.SetForegroundOpacity(foregroundOpacity)
        if label != "keep-current":
            sliceViewer.SetLabelVolumeID(_nodeID(label))
        if labelOpacity is not None:
            sliceViewer.SetLabelOpacity(labelOpacity)

    if rotateToVolumePlane:
        if background != "keep-current":
            volumeNode = slicer.mrmlScene.GetNodeByID(_nodeID(background))
        elif foreground != "keep-current":
            volumeNode = slicer.mrmlScene.GetNodeByID(_nodeID(foreground))
        elif label != "keep-current":
            volumeNode = slicer.mrmlScene.GetNodeByID(_nodeID(label))
        else:
            volumeNode = None
        if volumeNode:
            layoutManager = slicer.app.layoutManager()
            for sliceViewName in layoutManager.sliceViewNames():
                layoutManager.sliceWidget(sliceViewName).mrmlSliceNode().RotateToVolumePlane(volumeNode)

    if fit:
        layoutManager = slicer.app.layoutManager()
        if layoutManager is not None:
            sliceLogics = layoutManager.mrmlSliceLogics()
            for i in range(sliceLogics.GetNumberOfItems()):
                sliceLogic = sliceLogics.GetItemAsObject(i)
                if sliceLogic:
                    sliceLogic.FitSliceToBackground()


def setToolbarsVisible(visible, ignore=None):
    """Show/hide all existing toolbars, except those listed in ignore list.

    If there is no main window then the function has no effect.
    """
    mw = mainWindow()
    if not mw:
        return
    for toolbar in mainWindow().findChildren("QToolBar"):
        if ignore is not None and toolbar in ignore:
            continue
        toolbar.setVisible(visible)

    # Prevent sequence browser toolbar showing up automatically
    # when a sequence is loaded.
    # (put in try block because Sequence Browser module is not always installed)
    try:
        import slicer

        slicer.modules.sequences.autoShowToolBar = visible
    except:
        # Sequences module is not installed
        pass


def setMenuBarsVisible(visible, ignore=None):
    """Show/hide all menu bars, except those listed in ignore list.

    If there is no main window then the function has no effect.
    """
    mw = mainWindow()
    if not mw:
        return
    for menubar in mw.findChildren("QMenuBar"):
        if ignore is not None and menubar in ignore:
            continue
        menubar.setVisible(visible)


def setPythonConsoleVisible(visible):
    """Show/hide Python console.

    If there is no main window then the function has no effect.
    """
    mw = mainWindow()
    if not mw:
        return
    mw.pythonConsole().parent().setVisible(visible)


def setErrorLogVisible(visible):
    """Show/hide Error log window.

    If there is no main window then the function has no effect.
    """
    mw = mainWindow()
    if not mw:
        return
    mw.errorLogDockWidget().setVisible(visible)


def setStatusBarVisible(visible):
    """Show/hide status bar

    If there is no main window or status bar then the function has no effect.
    """
    mw = mainWindow()
    if not mw or not mw.statusBar():
        return
    mw.statusBar().setVisible(visible)


def setViewControllersVisible(visible):
    """Show/hide view controller toolbar at the top of slice and 3D views"""
    import slicer

    lm = slicer.app.layoutManager()
    for viewIndex in range(lm.threeDViewCount):
        lm.threeDWidget(viewIndex).threeDController().setVisible(visible)
    for sliceViewName in lm.sliceViewNames():
        lm.sliceWidget(sliceViewName).sliceController().setVisible(visible)
    for viewIndex in range(lm.tableViewCount):
        lm.tableWidget(viewIndex).tableController().setVisible(visible)
    for viewIndex in range(lm.plotViewCount):
        lm.plotWidget(viewIndex).plotController().setVisible(visible)


def forceRenderAllViews():
    """Force rendering of all views"""
    import slicer

    lm = slicer.app.layoutManager()
    for viewIndex in range(lm.threeDViewCount):
        lm.threeDWidget(viewIndex).threeDView().forceRender()
    for sliceViewName in lm.sliceViewNames():
        lm.sliceWidget(sliceViewName).sliceView().forceRender()
    for viewIndex in range(lm.tableViewCount):
        lm.tableWidget(viewIndex).tableView().repaint()
    for viewIndex in range(lm.plotViewCount):
        lm.plotWidget(viewIndex).plotView().repaint()


#
# IO
#


def loadNodeFromFile(filename, filetype=None, properties={}, returnNode=False):
    """Load node into the scene from a file.

    :param filename: full path of the file to load.
    :param filetype: specifies the file type, which determines which IO class will load the file.
                     If not specified then the reader with the highest confidence is used.
    :param properties: map containing additional parameters for the loading.
    :param returnNode: Deprecated. If set to true then the method returns status flag and node
      instead of signalling error by throwing an exception.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.
    :raises RuntimeError: in case of failure
    """
    from slicer import app, vtkMRMLMessageCollection
    from vtk import vtkCollection

    # We need to convert the path to string now, because Qt cannot convert a pathlib.Path object to string.
    properties["fileName"] = str(filename)

    if filetype is None:
        filetype = app.coreIOManager().fileType(filename)

    loadedNodesCollection = vtkCollection()
    userMessages = vtkMRMLMessageCollection()
    success = app.coreIOManager().loadNodes(filetype, properties, loadedNodesCollection, userMessages)
    loadedNode = loadedNodesCollection.GetItemAsObject(0) if loadedNodesCollection.GetNumberOfItems() > 0 else None

    # Deprecated way of returning status and node
    if returnNode:
        import logging
        logging.warning("loadNodeFromFile `returnNode` argument is deprecated. Loaded node is now returned directly if `returnNode` is not specified.")
        import traceback

        logging.debug("loadNodeFromFile was called from " + ("".join(traceback.format_stack())))
        return success, loadedNode

    if not success:
        errorMessage = f"Failed to load node from file: {filename}"
        if userMessages.GetNumberOfMessages() > 0:
            errorMessage += "\n" + userMessages.GetAllMessagesAsString()
        raise RuntimeError(errorMessage)

    return loadedNode


def loadNodesFromFile(filename, filetype=None, properties={}, returnNode=False):
    """Load nodes into the scene from a file.

    It differs from `loadNodeFromFile` in that it returns loaded node(s) in an iterator.

    :param filename: full path of the file to load.
    :param filetype: specifies the file type, which determines which IO class will load the file.
                     If not specified then the reader with the highest confidence is used.
    :param properties: map containing additional parameters for the loading.
    :return: loaded node(s) in an iterator object.
    :raises RuntimeError: in case of failure
    """
    from slicer import app, vtkMRMLMessageCollection
    from vtk import vtkCollection

    # We need to convert the path to string now, because Qt cannot convert a pathlib.Path object to string.
    properties["fileName"] = str(filename)

    loadedNodesCollection = vtkCollection()
    userMessages = vtkMRMLMessageCollection()
    if filetype is None:
        filetype = app.coreIOManager().fileType(filename)
    success = app.coreIOManager().loadNodes(filetype, properties, loadedNodesCollection, userMessages)
    if not success:
        errorMessage = f"Failed to load node from file: {filename}"
        if userMessages.GetNumberOfMessages() > 0:
            errorMessage += "\n" + userMessages.GetAllMessagesAsString()
        raise RuntimeError(errorMessage)

    return iter(loadedNodesCollection)


def loadColorTable(filename, returnNode=False):
    """Load node from file.

    :param filename: full path of the file to load.
    :param returnNode: Deprecated.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.
    """
    return loadNodeFromFile(filename, "ColorTableFile", {}, returnNode)


def loadFiberBundle(filename, returnNode=False):
    """Load fiber bundle node from file.

    .. warning::

      To ensure the FiberBundleFile reader is registered, the ``SlicerDMRI``
      extension may need to be installed.

    :param filename: full path of the file to load.
    :param returnNode: Deprecated.

    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.

    :raises RuntimeError: in case of failure
    """
    from slicer import app

    readerType = "FiberBundleFile"

    # Check if the appropriate reader is registered
    if app.ioManager().registeredFileReaderCount(readerType) == 0:
        errorMessage = f"{readerType} reader not registered: Failed to load node from file: {filename}"

        extensionName = "SlicerDMRI"
        moduleName = "TractographyDisplay"
        readerClassName = "qSlicerFiberBundleReader"

        errorMessage += (
            f"\n\n{readerType} reader is implemented in the {readerClassName} class expected to be "
            f"registered by the {moduleName} module provided by the {extensionName} extension."
            "\n\nStatus:"
        )

        if app.moduleManager().module(moduleName) is None:
            errorMessage += f"\n- {moduleName} module: Not loaded."

        if app.applicationName == "Slicer":
            # Check if the extension is installed
            em = app.extensionsManagerModel()
            if not em.isExtensionInstalled(extensionName):
                errorMessage += (
                    f"\n- {extensionName} extension: Not installed."
                    "\n\nPlease install the extension for proper functionality."
                )

        raise RuntimeError(errorMessage)

    return loadNodeFromFile(filename, readerType, {}, returnNode)


def loadAnnotationFiducial(filename, returnNode=False):
    """Load node from file.

    :param filename: full path of the file to load.
    :param returnNode: Deprecated.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.
    """
    return loadNodeFromFile(filename, "AnnotationFile", {"fiducial": 1}, returnNode)


def loadAnnotationRuler(filename, returnNode=False):
    """Load node from file.

    :param filename: full path of the file to load.
    :param returnNode: Deprecated.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.
    """
    return loadNodeFromFile(filename, "AnnotationFile", {"ruler": 1}, returnNode)


def loadAnnotationROI(filename, returnNode=False):
    """Load node from file.

    :param filename: full path of the file to load.
    :param returnNode: Deprecated.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.
    """
    return loadNodeFromFile(filename, "AnnotationFile", {"roi": 1}, returnNode)


def loadMarkupsFiducialList(filename, returnNode=False):
    """Load markups fiducials from file.

    .. deprecated:: 4.13.0
      Use the universal :func:`loadMarkups` function instead.
    """
    if returnNode:
        return loadMarkups(filename)
    else:
        node = loadMarkups(filename)
        return [node is not None, node]


def loadMarkupsCurve(filename):
    """Load markups curve from file.

    .. deprecated:: 4.13.0
      Use the universal :func:`loadMarkups` function instead.
    """
    return loadMarkups(filename)


def loadMarkupsClosedCurve(filename):
    """Load markups closed curve from file.

    .. deprecated:: 4.13.0
      Use the universal :func:`loadMarkups` function instead.
    """
    return loadMarkups(filename)


def loadMarkups(filename):
    """Load node from file.

    :param filename: full path of the file to load.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
    """
    return loadNodeFromFile(filename, "MarkupsFile")


def loadModel(filename, returnNode=False):
    """Load node from file.

    :param filename: full path of the file to load.
    :param returnNode: Deprecated.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.
    """
    return loadNodeFromFile(filename, "ModelFile", {}, returnNode)


def loadScalarOverlay(filename, modelNodeID, returnNode=False):
    """Load node from file.

    :param filename: full path of the file to load.
    :param returnNode: Deprecated.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.
    """
    return loadNodeFromFile(filename, "ScalarOverlayFile", {"modelNodeId": modelNodeID}, returnNode)


def loadSegmentation(filename, properties={}, returnNode=False):
    """Load node from file.

    :param filename: full path of the file to load.
    :param properties: dict object with any of the following keys

        - name: this name will be used as node name for the loaded volume
        - autoOpacities: automatically make large segments semi-transparent to make segments inside more visible
          (only used when loading segmentation from image file)
        - colorNodeID: use a color node (that already in the scene) to display the image
          (only used when loading segmentation from image file)
    :param returnNode: Deprecated.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.
    """
    return loadNodeFromFile(filename, "SegmentationFile", properties, returnNode)


def loadTransform(filename, returnNode=False):
    """Load node from file.

    :param filename: full path of the file to load.
    :param returnNode: Deprecated.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.
    """
    return loadNodeFromFile(filename, "TransformFile", {}, returnNode)


def loadTable(filename):
    """Load table node from file.

    :param filename: full path of the file to load.
    :return: loaded table node
    """
    return loadNodeFromFile(filename, "TableFile")


def loadLabelVolume(filename, properties={}, returnNode=False):
    """Load node from file.

    :param filename: full path of the file to load.
    :param returnNode: Deprecated.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.
    """
    properties["labelmap"] = True
    return loadNodeFromFile(filename, "VolumeFile", properties, returnNode)


def loadShaderProperty(filename, returnNode=False):
    """Load node from file.

    :param filename: full path of the file to load.
    :param returnNode: Deprecated.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.
    """
    return loadNodeFromFile(filename, "ShaderPropertyFile", {}, returnNode)


def loadText(filename):
    """Load node from file.

    :param filename: full path of the text file to load.
    :return: loaded text node.
    """
    return loadNodeFromFile(filename, "TextFile")


def loadVolume(filename, properties={}, returnNode=False):
    """Load node from file.

    :param filename: full path of the file to load.
    :param properties: dict object with any of the following keys
      - name: this name will be used as node name for the loaded volume
      - labelmap: interpret volume as labelmap
      - singleFile: ignore all other files in the directory
      - center: ignore image position
      - discardOrientation: ignore image axis directions
      - autoWindowLevel: compute window/level automatically
      - show: display volume in slice viewers after loading is completed
      - colorNodeID: use a color node (that already in the scene) to display the image
      - fileNames: list of filenames to load the volume from
    :param returnNode: Deprecated.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.
    """
    filetype = "VolumeFile"
    return loadNodeFromFile(filename, filetype, properties, returnNode)


def loadSequence(filename, properties={}):
    """Load sequence (4D data set) from file.

    :param filename: full path of the file to load.
    :param properties: dict object with any of the following keys
      - name: this name will be used as node name for the loaded volume
      - show: display volume in slice viewers after loading is completed
      - colorNodeID: color node to set in the proxy nodes's display node
    :return: loaded sequence node.
    """
    filetype = "SequenceFile"
    return loadNodeFromFile(filename, filetype, properties)


def loadScene(filename, properties={}):
    """Load node from file.

    :param filename: full path of the file to load.
    :param returnNode: Deprecated.
    :return: loaded node (if multiple nodes are loaded then a list of nodes).
      If returnNode is True then a status flag and loaded node are returned.
    """
    filetype = "SceneFile"
    return loadNodeFromFile(filename, filetype, properties, returnNode=False)


def openAddDataDialog():
    from slicer import app

    return app.coreIOManager().openAddDataDialog()


def openAddVolumeDialog():
    from slicer import app

    return app.coreIOManager().openAddVolumeDialog()


def openAddModelDialog():
    from slicer import app

    return app.coreIOManager().openAddModelDialog()


def openAddScalarOverlayDialog():
    from slicer import app

    return app.coreIOManager().openAddScalarOverlayDialog()


def openAddSegmentationDialog():
    from slicer import app, qSlicerFileDialog

    return app.coreIOManager().openDialog("SegmentationFile", qSlicerFileDialog.Read)


def openAddTransformDialog():
    from slicer import app

    return app.coreIOManager().openAddTransformDialog()


def openAddColorTableDialog():
    from slicer import app

    return app.coreIOManager().openAddColorTableDialog()


def openAddFiducialDialog():
    from slicer import app

    return app.coreIOManager().openAddFiducialDialog()


def openAddMarkupsDialog():
    from slicer import app

    return app.coreIOManager().openAddMarkupsDialog()


def openAddFiberBundleDialog():
    from slicer import app

    return app.coreIOManager().openAddFiberBundleDialog()


def openAddShaderPropertyDialog():
    from slicer import app, qSlicerFileDialog

    return app.coreIOManager().openDialog("ShaderPropertyFile", qSlicerFileDialog.Read)


def openSaveDataDialog():
    from slicer import app

    return app.coreIOManager().openSaveDataDialog()


def saveNode(node, filename, properties={}):
    """Save 'node' data into 'filename'.

    It is the user responsibility to provide the appropriate file extension.

    User has also the possibility to overwrite the fileType internally retrieved using
    method 'qSlicerCoreIOManager::fileWriterFileType(vtkObject*)'. This can be done
    by specifying a 'fileType'attribute to the optional 'properties' dictionary.
    """
    from slicer import app, vtkMRMLMessageCollection

    properties["nodeID"] = node.GetID()
    properties["fileName"] = filename
    if hasattr(properties, "fileType"):
        filetype = properties["fileType"]
    else:
        filetype = app.coreIOManager().fileWriterFileType(node)
    userMessages = vtkMRMLMessageCollection()
    success = app.coreIOManager().saveNodes(filetype, properties, userMessages)

    if not success:
        import logging

        errorMessage = f"Failed to save node to file: {filename}"
        if userMessages.GetNumberOfMessages() > 0:
            errorMessage += "\n" + userMessages.GetAllMessagesAsString()
        logging.error(errorMessage)

    return success


def saveScene(filename, properties={}):
    """Save the current scene.

    Based on the value of 'filename', the current scene is saved either
    as a MRML file, MRB file or directory.

    If filename ends with '.mrml', the scene is saved as a single file
    without associated data.

    If filename ends with '.mrb', the scene is saved as a MRML bundle (Zip
    archive with scene and data files).

    In every other case, the scene is saved in the directory
    specified by 'filename'. Both MRML scene file and data
    will be written to disk. If needed, directories and sub-directories
    will be created.
    """
    from slicer import app, vtkMRMLMessageCollection

    filetype = "SceneFile"
    properties["fileName"] = filename
    userMessages = vtkMRMLMessageCollection()
    success = app.coreIOManager().saveNodes(filetype, properties, userMessages)

    if not success:
        import logging

        errorMessage = f"Failed to save scene to file: {filename}"
        if userMessages.GetNumberOfMessages() > 0:
            errorMessage += "\n" + userMessages.GetAllMessagesAsString()
        logging.error(errorMessage)

    return success


def exportNode(node, filename, properties={}, world=False):
    """Export 'node' data into 'filename'.

    If `world` is set to True then the node will be exported in the world coordinate system
    (equivalent to hardening the transform before exporting).

    This method is different from saveNode in that it does not modify any existing storage node
    and therefore does not change the filename or filetype that is used when saving the scene.
    """
    from slicer import app, vtkDataFileFormatHelper, vtkMRMLMessageCollection

    nodeIDs = [node.GetID()]
    fileNames = [filename]
    hardenTransform = world

    if "fileFormat" not in properties:
        foundFileFormat = None
        currentExtension = app.coreIOManager().extractKnownExtension(filename, node)
        fileWriterExtensions = app.coreIOManager().fileWriterExtensions(node)
        for fileFormat in fileWriterExtensions:
            extension = vtkDataFileFormatHelper.GetFileExtensionFromFormatString(fileFormat)
            if extension == currentExtension:
                foundFileFormat = fileFormat
                break
        if not foundFileFormat:
            raise ValueError(f"Failed to export {node.GetID()} - no known file format was found for filename {filename}")
        properties["fileFormat"] = foundFileFormat

    userMessages = vtkMRMLMessageCollection()
    success = app.coreIOManager().exportNodes(nodeIDs, fileNames, properties, hardenTransform, userMessages)

    if not success:
        import logging

        errorMessage = f"Failed to export node to file: {filename}"
        if userMessages.GetNumberOfMessages() > 0:
            errorMessage += "\n" + userMessages.GetAllMessagesAsString()
        logging.error(errorMessage)

    return success


#
# Module
#


def moduleSelector():
    """Return module selector widget.

    :return: module widget object
    :raises RuntimeError: if there is no module selector (for example, the application runs without a main window).
    """
    mw = mainWindow()
    if not mw:
        raise RuntimeError("Could not find main window")
    return mw.moduleSelector()


def selectModule(module):
    """Set currently active module.

    Throws a RuntimeError exception in case of failure (no such module or the application runs without a main window).

    :param module: module name or object
    :raises RuntimeError: in case of failure
    """
    moduleName = module
    if not isinstance(module, str):
        moduleName = module.name
    selector = moduleSelector()
    if not selector:
        raise RuntimeError("Could not find moduleSelector in the main window")
    moduleSelector().selectModule(moduleName)


def selectedModule():
    """Return currently active module.

    :return: module object
    :raises RuntimeError: in case of failure (no such module or the application runs without a main window).
    """
    selector = moduleSelector()
    if not selector:
        raise RuntimeError("Could not find moduleSelector in the main window")
    return selector.selectedModule


def moduleNames():
    """Get list containing name of all successfully loaded modules.

    :return: list of module names
    """
    from slicer import app

    return app.moduleManager().factoryManager().loadedModuleNames()


def getModule(moduleName):
    """Get module object from module name.

    :return: module object
    :raises RuntimeError: in case of failure (no such module).
    """
    from slicer import app

    module = app.moduleManager().module(moduleName)
    if not module:
        raise RuntimeError("Could not find module with name '%s'" % moduleName)
    return module


def getModuleGui(module):
    """Get module widget.

    .. deprecated:: 4.13.0
      Use the universal :func:`getModuleWidget` function instead.
    """
    return getModuleWidget(module)


def getNewModuleGui(module):
    """Create new module widget.

    .. deprecated:: 4.13.0
      Use the universal :func:`getNewModuleWidget` function instead.
    """
    return getNewModuleWidget(module)


def getModuleWidget(module):
    """Return module widget (user interface) object for a module.

    :param module: module name or module object
    :return: module widget object
    :raises RuntimeError: if the module does not have widget.
    """
    import slicer

    if isinstance(module, str):
        module = getModule(module)
    widgetRepr = module.widgetRepresentation()
    if not widgetRepr:
        raise RuntimeError("Could not find module widget representation with name '%s'" % module.name)
    if isinstance(widgetRepr, slicer.qSlicerScriptedLoadableModuleWidget):
        # Scripted module, return the Python class
        return widgetRepr.self()
    else:
        # C++ module
        return widgetRepr


def getNewModuleWidget(module):
    """Create new module widget instance.

    In general, not recommended, as module widget may be developed expecting that there is only a single
    instance of this widget. Instead, of instantiating a complete module GUI, it is recommended to create
    only selected widgets that are used in the module GUI.

    :param module: module name or module object
    :return: module widget object
    :raises RuntimeError: if the module does not have widget.
    """
    import slicer

    if isinstance(module, str):
        module = getModule(module)
    widgetRepr = module.createNewWidgetRepresentation()
    if not widgetRepr:
        raise RuntimeError("Could not find module widget representation with name '%s'" % module.name)
    if isinstance(widgetRepr, slicer.qSlicerScriptedLoadableModuleWidget):
        # Scripted module, return the Python class
        return widgetRepr.self()
    else:
        # C++ module
        return widgetRepr


def getModuleLogic(module):
    """Get module logic object.

    Module logic allows a module to use features offered by another module.

    :param module: module name or module object
    :return: module logic object
    :raises RuntimeError: if the module does not have widget.
    """
    import slicer

    if isinstance(module, str):
        module = getModule(module)
    if isinstance(module, slicer.qSlicerScriptedLoadableModule):
        try:
            logic = getModuleGui(module).logic
        except AttributeError:
            # This widget does not have a logic instance in its widget class
            logic = None
    else:
        logic = module.logic()
    if not logic:
        raise RuntimeError("Could not find module widget representation with name '%s'" % module.name)
    return logic


def modulePath(moduleName):
    """Return the path where the module was discovered and loaded from.

    :param moduleName: module name
    :return: file path of the module
    """
    import slicer  # noqa: F401

    return eval("slicer.modules.%s.path" % moduleName.lower())


def reloadScriptedModule(moduleName):
    """Generic reload method for any scripted module.

    The function performs the following:

    * Ensure ``sys.path`` includes the module path and use ``importlib`` to load the associated script.
    * For the current module widget representation:

      * Hide all children widgets
      * Call ``cleanup()`` function and disconnect ``ScriptedLoadableModuleWidget_onModuleAboutToBeUnloaded``
      * Remove layout items

    * Instantiate new widget representation
    * Call ``setup()`` function
    * Update ``slicer.modules.<moduleName>Widget`` attribute
    """
    import importlib.util
    import sys, os
    import slicer

    widgetName = moduleName + "Widget"

    # reload the source code
    filePath = modulePath(moduleName)
    p = os.path.dirname(filePath)

    if p not in sys.path:
        sys.path.insert(0, p)

    # Use importlib to load the module from file path
    spec = importlib.util.spec_from_file_location(moduleName, filePath)
    reloaded_module = importlib.util.module_from_spec(spec)
    sys.modules[moduleName] = reloaded_module
    spec.loader.exec_module(reloaded_module)

    # find and hide the existing widget
    parent = eval("slicer.modules.%s.widgetRepresentation()" % moduleName.lower())
    for child in parent.children():
        try:
            child.hide()
        except AttributeError:
            pass

    # if the module widget has been instantiated, call cleanup function and
    # disconnect "_onModuleAboutToBeUnloaded" (this avoids double-cleanup on
    # application exit)
    if hasattr(slicer.modules, widgetName):
        widget = getattr(slicer.modules, widgetName)
        widget.cleanup()

        if hasattr(widget, "_onModuleAboutToBeUnloaded"):
            slicer.app.moduleManager().disconnect("moduleAboutToBeUnloaded(QString)", widget._onModuleAboutToBeUnloaded)

    # remove layout items (remaining spacer items would add space above the widget)
    items = []
    for itemIndex in range(parent.layout().count()):
        items.append(parent.layout().itemAt(itemIndex))
    for item in items:
        parent.layout().removeItem(item)

    # Creates new widget at slicer.modules.{widgetName}.
    # Also ensures that qSlicerScriptedLoadableModuleWidget has references to updated enter/exit/setup methods.
    # See https://github.com/Slicer/Slicer/issues/7424
    widget.parent.reload()

    return reloaded_module


def setModulePanelTitleVisible(visible):
    """Show/hide module panel title bar at the top of module panel.

    If the title bar is not visible then it is not possible to drag and dock the
    module panel to a different location.

    If there is no main window then the function has no effect.
    """
    mw = mainWindow()
    if mw is None:
        return
    modulePanelDockWidget = mw.findChildren("QDockWidget", "PanelDockWidget")[0]
    if visible:
        modulePanelDockWidget.setTitleBarWidget(None)
    else:
        import qt

        modulePanelDockWidget.setTitleBarWidget(qt.QWidget(modulePanelDockWidget))


def setApplicationLogoVisible(visible=True, scaleFactor=None, icon=None):
    """Customize appearance of the application logo at the top of module panel.

    :param visible: if True then the logo is displayed, otherwise the area is left empty.
    :param scaleFactor: specifies the displayed size of the icon. 1.0 means original size, larger value means larger displayed size.
    :param icon: a qt.QIcon object specifying what icon to display as application logo.

    If there is no main window then the function has no effect.
    """

    mw = mainWindow()
    if mw is None:
        return
    logoLabel = findChild(mw, "LogoLabel")

    if icon is not None or scaleFactor is not None:
        if icon is None:
            import qt

            icon = qt.QIcon(":/ModulePanelLogo.png")
        if scaleFactor is None:
            scaleFactor = 1.0
        logo = icon.pixmap(icon.availableSizes()[0] * scaleFactor)
        logoLabel.setPixmap(logo)

    logoLabel.setVisible(visible)


def setModuleHelpSectionVisible(visible):
    """Show/hide Help section at the top of module panel.

    If there is no main window then the function has no effect.
    """
    mw = mainWindow()
    if mw is None:
        return
    modulePanel = findChild(mw, "ModulePanel")
    modulePanel.helpAndAcknowledgmentVisible = visible


def setDataProbeVisible(visible):
    """Show/hide Data probe at the bottom of module panel.

    If there is no main window then the function has no effect.
    """
    mw = mainWindow()
    if mw is None:
        return
    widget = findChild(mw, "DataProbeCollapsibleWidget")
    widget.setVisible(visible)


#
# Layout
#


def resetThreeDViews():
    """Reset focal view around volumes"""
    import slicer

    slicer.app.layoutManager().resetThreeDViews()


def resetSliceViews():
    """Reset focal view around volumes"""
    import slicer

    manager = slicer.app.layoutManager().resetSliceViews()


#
# MRML
#


class MRMLNodeNotFoundException(Exception):
    """Exception raised when a requested MRML node was not found."""

    pass


def getNodes(pattern="*", scene=None, useLists=False):
    """Return a dictionary of nodes where the name or id matches the ``pattern``.

    By default, ``pattern`` is a wildcard and it returns all nodes associated
    with ``slicer.mrmlScene``.

    If multiple node share the same name, using ``useLists=False`` (default behavior)
    returns only the last node with that name. If ``useLists=True``, it returns
    a dictionary of lists of nodes.
    """
    import slicer, collections, fnmatch

    nodes = collections.OrderedDict()
    if scene is None:
        scene = slicer.mrmlScene
    count = scene.GetNumberOfNodes()
    for idx in range(count):
        node = scene.GetNthNode(idx)
        name = node.GetName()
        id = node.GetID()
        if fnmatch.fnmatchcase(name, pattern) or fnmatch.fnmatchcase(id, pattern):
            if useLists:
                nodes.setdefault(node.GetName(), []).append(node)
            else:
                nodes[node.GetName()] = node
    return nodes


def getNode(pattern="*", index=0, scene=None):
    """Return the indexth node where name or id matches ``pattern``.

    By default, ``pattern`` is a wildcard and it returns the first node
    associated with ``slicer.mrmlScene``.

    This function is only intended for quick access to nodes for testing and troubleshooting, due to two important limitations.
    1. Due to the filename pattern matching uses some special characters (`*?[]`) it may be difficult to use it for nodes
      that have these characters in their names.
    2. In a scene often several nodes have the same name and this function only returns the first one.

    If a node name uses special characters, or if there are multiple nodes with the same name, then you can use the node ID as input
    or use :py:meth:`getFirstNodeByClassByName` to get the first node of a specific class.

    :raises MRMLNodeNotFoundException: if no node is found
     that matches the specified pattern.
    """
    nodes = getNodes(pattern, scene)
    if not nodes:
        raise MRMLNodeNotFoundException("could not find nodes in the scene by name or id '%s'" % (pattern if (isinstance(pattern, str)) else ""))
    return list(nodes.values())[index]


def getNodesByClass(className, scene=None):
    """Return all nodes in the scene of the specified class."""
    import slicer

    if scene is None:
        scene = slicer.mrmlScene
    nodes = slicer.mrmlScene.GetNodesByClass(className)
    nodes.UnRegister(slicer.mrmlScene)
    nodeList = []
    nodes.InitTraversal()
    node = nodes.GetNextItemAsObject()
    while node:
        nodeList.append(node)
        node = nodes.GetNextItemAsObject()
    return nodeList


def getFirstNodeByClassByName(className, name, scene=None):
    """Return the first node in the scene that matches the specified node name and node class."""
    import slicer

    if scene is None:
        scene = slicer.mrmlScene
    return scene.GetFirstNode(name, className)


def getFirstNodeByName(name, className=None):
    """Get the first MRML node that name starts with the specified name.

    Optionally specify a classname that must also match.
    """
    import slicer

    scene = slicer.mrmlScene
    return scene.GetFirstNode(name, className, False, False)


class NodeModify:
    """Context manager to conveniently compress mrml node modified event."""

    def __init__(self, node):
        self.node = node

    def __enter__(self):
        self.wasModifying = self.node.StartModify()
        return self.node

    def __exit__(self, type, value, traceback):
        self.node.EndModify(self.wasModifying)


class RenderBlocker:
    """
    Context manager to conveniently pause and resume view rendering. This makes sure that we are not displaying incomplete states to the user.
    Pausing the views can be useful to improve performance and ensure consistency by skipping all rendering calls until the current code block has completed.

    Code blocks such as::

      try:
        slicer.app.pauseRender()
        # Do things
      finally:
        slicer.app.resumeRender()

    Can be written as::

      with slicer.util.RenderBlocker():
        # Do things

    """

    def __enter__(self):
        import slicer

        slicer.app.pauseRender()

    def __exit__(self, type, value, traceback):
        import slicer

        slicer.app.resumeRender()


#
# Subject hierarchy
#
def getSubjectHierarchyItemChildren(parentItem=None, recursive=False):
    """Convenience method to get children of a subject hierarchy item.

    :param vtkIdType parentItem: Item for which to get children for. If omitted
           or None then use scene item (i.e. get all items)
    :param bool recursive: Whether the query is recursive. False by default
    :return: List of child item IDs
    """
    import slicer, vtk

    children = []
    shNode = slicer.mrmlScene.GetSubjectHierarchyNode()
    # Use scene as parent item if not given
    if not parentItem:
        parentItem = shNode.GetSceneItemID()
    childrenIdList = vtk.vtkIdList()
    shNode.GetItemChildren(parentItem, childrenIdList, recursive)
    for childIndex in range(childrenIdList.GetNumberOfIds()):
        children.append(childrenIdList.GetId(childIndex))
    return children


#
# MRML-numpy
#


def array(pattern="", index=0):
    """Return the array you are "most likely to want" from the indexth

    MRML node that matches the pattern.

    :raises RuntimeError: if the node cannot be accessed as an array.

    .. warning::

      Meant to be used in the python console for quick debugging/testing.

    More specific API should be used in scripts to be sure you get exactly
    what you want, such as :py:meth:`arrayFromVolume`, :py:meth:`arrayFromModelPoints`,
    and :py:meth:`arrayFromGridTransform`.
    """
    node = getNode(pattern=pattern, index=index)
    import slicer

    if isinstance(node, slicer.vtkMRMLVolumeNode):
        return arrayFromVolume(node)
    elif isinstance(node, slicer.vtkMRMLModelNode):
        return arrayFromModelPoints(node)
    elif isinstance(node, slicer.vtkMRMLGridTransformNode):
        return arrayFromGridTransform(node)
    elif isinstance(node, slicer.vtkMRMLMarkupsNode):
        return arrayFromMarkupsControlPoints(node)
    elif isinstance(node, slicer.vtkMRMLTransformNode):
        return arrayFromTransformMatrix(node)

    # TODO: accessors for other node types: polydata (verts, polys...), colors
    raise RuntimeError("Cannot get node " + node.GetID() + " as array")


def arrayFromVolume(volumeNode):
    """Return voxel array from volume node as numpy array.

    Voxels values are not copied. Voxel values in the volume node can be modified
    by changing values in the numpy array.
    After all modifications has been completed, call :py:meth:`arrayFromVolumeModified`.

    :raises RuntimeError: in case of failure

    .. warning:: Memory area of the returned array is managed by VTK, therefore
      values in the array may be changed, but the array must not be reallocated
      (change array size, shallow-copy content from other array most likely causes
      application crash). To allow arbitrary numpy operations on a volume array:

        1. Make a deep-copy of the returned VTK-managed array using :func:`numpy.copy`.
        2. Perform any computations using the copied array.
        3. Write results back to the image data using :py:meth:`updateVolumeFromArray`.
    """
    scalarTypes = ["vtkMRMLScalarVolumeNode", "vtkMRMLLabelMapVolumeNode"]
    vectorTypes = ["vtkMRMLVectorVolumeNode", "vtkMRMLMultiVolumeNode", "vtkMRMLDiffusionWeightedVolumeNode"]
    tensorTypes = ["vtkMRMLDiffusionTensorVolumeNode"]
    vimage = volumeNode.GetImageData()
    nshape = tuple(reversed(volumeNode.GetImageData().GetDimensions()))
    import vtk.util.numpy_support

    narray = None
    if volumeNode.GetClassName() in scalarTypes:
        narray = vtk.util.numpy_support.vtk_to_numpy(vimage.GetPointData().GetScalars()).reshape(nshape)
    elif volumeNode.GetClassName() in vectorTypes:
        components = vimage.GetNumberOfScalarComponents()
        if components > 1:
            nshape = nshape + (components,)
        narray = vtk.util.numpy_support.vtk_to_numpy(vimage.GetPointData().GetScalars()).reshape(nshape)
    elif volumeNode.GetClassName() in tensorTypes:
        narray = vtk.util.numpy_support.vtk_to_numpy(vimage.GetPointData().GetTensors()).reshape(nshape + (3, 3))
    else:
        raise RuntimeError("Unsupported volume type: " + volumeNode.GetClassName())
    return narray


def arrayFromVolumeModified(volumeNode):
    """Indicate that modification of a numpy array returned by :py:meth:`arrayFromVolume` has been completed."""
    imageData = volumeNode.GetImageData()
    pointData = imageData.GetPointData() if imageData else None
    if pointData:
        if pointData.GetScalars():
            pointData.GetScalars().Modified()
        if pointData.GetTensors():
            pointData.GetTensors().Modified()
    volumeNode.Modified()


def arrayFromModelPoints(modelNode):
    """Return point positions of a model node as numpy array.

    Point coordinates can be modified by modifying the numpy array.
    After all modifications has been completed, call :py:meth:`arrayFromModelPointsModified`.

    .. warning:: Important: memory area of the returned array is managed by VTK,
      therefore values in the array may be changed, but the array must not be reallocated.
      See :py:meth:`arrayFromVolume` for details.
    """
    import vtk.util.numpy_support

    pointData = modelNode.GetMesh().GetPoints().GetData()
    narray = vtk.util.numpy_support.vtk_to_numpy(pointData)
    return narray


def arrayFromModelPointsModified(modelNode):
    """Indicate that modification of a numpy array returned by :py:meth:`arrayFromModelPoints` has been completed."""
    if modelNode.GetMesh():
        modelNode.GetMesh().GetPoints().GetData().Modified()
    # Trigger re-render
    modelNode.GetDisplayNode().Modified()


def _vtkArrayFromModelData(modelNode, arrayName, location):
    """Helper function for getting VTK point data array that throws exception

    with informative error message if the data array is not found.
    Point or cell data can be selected by setting 'location' argument to 'point' or 'cell'.

    :raises ValueError: in case of failure
    """
    if location == "point":
        modelData = modelNode.GetMesh().GetPointData()
    elif location == "cell":
        modelData = modelNode.GetMesh().GetCellData()
    else:
        raise ValueError("Location attribute must be set to 'point' or 'cell'")
    if not modelData or modelData.GetNumberOfArrays() == 0:
        raise ValueError(f"Input modelNode does not contain {location} data")
    arrayVtk = modelData.GetArray(arrayName)
    if not arrayVtk:
        availableArrayNames = [modelData.GetArrayName(i) for i in range(modelData.GetNumberOfArrays())]
        raise ValueError("Input modelNode does not contain {} data array '{}'. Available array names: '{}'".format(
            location, arrayName, "', '".join(availableArrayNames)))
    return arrayVtk


def arrayFromModelPointData(modelNode, arrayName):
    """Return point data array of a model node as numpy array.

    .. warning:: Important: memory area of the returned array is managed by VTK,
      therefore values in the array may be changed, but the array must not be reallocated.
      See :py:meth:`arrayFromVolume` for details.
    """
    import vtk.util.numpy_support

    arrayVtk = _vtkArrayFromModelData(modelNode, arrayName, "point")
    narray = vtk.util.numpy_support.vtk_to_numpy(arrayVtk)
    return narray


def arrayFromModelPointDataModified(modelNode, arrayName):
    """Indicate that modification of a numpy array returned by :py:meth:`arrayFromModelPointData` has been completed."""
    arrayVtk = _vtkArrayFromModelData(modelNode, arrayName, "point")
    arrayVtk.Modified()


def arrayFromModelCellData(modelNode, arrayName):
    """Return cell data array of a model node as numpy array.

    .. warning:: Important: memory area of the returned array is managed by VTK,
      therefore values in the array may be changed, but the array must not be reallocated.
      See :py:meth:`arrayFromVolume` for details.
    """
    import vtk.util.numpy_support

    arrayVtk = _vtkArrayFromModelData(modelNode, arrayName, "cell")
    narray = vtk.util.numpy_support.vtk_to_numpy(arrayVtk)
    return narray


def arrayFromModelCellDataModified(modelNode, arrayName):
    """Indicate that modification of a numpy array returned by :py:meth:`arrayFromModelCellData` has been completed."""
    arrayVtk = _vtkArrayFromModelData(modelNode, arrayName, "cell")
    arrayVtk.Modified()


def arrayFromMarkupsControlPointData(markupsNode, arrayName):
    """Return control point data array of a markups node as numpy array.

    .. warning:: Important: memory area of the returned array is managed by VTK,
      therefore values in the array may be changed, but the array must not be reallocated.
      See :py:meth:`arrayFromVolume` for details.
    """
    import vtk.util.numpy_support

    for measurementIndex in range(markupsNode.GetNumberOfMeasurements()):
        measurement = markupsNode.GetNthMeasurement(measurementIndex)
        doubleArrayVtk = measurement.GetControlPointValues()
        if doubleArrayVtk and doubleArrayVtk.GetName() == arrayName:
            narray = vtk.util.numpy_support.vtk_to_numpy(doubleArrayVtk)
            return narray


def arrayFromMarkupsControlPointDataModified(markupsNode, arrayName):
    """Indicate that modification of a numpy array returned by :py:meth:`arrayFromMarkupsControlPointData` has been completed."""
    for measurementIndex in range(markupsNode.GetNumberOfMeasurements()):
        measurement = markupsNode.GetNthMeasurement(measurementIndex)
        doubleArrayVtk = measurement.GetControlPointValues()
        if doubleArrayVtk and doubleArrayVtk.GetName() == arrayName:
            doubleArrayVtk.Modified()


def arrayFromModelPolyIds(modelNode):
    """Return poly id array of a model node as numpy array.

    These ids are the following format:
    [ n(0), i(0,0), i(0,1), ... i(0,n(00),..., n(j), i(j,0), ... i(j,n(j))...]
    where n(j) is the number of vertices in polygon j
    and i(j,k) is the index into the vertex array for vertex k of poly j.

    As described here:
    https://vtk.org/wp-content/uploads/2015/04/file-formats.pdf

    Typically in Slicer n(j) will always be 3 because a model node's
    polygons will be triangles.

    .. warning:: Important: memory area of the returned array is managed by VTK,
      therefore values in the array may be changed, but the array must not be reallocated.
      See :py:meth:`arrayFromVolume` for details.
    """
    import vtk.util.numpy_support

    arrayVtk = modelNode.GetPolyData().GetPolys().GetData()
    narray = vtk.util.numpy_support.vtk_to_numpy(arrayVtk)
    return narray


def arrayFromGridTransform(gridTransformNode):
    """Return voxel array from transform node as numpy array.

    Vector values are not copied. Values in the transform node can be modified
    by changing values in the numpy array.
    After all modifications has been completed, call :py:meth:`arrayFromGridTransformModified`.

    .. warning:: Important: memory area of the returned array is managed by VTK,
      therefore values in the array may be changed, but the array must not be reallocated.
      See :py:meth:`arrayFromVolume` for details.
    """
    transformGrid = gridTransformNode.GetTransformFromParent()
    displacementGrid = transformGrid.GetDisplacementGrid()
    nshape = tuple(reversed(displacementGrid.GetDimensions()))
    import vtk.util.numpy_support

    nshape = nshape + (3,)
    narray = vtk.util.numpy_support.vtk_to_numpy(displacementGrid.GetPointData().GetScalars()).reshape(nshape)
    return narray


def arrayFromVTKMatrix(vmatrix):
    """Return vtkMatrix4x4 or vtkMatrix3x3 elements as numpy array.

    :raises RuntimeError: in case of failure

    The returned array is just a copy and so any modification in the array will not affect the input matrix.
    To set VTK matrix from a numpy array, use :py:meth:`vtkMatrixFromArray` or
    :py:meth:`updateVTKMatrixFromArray`.
    """
    from vtk import vtkMatrix4x4
    from vtk import vtkMatrix3x3
    import numpy as np

    if isinstance(vmatrix, vtkMatrix4x4):
        matrixSize = 4
    elif isinstance(vmatrix, vtkMatrix3x3):
        matrixSize = 3
    else:
        raise RuntimeError("Input must be vtk.vtkMatrix3x3 or vtk.vtkMatrix4x4")
    narray = np.eye(matrixSize)
    vmatrix.DeepCopy(narray.ravel(), vmatrix)
    return narray


def vtkMatrixFromArray(narray):
    """Create VTK matrix from a 3x3 or 4x4 numpy array.

    :param narray: input numpy array
    :raises RuntimeError: in case of failure

    The returned matrix is just a copy and so any modification in the array will not affect the output matrix.
    To set numpy array from VTK matrix, use :py:meth:`arrayFromVTKMatrix`.
    """
    from vtk import vtkMatrix4x4
    from vtk import vtkMatrix3x3

    narrayshape = narray.shape
    if narrayshape == (4, 4):
        vmatrix = vtkMatrix4x4()
        updateVTKMatrixFromArray(vmatrix, narray)
        return vmatrix
    elif narrayshape == (3, 3):
        vmatrix = vtkMatrix3x3()
        updateVTKMatrixFromArray(vmatrix, narray)
        return vmatrix
    else:
        raise RuntimeError("Unsupported numpy array shape: " + str(narrayshape) + " expected (4,4)")


def updateVTKMatrixFromArray(vmatrix, narray):
    """Update VTK matrix values from a numpy array.

    :param vmatrix: VTK matrix (vtkMatrix4x4 or vtkMatrix3x3) that will be update
    :param narray: input numpy array
    :raises RuntimeError: in case of failure

    To set numpy array from VTK matrix, use :py:meth:`arrayFromVTKMatrix`.
    """
    from vtk import vtkMatrix4x4
    from vtk import vtkMatrix3x3

    if isinstance(vmatrix, vtkMatrix4x4):
        matrixSize = 4
    elif isinstance(vmatrix, vtkMatrix3x3):
        matrixSize = 3
    else:
        raise RuntimeError("Output vmatrix must be vtk.vtkMatrix3x3 or vtk.vtkMatrix4x4")
    if narray.shape != (matrixSize, matrixSize):
        raise RuntimeError("Input narray size must match output vmatrix size ({0}x{0})".format(matrixSize))
    vmatrix.DeepCopy(narray.ravel())


def arrayFromTransformMatrix(transformNode, toWorld=False):
    """Return 4x4 transformation matrix as numpy array.

    :param toWorld: if set to True then the transform to world coordinate system is returned
      (effect of parent transform to the node is applied), otherwise transform to parent transform is returned.
    :return: numpy array
    :raises RuntimeError: in case of failure

    The returned array is just a copy and so any modification in the array will not affect the transform node.

    To set transformation matrix from a numpy array, use :py:meth:`updateTransformMatrixFromArray`.
    """
    from vtk import vtkMatrix4x4

    vmatrix = vtkMatrix4x4()
    if toWorld:
        success = transformNode.GetMatrixTransformToWorld(vmatrix)
    else:
        success = transformNode.GetMatrixTransformToParent(vmatrix)
    if not success:
        raise RuntimeError("Failed to get transformation matrix from node " + transformNode.GetID())
    return arrayFromVTKMatrix(vmatrix)


def updateTransformMatrixFromArray(transformNode, narray, toWorld=False):
    """Set transformation matrix from a numpy array of size 4x4 (toParent).

    :param world: if set to True then the transform will be set so that transform
      to world matrix will be equal to narray; otherwise transform to parent will be
      set as narray.
    :raises RuntimeError: in case of failure
    """
    import numpy as np
    from vtk import vtkMatrix4x4

    narrayshape = narray.shape
    if narrayshape != (4, 4):
        raise RuntimeError("Unsupported numpy array shape: " + str(narrayshape) + " expected (4,4)")
    if toWorld and transformNode.GetParentTransformNode():
        # thisToParent = worldToParent * thisToWorld = inv(parentToWorld) * toWorld
        narrayParentToWorld = arrayFromTransformMatrix(transformNode.GetParentTransformNode())
        thisToParent = np.dot(np.linalg.inv(narrayParentToWorld), narray)
        updateTransformMatrixFromArray(transformNode, thisToParent, toWorld=False)
    else:
        vmatrix = vtkMatrix4x4()
        updateVTKMatrixFromArray(vmatrix, narray)
        transformNode.SetMatrixTransformToParent(vmatrix)


def arrayFromGridTransformModified(gridTransformNode):
    """Indicate that modification of a numpy array returned by :py:meth:`arrayFromGridTransform` has been completed."""
    transformGrid = gridTransformNode.GetTransformFromParent()
    displacementGrid = transformGrid.GetDisplacementGrid()
    displacementGrid.GetPointData().GetScalars().Modified()
    displacementGrid.Modified()


def arrayFromSegment(segmentationNode, segmentId):
    """Get segment as numpy array.

    .. warning:: Important: binary labelmap representation may be shared between multiple segments.

    .. deprecated:: 4.13.0
      Use arrayFromSegmentBinaryLabelmap to access a copy of the binary labelmap that will not modify the original labelmap."
      Use arrayFromSegmentInternalBinaryLabelmap to access a modifiable internal labelmap representation that may be shared"
      between multiple segments.
    """
    import logging
    logging.warning("arrayFromSegment is deprecated. Binary labelmap representation may be shared between multiple segments.")
    return arrayFromSegmentBinaryLabelmap(segmentationNode, segmentId)


def arrayFromSegmentInternalBinaryLabelmap(segmentationNode, segmentId):
    """Return voxel array of a segment's binary labelmap representation as numpy array.

    Voxels values are not copied.
    The labelmap containing the specified segment may be a shared labelmap containing multiple segments.

    To get and modify the array for a single segment, calling::

      segmentationNode->GetSegmentation()->SeparateSegment(segmentId)

    will transfer the segment from a shared labelmap into a new layer.

    Layers can be merged by calling::

      segmentationNode->GetSegmentation()->CollapseBinaryLabelmaps()

    If binary labelmap is the source representation then voxel values in the volume node can be modified
    by changing values in the numpy array. After all modifications has been completed, call::

      segmentationNode.GetSegmentation().GetSegment(segmentID).Modified()

    .. warning:: Important: memory area of the returned array is managed by VTK,
      therefore values in the array may be changed, but the array must not be reallocated.
      See :py:meth:`arrayFromVolume` for details.
    """
    vimage = segmentationNode.GetBinaryLabelmapInternalRepresentation(segmentId)
    nshape = tuple(reversed(vimage.GetDimensions()))
    import vtk.util.numpy_support

    narray = vtk.util.numpy_support.vtk_to_numpy(vimage.GetPointData().GetScalars()).reshape(nshape)
    return narray


def arrayFromSegmentBinaryLabelmap(segmentationNode, segmentId, referenceVolumeNode=None):
    """Return voxel array of a segment's binary labelmap representation as numpy array.

    :param segmentationNode: source segmentation node.
    :param segmentId: ID of the source segment.
      Can be determined from segment name by calling ``segmentationNode.GetSegmentation().GetSegmentIdBySegmentName(segmentName)``.
    :param referenceVolumeNode: a volume node that determines geometry (origin, spacing, axis directions, extents) of the array.
      If not specified then the volume that was used for setting the segmentation's geometry is used as reference volume.

    :raises RuntimeError: in case of failure

    Voxels values are copied, therefore changing the returned numpy array has no effect on the source segmentation.
    The modified array can be written back to the segmentation by calling :py:meth:`updateSegmentBinaryLabelmapFromArray`.

    To get voxels of a segment as a modifiable numpy array, you can use :py:meth:`arrayFromSegmentInternalBinaryLabelmap`.
    """

    import slicer
    import vtk

    # Get reference volume
    if not referenceVolumeNode:
        referenceVolumeNode = segmentationNode.GetNodeReference(slicer.vtkMRMLSegmentationNode.GetReferenceImageGeometryReferenceRole())
        if not referenceVolumeNode:
            raise RuntimeError("No reference volume is found in the input segmentationNode, therefore a valid referenceVolumeNode input is required.")

    # Export segment as vtkImageData (via temporary labelmap volume node)
    segmentIds = vtk.vtkStringArray()
    segmentIds.InsertNextValue(segmentId)
    labelmapVolumeNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLLabelMapVolumeNode", "__temp__")
    try:
        if not slicer.modules.segmentations.logic().ExportSegmentsToLabelmapNode(segmentationNode, segmentIds, labelmapVolumeNode, referenceVolumeNode):
            raise RuntimeError("Export of segment failed.")
        narray = slicer.util.arrayFromVolume(labelmapVolumeNode)
    finally:
        if labelmapVolumeNode.GetDisplayNode():
            slicer.mrmlScene.RemoveNode(labelmapVolumeNode.GetDisplayNode().GetColorNode())
        slicer.mrmlScene.RemoveNode(labelmapVolumeNode)

    return narray


def updateSegmentBinaryLabelmapFromArray(narray, segmentationNode, segmentId, referenceVolumeNode=None):
    """Sets binary labelmap representation of a segment from a numpy array.

    :param narray: voxel array, containing 0 outside the segment, 1 inside the segment.
    :param segmentationNode: segmentation node that will be updated.
    :param segmentId: ID of the segment that will be updated.
      Can be determined from segment name by calling ``segmentationNode.GetSegmentation().GetSegmentIdBySegmentName(segmentName)``.
    :param referenceVolumeNode: a volume node that determines geometry (origin, spacing, axis directions, extents) of the array.
      If not specified then the volume that was used for setting the segmentation's geometry is used as reference volume.

    :raises RuntimeError: in case of failure

    .. warning::
      Voxels values are deep-copied, therefore if the numpy array is modified after calling this method, segmentation node will not change.
    """

    # Export segment as vtkImageData (via temporary labelmap volume node)
    import slicer
    import vtk

    # Get reference volume
    if not referenceVolumeNode:
        referenceVolumeNode = segmentationNode.GetNodeReference(slicer.vtkMRMLSegmentationNode.GetReferenceImageGeometryReferenceRole())
        if not referenceVolumeNode:
            raise RuntimeError("No reference volume is found in the input segmentationNode, therefore a valid referenceVolumeNode input is required.")

    # Update segment in segmentation
    labelmapVolumeNode = slicer.modules.volumes.logic().CreateAndAddLabelVolume(referenceVolumeNode, "__temp__")
    try:
        if narray.min() >= 0 and narray.max() <= 1:
            # input array seems to be valid, use it as is (faster)
            updateVolumeFromArray(labelmapVolumeNode, narray)
        else:
            # need to normalize the data because the label value must be 1
            import numpy as np

            narrayNormalized = np.zeros(narray.shape, np.uint8)
            narrayNormalized[narray > 0] = 1
            updateVolumeFromArray(labelmapVolumeNode, narrayNormalized)
        segmentIds = vtk.vtkStringArray()
        segmentIds.InsertNextValue(segmentId)
        if not slicer.modules.segmentations.logic().ImportLabelmapToSegmentationNode(labelmapVolumeNode, segmentationNode, segmentIds):
            raise RuntimeError("Importing of segment failed.")
    finally:
        slicer.mrmlScene.RemoveNode(labelmapVolumeNode)


def arrayFromMarkupsControlPoints(markupsNode, world=False):
    """Return control point positions of a markups node as rows in a numpy array (of size Nx3).

    :param world: if set to True then the control points coordinates are returned in world coordinate system
      (effect of parent transform to the node is applied).

    The returned array is just a copy and so any modification in the array will not affect the markup node.

    To modify markup control points based on a numpy array, use :py:meth:`updateMarkupsControlPointsFromArray`.
    """
    numberOfControlPoints = markupsNode.GetNumberOfControlPoints()
    import numpy as np

    narray = np.zeros([numberOfControlPoints, 3])
    for controlPointIndex in range(numberOfControlPoints):
        if world:
            markupsNode.GetNthControlPointPositionWorld(controlPointIndex, narray[controlPointIndex, :])
        else:
            markupsNode.GetNthControlPointPosition(controlPointIndex, narray[controlPointIndex, :])
    return narray


def updateMarkupsControlPointsFromArray(markupsNode, narray, world=False):
    """Sets control point positions in a markups node from a numpy array of size Nx3.

    :param world: if set to True then the control point coordinates are expected in world coordinate system.
    :raises RuntimeError: in case of failure

    All previous content of the node is deleted.
    """
    narrayshape = narray.shape
    if narrayshape == (0,):
        markupsNode.RemoveAllControlPoints()
        return
    if len(narrayshape) != 2 or narrayshape[1] != 3:
        raise RuntimeError("Unsupported numpy array shape: " + str(narrayshape) + " expected (N,3)")
    numberOfControlPoints = narrayshape[0]
    oldNumberOfControlPoints = markupsNode.GetNumberOfControlPoints()
    # Update existing control points
    wasModify = markupsNode.StartModify()
    try:
        for controlPointIndex in range(min(numberOfControlPoints, oldNumberOfControlPoints)):
            if world:
                markupsNode.SetNthControlPointPositionWorld(controlPointIndex, narray[controlPointIndex, :])
            else:
                markupsNode.SetNthControlPointPosition(controlPointIndex, narray[controlPointIndex, :])
        if numberOfControlPoints >= oldNumberOfControlPoints:
            # Add new points to the markup node
            for controlPointIndex in range(oldNumberOfControlPoints, numberOfControlPoints):
                if world:
                    markupsNode.AddControlPointWorld(narray[controlPointIndex, :])
                else:
                    markupsNode.AddControlPoint(narray[controlPointIndex, :])
        else:
            # Remove extra point from the markup node
            for controlPointIndex in range(oldNumberOfControlPoints, numberOfControlPoints, -1):
                markupsNode.RemoveNthControlPoint(controlPointIndex - 1)
    finally:
        markupsNode.EndModify(wasModify)


def arrayFromMarkupsCurvePoints(markupsNode, world=False):
    """Return interpolated curve point positions of a markups node as rows in a numpy array (of size Nx3).

    :param world: if set to True then the point coordinates are returned in world coordinate system
      (effect of parent transform to the node is applied).

    The returned array is just a copy and so any modification in the array will not affect the markup node.
    """
    import vtk.util.numpy_support

    if world:
        pointData = markupsNode.GetCurvePointsWorld().GetData()
    else:
        pointData = markupsNode.GetCurvePoints().GetData()
    narray = vtk.util.numpy_support.vtk_to_numpy(pointData)
    return narray


def arrayFromMarkupsCurveData(markupsNode, arrayName, world=False):
    """Return curve measurement results from a markups node as a numpy array.

    :param markupsNode: node to get the curve point data from.
    :param arrayName: array name to get (for example `Curvature`)
    :param world: if set to True then the point coordinates are returned in world coordinate system
      (effect of parent transform to the node is applied).
    :raises ValueError: in case of failure

    .. warning::

      - Not all array may be available in both node and world coordinate systems.
        For example, `Curvature` is only computed for the curve in world coordinate system.

      - The returned array is not intended to be modified, as arrays are expected to be written only
        by measurement objects.
    """
    import vtk.util.numpy_support

    if world:
        curvePolyData = markupsNode.GetCurveWorld()
    else:
        curvePolyData = markupsNode.GetCurve()
    pointData = curvePolyData.GetPointData()
    if not pointData or pointData.GetNumberOfArrays() == 0:
        raise ValueError(f"Input markups curve does not contain point data")

    arrayVtk = pointData.GetArray(arrayName)
    if not arrayVtk:
        availableArrayNames = [pointData.GetArrayName(i) for i in range(pointData.GetNumberOfArrays())]
        raise ValueError("Input markupsNode does not contain curve point data array '{}'. Available array names: '{}'".format(
            arrayName, "', '".join(availableArrayNames)))

    narray = vtk.util.numpy_support.vtk_to_numpy(arrayVtk)
    return narray


def updateVolumeFromArray(volumeNode, narray):
    """Sets voxels of a volume node from a numpy array.

    :raises RuntimeError: in case of failure

    Voxels values are deep-copied, therefore if the numpy array
    is modified after calling this method, voxel values in the volume node will not change.
    Dimensions and voxel type of the source numpy array does not have to match the current
    content of the volume node.
    """

    vshape = tuple(reversed(narray.shape))
    if len(vshape) == 1:
        # Line of pixels
        vcomponents = 1
        # Put the slice into a single-slice 3D volume
        import numpy as np

        narray3d = np.zeros([1, 1, narray.shape[0]])
        narray3d[0, 0, :] = narray
        narray = narray3d
        vshape = tuple(reversed(narray.shape))
    elif len(vshape) == 2:
        # Scalar 2D volume
        vcomponents = 1
        # Put the slice into a single-slice 3D volume
        import numpy as np

        narray3d = np.zeros([1, narray.shape[0], narray.shape[1]])
        narray3d[0] = narray
        narray = narray3d
        vshape = tuple(reversed(narray.shape))
    elif len(vshape) == 3:
        # Scalar volume
        vcomponents = 1
    elif len(vshape) == 4:
        # Vector volume
        vcomponents = vshape[0]
        vshape = vshape[1:4]
    else:
        # TODO: add support for tensor volumes
        raise RuntimeError("Unsupported numpy array shape: " + str(narray.shape))

    vimage = volumeNode.GetImageData()
    if not vimage:
        import vtk

        vimage = vtk.vtkImageData()
        volumeNode.SetAndObserveImageData(vimage)
    import vtk.util.numpy_support

    vtype = vtk.util.numpy_support.get_vtk_array_type(narray.dtype)

    # Volumes with "long long" scalar type are not rendered correctly.
    # Probably this could be fixed in VTK or Slicer but for now just reject it.
    if vtype == vtk.VTK_LONG_LONG:
        raise RuntimeError("Unsupported numpy array type: long long")

    vimage.SetDimensions(vshape)
    vimage.AllocateScalars(vtype, vcomponents)

    narrayTarget = arrayFromVolume(volumeNode)
    narrayTarget[:] = narray

    # Notify the application that image data is changed
    # (same notifications as in vtkMRMLVolumeNode.SetImageDataConnection)
    import slicer

    volumeNode.StorableModified()
    volumeNode.Modified()
    volumeNode.InvokeEvent(slicer.vtkMRMLVolumeNode.ImageDataModifiedEvent, volumeNode)


def addVolumeFromArray(narray, ijkToRAS=None, name=None, nodeClassName=None):
    """Create a new volume node from content of a numpy array and add it to the scene.

    Voxels values are deep-copied, therefore if the numpy array
    is modified after calling this method, voxel values in the volume node will not change.

    :param narray: numpy array containing volume voxels.
    :param ijkToRAS: 4x4 numpy array or vtk.vtkMatrix4x4 that defines mapping from IJK to RAS coordinate system (specifying origin, spacing, directions)
    :param name: volume node name
    :param nodeClassName: type of created volume, default: ``vtkMRMLScalarVolumeNode``.
      Use ``vtkMRMLLabelMapVolumeNode`` for labelmap volume, ``vtkMRMLVectorVolumeNode`` for vector volume.
    :return: created new volume node

    Example::

      # create zero-filled volume
      import numpy as np
      volumeNode = slicer.util.addVolumeFromArray(np.zeros((30, 40, 50)))

    Example::

      # create labelmap volume filled with voxel value of 120
      import numpy as np
      volumeNode = slicer.util.addVolumeFromArray(np.ones((30, 40, 50), 'int8') * 120,
        np.diag([0.2, 0.2, 0.5, 1.0]), nodeClassName="vtkMRMLLabelMapVolumeNode")
    """
    import slicer
    from vtk import vtkMatrix4x4

    if name is None:
        name = ""
    if nodeClassName is None:
        nodeClassName = "vtkMRMLScalarVolumeNode"

    volumeNode = slicer.mrmlScene.AddNewNodeByClass(nodeClassName, name)
    if ijkToRAS is not None:
        if not isinstance(ijkToRAS, vtkMatrix4x4):
            ijkToRAS = vtkMatrixFromArray(ijkToRAS)
        volumeNode.SetIJKToRASMatrix(ijkToRAS)
    updateVolumeFromArray(volumeNode, narray)
    volumeNode.CreateDefaultDisplayNodes()

    return volumeNode


def arrayFromTableColumn(tableNode, columnName):
    """Return values of a table node's column as numpy array.

    Values can be modified by modifying the numpy array.
    After all modifications has been completed, call :py:meth:`arrayFromTableColumnModified`.

    .. warning:: Important: memory area of the returned array is managed by VTK,
      therefore values in the array may be changed, but the array must not be reallocated.
      See :py:meth:`arrayFromVolume` for details.
    """
    import vtk.util.numpy_support

    columnData = tableNode.GetTable().GetColumnByName(columnName)
    narray = vtk.util.numpy_support.vtk_to_numpy(columnData)
    return narray


def arrayFromTableColumnModified(tableNode, columnName):
    """Indicate that modification of a numpy array returned by :py:meth:`arrayFromTableColumn` has been completed."""
    columnData = tableNode.GetTable().GetColumnByName(columnName)
    columnData.Modified()
    tableNode.GetTable().Modified()


def _vtkArrayFromNumpyArray(narray, deep=False, arrayType=None):
    """
    Convert a NumPy array into a VTK-compatible array, with special handling for ``VTK_BIT`` arrays.

    When ``arrayType`` is set to ``vtk.VTK_BIT``, this function works around an existing issue in
    the built-in VTK helper ``vtk.util.numpy_support.numpy_to_vtk.numpy_to_vtk`` by manually packing
    the bits. See https://gitlab.kitware.com/vtk/vtk/-/issues/19610
    """
    import numpy as np
    import vtk.util.numpy_support

    if arrayType == vtk.VTK_BIT:
        # Workaround issue with built-in VTK function numpy_to_vtk.
        # See https://gitlab.kitware.com/vtk/vtk/-/issues/19610
        packedBits = np.packbits(narray)
        resultArray = vtk.util.numpy_support.create_vtk_array(arrayType)
        resultArray.SetNumberOfComponents(1)
        shape = narray.shape
        resultArray.SetNumberOfTuples(shape[0])
        resultArray.SetVoidArray(packedBits, shape[0], 1)
        # Since packedBits is a standalone object with no references from the caller.
        # As such, it will drop out of this scope and cause memory issues if we
        # do not deep copy its data.
        copy = resultArray.NewInstance()
        copy.DeepCopy(resultArray)
        resultArray = copy
        return resultArray

    return vtk.util.numpy_support.numpy_to_vtk(narray, deep=deep, array_type=arrayType)


def updateTableFromArray(tableNode, narrays, columnNames=None, setBoolAsUchar=False):
    """Set values in a table node from a NumPy array or an array-like object (list/tuple of NumPy arrays).

    This function can handle:

      - **Structured (record) array** with named fields. Each field becomes a separate column.
        If ``columnNames`` is not provided, the field names in the dtype are used.
      - **1D NumPy array**, which is added as a single column.
      - **2D NumPy array**, where each column in the array is added as a separate column in the
        table node (note that the array is transposed).
      - **List/tuple** of 1D NumPy arrays, which are each added as separate columns.

    :param tableNode: The table node to be updated. If ``None``, a new ``vtkMRMLTableNode`` is
      created and added to the scene.
    :type tableNode: vtkMRMLTableNode or None
    :param narrays: NumPy array or array-like object (structured array, 1D/2D array, or list/tuple of 1D arrays).
    :type narrays: np.ndarray, tuple, or list
    :param columnNames: Optional string or list of strings specifying names for the columns. If fewer
      names are provided than columns, only the specified columns are named;
      otherwise columns get default names. If ``None`` is passed, no column names are set.
    :type columnNames: str, list of str, or None
    :param setBoolAsUchar: If ``True``, boolean arrays are converted to ``uint8`` for plotting compatibility.
      This leverages the :func:`_vtkArrayFromNumpyArray` function's handling of ``VTK_BIT`` arrays.
      Default is ``False``.
    :type setBoolAsUchar: bool
    :return: Updated ``vtkMRMLTableNode``.
    :raises ValueError: If the input ``narrays`` is not a recognized format.

    This function automatically detects the data type of each input array using
    ``vtk.util.numpy_support.get_vtk_array_type`` and maps it to the corresponding VTK array type. As a
    result, a broader range of numeric and complex data (e.g., int, float, and complex) is supported
    without requiring manual conversions. All existing columns in the target table node are removed
    before adding new columns.

    .. warning:: Data in the table node is stored by value (deep copy). Modifying the NumPy array after calling
        this function does not update the table node's data.

    .. code-block:: python

      import numpy as np
      histogram = np.histogram(arrayFromVolume(getNode('MRHead')))
      tableNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLTableNode")
      updateTableFromArray(tableNode, histogram, ["Count", "Intensity"])
    """
    import numpy as np
    import vtk.util.numpy_support
    import slicer

    if tableNode is None:
        tableNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLTableNode")

    if isinstance(narrays, np.ndarray) and narrays.dtype.names:
        # Structured array with named fields
        fieldNames = narrays.dtype.names
        if columnNames is None:
            columnNames = list(fieldNames)
        ncolumns = [narrays[fieldName] for fieldName in fieldNames]
    elif isinstance(narrays, np.ndarray) and len(narrays.shape) == 1:
        # Single 1D array
        ncolumns = [narrays]
    elif isinstance(narrays, np.ndarray) and len(narrays.shape) == 2:
        # 2D array -> transpose so columns become separate arrays
        ncolumns = narrays.T
    elif isinstance(narrays, tuple) or isinstance(narrays, list):
        # List or tuple of 1D arrays
        ncolumns = narrays
    else:
        # Unrecognized format
        msg = (
            "Expected a structured NumPy array (with named fields), "
            "a 1D/2D NumPy array, or a list/tuple of 1D arrays; "
            "got {object_type} instead.".format(object_type=type(narrays))
        )
        raise ValueError(msg)

    tableNode.RemoveAllColumns()
    # Convert single string to a single-element string list
    if columnNames is None:
        columnNames = []
    if isinstance(columnNames, str):
        columnNames = [columnNames]

    # For each extracted column, convert to a VTK array and add to the table.
    for columnIndex, ncolumn in enumerate(ncolumns):
        vtype = None
        if ncolumn.dtype == np.bool_:
            if setBoolAsUchar:
                # Convert boolean arrays to uint to ensure compatibility with VTK plotting infrastructure:
                # vtkContext2D does not support vtkBitArray, and plotting calls that rely on
                # vtkArrayDownCast<vtkFloatArray>() would fail with a bit array.
                ncolumn = ncolumn.astype("uint8")
            else:
                vtype = vtk.VTK_BIT
        if vtype is None:
            vtype = vtk.util.numpy_support.get_vtk_array_type(ncolumn.dtype)
        vcolumn = _vtkArrayFromNumpyArray(ncolumn.ravel(), deep=True, arrayType=vtype)
        if (columnNames is not None) and (columnIndex < len(columnNames)):
            vcolumn.SetName(columnNames[columnIndex])
        tableNode.AddColumn(vcolumn)
    return tableNode


#
# MRML-pandas
#


def dataframeFromTable(tableNode):
    """Convert table node content to pandas dataframe.

    Table content is copied. Therefore, changes in table node do not affect the dataframe,
    and dataframe changes do not affect the original table node.
    """
    try:
        # Suppress "lzma compression not available" UserWarning when loading pandas
        import warnings

        with warnings.catch_warnings():
            warnings.simplefilter(action="ignore", category=UserWarning)
            import pandas as pd
    except ImportError:
        raise ImportError("Failed to convert to pandas dataframe. Please install pandas by running `slicer.util.pip_install('pandas')`")
    dataframe = pd.DataFrame()
    vtable = tableNode.GetTable()
    for columnIndex in range(vtable.GetNumberOfColumns()):
        vcolumn = vtable.GetColumn(columnIndex)
        column = []
        numberOfComponents = vcolumn.GetNumberOfComponents()
        if numberOfComponents == 1:
            # most common, simple case
            for rowIndex in range(vcolumn.GetNumberOfValues()):
                column.append(vcolumn.GetValue(rowIndex))
        else:
            # rare case: column contains multiple components
            valueIndex = 0
            for rowIndex in range(vcolumn.GetNumberOfTuples()):
                item = []
                for componentIndex in range(numberOfComponents):
                    item.append(vcolumn.GetValue(valueIndex))
                    valueIndex += 1
                column.append(item)
        dataframe[vcolumn.GetName()] = column
    return dataframe


def dataframeFromMarkups(markupsNode):
    """Convert markups node content to pandas dataframe.

    Markups content is copied. Therefore, changes in markups node do not affect the dataframe,
    and dataframe changes do not affect the original markups node.
    """
    try:
        # Suppress "lzma compression not available" UserWarning when loading pandas
        import warnings

        with warnings.catch_warnings():
            warnings.simplefilter(action="ignore", category=UserWarning)
            import pandas as pd
    except ImportError:
        raise ImportError("Failed to convert to pandas dataframe. Please install pandas by running `slicer.util.pip_install('pandas')`")

    label = []
    description = []
    positionWorldR = []
    positionWorldA = []
    positionWorldS = []
    selected = []
    visible = []

    numberOfControlPoints = markupsNode.GetNumberOfControlPoints()
    for controlPointIndex in range(numberOfControlPoints):
        label.append(markupsNode.GetNthControlPointLabel(controlPointIndex))
        description.append(markupsNode.GetNthControlPointDescription(controlPointIndex))
        p = [0, 0, 0]
        markupsNode.GetNthControlPointPositionWorld(controlPointIndex, p)
        positionWorldR.append(p[0])
        positionWorldA.append(p[1])
        positionWorldS.append(p[2])
        selected.append(markupsNode.GetNthControlPointSelected(controlPointIndex) != 0)
        visible.append(markupsNode.GetNthControlPointVisibility(controlPointIndex) != 0)

    dataframe = pd.DataFrame({
        "label": label,
        "position.R": positionWorldR,
        "position.A": positionWorldA,
        "position.S": positionWorldS,
        "selected": selected,
        "visible": visible,
        "description": description})
    return dataframe


#
# MRML-ITKImage
#


def itkImageFromVolume(volumeNode):
    """Return ITK image from volume node.

    Voxels values are not copied. Voxel values in the volume node can be modified
    by changing values in the ITK image.
    After all modifications has been completed, call :py:meth:`itkImageFromVolumeModified`.

    .. warning:: Important: Memory area of the returned ITK image is managed by VTK (through the
      ``vtkImageData`` object stored in the MRML volume node), therefore values in the
      Voxel values in the ITK image may be changed, but the ITK image must not be reallocated.
    """
    import itk
    import vtk

    def _updateMatrix3x3From4x4(matrix3x3, matrix4x4):
        for i_idx in range(3):
            for j_idx in range(3):
                matrix3x3.SetElement(i_idx, j_idx, matrix4x4.GetElement(i_idx, j_idx))

    # Get direction from volume node
    directionMatrix4x4 = vtk.vtkMatrix4x4()
    volumeNode.GetIJKToRASDirectionMatrix(directionMatrix4x4)
    directionMatrix3x3 = vtk.vtkMatrix3x3()
    _updateMatrix3x3From4x4(directionMatrix3x3, directionMatrix4x4)

    # Transform from RAS (used in Slicer) to LPS (used in ITK)
    rasToLps = vtk.vtkMatrix3x3()
    rasToLps.SetElement(0, 0, -1)
    rasToLps.SetElement(1, 1, -1)
    vtk.vtkMatrix3x3.Multiply3x3(rasToLps, directionMatrix3x3, directionMatrix3x3)

    # Create VTK image data
    vtkImage = vtk.vtkImageData()
    vtkImage.ShallowCopy(volumeNode.GetImageData())

    # Update VTK image direction/origin/spacing
    vtkImage.SetDirectionMatrix(directionMatrix3x3)
    vtkImage.SetOrigin(volumeNode.GetOrigin())
    vtkImage.SetSpacing(volumeNode.GetSpacing())

    return itk.image_from_vtk_image(vtkImage)


def itkImageFromVolumeModified(volumeNode):
    """Indicate that modification of a ITK image returned by :py:meth:`itkImageFromVolume` (or
    associated with a volume node using :py:meth:`updateVolumeFromITKImage`) has been completed.
    """
    imageData = volumeNode.GetImageData()
    pointData = imageData.GetPointData() if imageData else None
    if pointData:
        if pointData.GetScalars():
            pointData.GetScalars().Modified()
        if pointData.GetTensors():
            pointData.GetTensors().Modified()
        if pointData.GetVectors():
            pointData.GetVectors().Modified()
    volumeNode.Modified()


def addVolumeFromITKImage(itkImage, name=None, nodeClassName=None, deepCopy=True):
    """Create a new volume node from content of an ITK image and add it to the scene.

    By default, voxels values are deep-copied, therefore if the ITK image is modified
    after calling this method, voxel values in the volume node will not change.

    See :py:meth:`updateVolumeFromITKImage` to understand memory ownership.

    :param itkImage: ITK image containing volume voxels.
    :param name: volume node name
    :param nodeClassName: type of created volume, default: ``vtkMRMLScalarVolumeNode``.
      Use ``vtkMRMLLabelMapVolumeNode`` for labelmap volume, ``vtkMRMLVectorVolumeNode`` for vector volume.
    :param deepCopy: Whether voxels values are deep-copied or not.

    :return: created new volume node
    """
    import slicer

    if name is None:
        name = ""
    if nodeClassName is None:
        nodeClassName = "vtkMRMLScalarVolumeNode"

    volumeNode = slicer.mrmlScene.AddNewNodeByClass(nodeClassName, name)
    updateVolumeFromITKImage(volumeNode, itkImage, deepCopy)
    volumeNode.CreateDefaultDisplayNodes()

    return volumeNode


def updateVolumeFromITKImage(volumeNode, itkImage, deepCopy=True):
    """Set voxels of a volume node from an ITK image.

    By default, voxels values are deep-copied, therefore if the ITK image is modified
    after calling this method, voxel values in the volume node will not change.

    .. warning:: Important: Setting `deepCopy` to False means that the memory area is
      shared between the ITK image and the ``vtkImageData`` object in the MRML volume node,
      therefore modifying the ITK image values requires to call :py:meth:`itkImageFromVolumeModified`.

      If the ITK image is reallocated, calling this function is required.
    """
    import itk
    import vtk
    import vtkAddon

    # Convert ITK image to VTK image
    vtkImage = itk.vtk_image_from_image(itkImage)

    # Get direction from VTK image
    ijkToRASMatrix = vtk.vtkMatrix4x4()
    vtkAddon.vtkAddonMathUtilities.SetOrientationMatrix(vtkImage.GetDirectionMatrix(), ijkToRASMatrix)

    # Transform from LPS (used in ITK) to RAS (used in Slicer)
    lpsToRas = vtk.vtkMatrix4x4()
    lpsToRas.SetElement(0, 0, -1)
    lpsToRas.SetElement(1, 1, -1)
    vtk.vtkMatrix4x4.Multiply4x4(lpsToRas, ijkToRASMatrix, ijkToRASMatrix)

    # Update output node direction/origin/spacing
    volumeNode.SetIJKToRASMatrix(ijkToRASMatrix)
    volumeNode.SetOrigin(*vtkImage.GetOrigin())
    volumeNode.SetSpacing(*vtkImage.GetSpacing())

    # Reset direction/origin/spacing of the VTK output image.
    # See https://github.com/Slicer/Slicer/issues/6911
    identidyMatrix = vtk.vtkMatrix3x3()
    vtkImage.SetDirectionMatrix(identidyMatrix)
    vtkImage.SetOrigin((0, 0, 0))
    vtkImage.SetSpacing((1.0, 1.0, 1.0))

    # Update output node setting VTK image data
    if deepCopy:
        vtkImageCopy = vtk.vtkImageData()
        vtkImageCopy.DeepCopy(vtkImage)
        volumeNode.SetAndObserveImageData(vtkImageCopy)
    else:
        volumeNode.SetAndObserveImageData(vtkImage)


#
# VTK
#


class VTKObservationMixin:
    def __init__(self):
        super().__init__()

        self.__observations = {}
        # {obj: {event: {method: (group, tag, priority)}}}

    @property
    def Observations(self):
        return [
            (obj, event, method, group, tag, priority)
            for obj, events in self.__observations.items()
            for event, methods in events.items()
            for method, (group, tag, priority) in methods.items()
        ]

    def removeObservers(self, method=None):
        if method is None:
            for obj, _, _, _, tag, _ in self.Observations:
                obj.RemoveObserver(tag)
            self.__observations.clear()
        else:
            for obj, events in self.__observations.items():
                for e, methods in events.items():
                    if method in methods:
                        g, t, p = methods.pop(method)
                        obj.RemoveObserver(t)

    def addObserver(self, obj, event, method, group="none", priority=0.0):
        from warnings import warn

        events = self.__observations.setdefault(obj, {})
        methods = events.setdefault(event, {})

        if method in methods:
            warn("already has observer")
            return

        tag = obj.AddObserver(event, method, priority)
        methods[method] = group, tag, priority

    def removeObserver(self, obj, event, method):
        from warnings import warn

        try:
            events = self.__observations[obj]
            methods = events[event]
            group, tag, priority = methods.pop(method)
            obj.RemoveObserver(tag)
        except KeyError:
            warn("does not have observer")

    def getObserver(self, obj, event, method, default=None):
        try:
            events = self.__observations[obj]
            methods = events[event]
            group, tag, priority = methods[method]
            return group, tag, priority
        except KeyError:
            return default

    def hasObserver(self, obj, event, method):
        return self.getObserver(obj, event, method) is not None

    def observer(self, event, method, default=None):
        for obj, events in self.__observations.items():
            if self.hasObserver(obj, event, method):
                return obj

        return default


def toVTKString(text):
    """Convert unicode string into VTK string.

    .. deprecated:: 4.11.0
      Since now VTK assumes that all strings are in UTF-8 and all strings in Slicer are UTF-8, too,
      conversion is no longer necessary.
      The method is only kept for backward compatibility and will be removed in the future.
    """
    import logging

    logging.warning("toVTKString is deprecated! Conversion is no longer necessary.")
    import traceback

    logging.debug("toVTKString was called from " + ("".join(traceback.format_stack())))
    return text


def toLatin1String(text):
    """Convert string to latin1 encoding."""
    vtkStr = ""
    for c in text:
        try:
            cc = c.encode("latin1", "ignore").decode()
        except UnicodeDecodeError:
            cc = "?"
        vtkStr = vtkStr + cc
    return vtkStr


#
# File Utilities
#


def tempDirectory(key="__SlicerTemp__", tempDir=None, includeDateTime=True):
    """Come up with a unique directory name in the temp dir and make it and return it

    .. note:: This directory is not automatically cleaned up.
    """
    # TODO: switch to QTemporaryDir in Qt5.
    import qt, slicer

    if not tempDir:
        tempDir = qt.QDir(slicer.app.temporaryPath)
    if includeDateTime:
        # Force using en-US locale, otherwise for example on a computer with
        # Egyptian Arabic (ar-EG) locale, Arabic numerals may be used.
        enUsLocale = qt.QLocale(qt.QLocale.English, qt.QLocale.UnitedStates)
        tempDirName = key + enUsLocale.toString(qt.QDateTime.currentDateTime(), "yyyy-MM-dd_hh+mm+ss.zzz")
    else:
        tempDirName = key
    fileInfo = qt.QFileInfo(qt.QDir(tempDir), tempDirName)
    dirPath = fileInfo.absoluteFilePath()
    qt.QDir().mkpath(dirPath)
    return dirPath


def delayDisplay(message, autoCloseMsec=1000, parent=None, **kwargs):
    """Display an information message in a popup window for a short time.

    If ``autoCloseMsec < 0`` then the window is not closed until the user clicks on it

    If ``0 <= autoCloseMsec < 400`` then only ``slicer.app.processEvents()`` is called.

    If ``autoCloseMsec >= 400`` then the window is closed after waiting for autoCloseMsec milliseconds
    """
    import qt, slicer
    import logging

    logging.info(message)
    if 0 <= autoCloseMsec < 400:
        slicer.app.processEvents()
        return
    messagePopup = qt.QDialog(parent if parent else mainWindow())
    for key, value in kwargs.items():
        if hasattr(messagePopup, key):
            setattr(messagePopup, key, value)
    layout = qt.QVBoxLayout()
    messagePopup.setLayout(layout)
    label = qt.QLabel(message, messagePopup)
    layout.addWidget(label)
    if autoCloseMsec >= 0:
        qt.QTimer.singleShot(autoCloseMsec, messagePopup.close)
    else:
        okButton = qt.QPushButton("OK")
        layout.addWidget(okButton)
        okButton.connect("clicked()", messagePopup.close)
    # Windows 10 peek feature in taskbar shows all hidden but not destroyed windows
    # (after creating and closing a messagebox, hovering over the mouse on Slicer icon, moving up the
    # mouse to the peek thumbnail would show it again).
    # Popup windows in other Qt applications often show closed popups (such as
    # Paraview's Edit / Find data dialog, MeshMixer's File/Preferences dialog).
    # By calling deleteLater, the messagebox is permanently deleted when the current call is completed.
    messagePopup.deleteLater()
    messagePopup.exec_()


def infoDisplay(text, windowTitle=None, parent=None, standardButtons=None, **kwargs):
    """Display popup with a info message.

    If there is no main window, or if the application is running in testing mode (``slicer.app.testingEnabled() == True``),
    then the text is only logged (at info level).
    """
    import qt, logging

    standardButtons = standardButtons if standardButtons else qt.QMessageBox.Ok
    _messageDisplay(logging.INFO, text, None, parent=parent, windowTitle=windowTitle, mainWindowNeeded=True,
                    icon=qt.QMessageBox.Information, standardButtons=standardButtons, **kwargs)


def warningDisplay(text, windowTitle=None, parent=None, standardButtons=None, **kwargs):
    """Display popup with a warning message.

    If there is no main window, or if the application is running in testing mode (``slicer.app.testingEnabled() == True``),
    then the text is only logged (at warning level).
    """
    import qt, logging

    standardButtons = standardButtons if standardButtons else qt.QMessageBox.Ok
    _messageDisplay(logging.WARNING, text, None, parent=parent, windowTitle=windowTitle, mainWindowNeeded=True,
                    icon=qt.QMessageBox.Warning, standardButtons=standardButtons, **kwargs)


def errorDisplay(text, windowTitle=None, parent=None, standardButtons=None, **kwargs):
    """Display an error popup.

    If there is no main window, or if the application is running in testing mode (``slicer.app.testingEnabled() == True``),
    then the text is only logged (at error level).
    """
    import qt, logging

    standardButtons = standardButtons if standardButtons else qt.QMessageBox.Ok
    _messageDisplay(logging.ERROR, text, None, parent=parent, windowTitle=windowTitle, mainWindowNeeded=True,
                    icon=qt.QMessageBox.Critical, standardButtons=standardButtons, **kwargs)


def confirmOkCancelDisplay(text, windowTitle=None, parent=None, **kwargs):
    """Display a confirmation popup. Return if confirmed with OK.

    When the application is running in testing mode (``slicer.app.testingEnabled() == True``),
    the popup is skipped and True ("Ok") is returned, with a message being logged to indicate this.
    """
    import qt, slicer, logging

    if not windowTitle:
        windowTitle = slicer.app.applicationName + " confirmation"
    result = _messageDisplay(logging.INFO, text, True, parent=parent, windowTitle=windowTitle, icon=qt.QMessageBox.Question,
                             standardButtons=qt.QMessageBox.Ok | qt.QMessageBox.Cancel, **kwargs)
    return result == qt.QMessageBox.Ok


def confirmYesNoDisplay(text, windowTitle=None, parent=None, **kwargs):
    """Display a confirmation popup. Return if confirmed with Yes.

    When the application is running in testing mode (``slicer.app.testingEnabled() == True``),
    the popup is skipped and True ("Yes") is returned, with a message being logged to indicate this.
    """
    import qt, slicer, logging

    if not windowTitle:
        windowTitle = slicer.app.applicationName + " confirmation"
    result = _messageDisplay(logging.INFO, text, True, parent=parent, windowTitle=windowTitle, icon=qt.QMessageBox.Question,
                             standardButtons=qt.QMessageBox.Yes | qt.QMessageBox.No, **kwargs)
    return result == qt.QMessageBox.Yes


def confirmRetryCloseDisplay(text, windowTitle=None, parent=None, **kwargs):
    """Display an error popup asking whether to retry, logging the text at error level.
    Return if confirmed with Retry.

    When the application is running in testing mode (``slicer.app.testingEnabled() == True``),
    the popup is skipped and False ("Close") is returned, with a message being logged to indicate this.
    """
    import qt, logging
    result = _messageDisplay(logging.ERROR, text, False, parent=parent, windowTitle=windowTitle,
                             icon=qt.QMessageBox.Critical, standardButtons=qt.QMessageBox.Retry | qt.QMessageBox.Close, **kwargs)
    return result == qt.QMessageBox.Retry


def _messageDisplay(logLevel, text, testingReturnValue, mainWindowNeeded=False, parent=None, windowTitle=None, **kwargs):
    """Displays a messagebox and logs message text; knows what to do in testing mode.

    :param logLevel: The level at which to log text, e.g. ``logging.INFO``, ``logging.ERROR``
    :param text: Message text
    :type text: str
    :param testingReturnValue: When the application is in testing mode, this value is returned instead of raising the message box.
    :param mainWindowNeeded: If True then the message box will not be raised if there is no mainWindow, but the text is still logged.
    :type mainWindowNeeded: bool, optional
    :param parent: The message box parent; by default it is set to main window by slicer.util.messageBox
    :type parent: QWidget, optional
    :param windowTitle: Window title; defaults to a generic title based on log level.
    :type windowTitle: str, optional
    :param kwargs: passed to :func:`messageBox`

    Returns:
      The output of :func:`messageBox`, with two exceptions:
      - If the application is running in testing mode, then ``testingReturnValue`` is returned.
      - Otherwise, if ``mainWindowNeeded`` is True and there is no main window, then None is returned.
    """
    import slicer, logging

    logging.log(logLevel, text)
    logLevelString = logging.getLevelName(logLevel).lower()  # e.g. this is "error" when logLevel is logging.ERROR
    if not windowTitle:
        windowTitle = slicer.app.applicationName + " " + logLevelString
    if slicer.app.testingEnabled():
        logging.info(f"Testing mode is enabled: Returning {testingReturnValue} and skipping message box [{windowTitle}].")
        return testingReturnValue
    if mainWindowNeeded and mainWindow() is None:
        return
    return messageBox(text, parent=parent, windowTitle=windowTitle, **kwargs)


def messageBox(text, parent=None, **kwargs):
    """Displays a messagebox.

    ctkMessageBox is used instead of a default qMessageBox to provide "Don't show again" checkbox.

    For example::

      slicer.util.messageBox("Some message", dontShowAgainSettingsKey = "MainWindow/DontShowSomeMessage")

    When the application is running in testing mode (``slicer.app.testingEnabled() == True``),
    an auto-closing popup with a delay of 3s is shown using :func:`delayDisplay()` and ``qt.QMessageBox.Ok``
    is returned, with the text being logged to indicate this.
    """
    import logging, qt, slicer

    if slicer.app.testingEnabled():
        testingReturnValue = qt.QMessageBox.Ok
        logging.info(f"Testing mode is enabled: Returning {testingReturnValue} (qt.QMessageBox.Ok) and displaying an auto-closing message box [{text}].")
        slicer.util.delayDisplay(text, autoCloseMsec=3000, parent=parent, **kwargs)
        return testingReturnValue

    # if there is detailed text, make the dialog wider by making a long title
    if "detailedText" in kwargs:
        windowTitle = kwargs["windowTitle"] if "windowTitle" in kwargs else slicer.app.applicationName
        padding = " " * ((150 - len(windowTitle)) // 2)  # to center the title
        kwargs["windowTitle"] = padding + windowTitle + padding

    import ctk

    mbox = ctk.ctkMessageBox(parent if parent else mainWindow())
    mbox.text = text
    for key, value in kwargs.items():
        if hasattr(mbox, key):
            setattr(mbox, key, value)
    # Windows 10 peek feature in taskbar shows all hidden but not destroyed windows
    # (after creating and closing a messagebox, hovering over the mouse on Slicer icon, moving up the
    # mouse to the peek thumbnail would show it again).
    # Popup windows in other Qt applications often show closed popups (such as
    # Paraview's Edit / Find data dialog, MeshMixer's File/Preferences dialog).
    # By calling deleteLater, the messagebox is permanently deleted when the current call is completed.
    mbox.deleteLater()
    return mbox.exec_()


def createProgressDialog(parent=None, value=0, maximum=100, labelText="", windowTitle="Processing...", **kwargs):
    """Display a modal QProgressDialog.

    Go to `QProgressDialog documentation <https://doc.qt.io/qt-5/qprogressdialog.html>`_ to
    learn about the available keyword arguments.

    Examples::

      # Prevent progress dialog from automatically closing
      progressbar = createProgressDialog(autoClose=False)

      # Update progress value
      progressbar.value = 50

      # Update label text
      progressbar.labelText = "processing XYZ"
    """
    import qt

    progressIndicator = qt.QProgressDialog(parent if parent else mainWindow())
    progressIndicator.minimumDuration = 0
    progressIndicator.maximum = maximum
    progressIndicator.value = value
    progressIndicator.windowTitle = windowTitle
    progressIndicator.labelText = labelText
    for key, argument in kwargs.items():
        if hasattr(progressIndicator, key):
            setattr(progressIndicator, key, argument)
    return progressIndicator


from contextlib import contextmanager


@contextmanager
def displayPythonShell(display=True):
    """Show the Python console while the code in the context manager is being run.

    The console stays visible only if it was visible already.

    :param display: If show is False, the context manager has no effect.

    .. code-block:: python

      with slicer.util.displayPythonShell():
        slicer.util.pip_install('nibabel')

    """
    import slicer

    def dockableWindowEnabled():
        import qt

        return toBool(qt.QSettings().value("Python/DockableWindow"))

    def getConsole():
        return mainWindow().pythonConsole().parent() if dockableWindowEnabled() else pythonShell()

    if display:
        console = getConsole()
        consoleVisible = console.visible
        console.show()
        slicer.app.processEvents()
    try:
        yield
    finally:
        if display:
            console.setVisible(consoleVisible)


class WaitCursor:
    """Display a wait cursor while the code in the context manager is being run.

    :param show: If show is False, no wait cursor is shown.

    .. code-block:: python

      import time

      n = 2
      with slicer.util.MessageDialog(f'Sleeping for {n} seconds...'):
        with slicer.util.WaitCursor():
          time.sleep(n)

    """

    def __init__(self, show=True):
        """Set the cursor to waiting mode while the code in the context manager is being run.

        :param show: If show is False, this context manager has no effect.

        .. code-block:: python

          import time

          with slicer.util.WaitCursor():
            time.sleep(2)
        """
        self.show = show

    def __enter__(self):
        import qt, slicer

        if self.show:
            qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)
            slicer.app.processEvents()

    def __exit__(self, type, value, traceback):
        if self.show:
            import qt

            qt.QApplication.restoreOverrideCursor()


class MessageDialog:
    def __init__(self, message, show=True, logLevel=None):
        """Log the message and show a message box while the code in the context manager is being run.
        When the application is running in testing mode (``slicer.app.testingEnabled() == True``), the message box is skipped.

        :param message: Text shown in the message box.
        :param show: If show is False, no dialog is shown.
        :param logLevel: Log level used to log the message. Default: logging.INFO

        .. code-block:: python

          import time

          n = 2
          with slicer.util.MessageDialog(f'Sleeping for {n} seconds...'):
            with slicer.util.WaitCursor():
              time.sleep(n)

        """
        import logging
        import slicer

        if logLevel is None:
            logLevel = logging.INFO
        if not isinstance(logLevel, int):
            raise ValueError(f"Invalid log level: {logLevel}")

        self.message = message
        self.show = show and not slicer.app.testingEnabled()
        self.logLevel = logLevel
        self.box = None

    def __enter__(self):
        import logging

        logging.log(self.logLevel, self.message)

        if self.show:
            import qt, slicer

            self.box = qt.QMessageBox()
            self.box.setStandardButtons(qt.QMessageBox.NoButton)
            self.box.setText(self.message)
            self.box.show()
            slicer.app.processEvents()

    def __exit__(self, type, value, traceback):
        if self.show:
            self.box.accept()


@contextmanager
def tryWithErrorDisplay(message=None, show=True, waitCursor=False):
    """Show an error display with the error details if an exception is raised.

    :param message: Text shown in the message box.
    :param show: If show is False, the context manager has no effect.
    :param waitCursor: If waitCrusor is set to True then mouse cursor is changed to
       wait cursor while the context manager is being run.

    .. code-block:: python

      import random

      def risky():
        if random.choice((True, False)):
          raise Exception('Error while trying to do some internal operations.')

      with slicer.util.tryWithErrorDisplay("Risky operation failed."):
        risky()
    """
    try:
        if waitCursor:
            import slicer, qt

            slicer.app.setOverrideCursor(qt.Qt.WaitCursor)
        yield
        if waitCursor:
            slicer.app.restoreOverrideCursor()
    except Exception as e:
        import slicer

        if waitCursor:
            slicer.app.restoreOverrideCursor()
        if show and not slicer.app.testingEnabled():
            if message is not None:
                errorMessage = f"{message}\n\n{e}"
            else:
                errorMessage = str(e)

            import traceback

            errorDisplay(errorMessage, detailedText=traceback.format_exc())
        raise


def toBool(value):
    """Convert any type of value to a boolean.

    The function uses the following heuristic:

    1. If the value can be converted to an integer, the integer is then
       converted to a boolean.
    2. If the value is a string, return True if it is equal to 'true'. False otherwise.
       Note that the comparison is case insensitive.
    3. If the value is neither an integer or a string, the bool() function is applied.

    >>> [toBool(x) for x in range(-2, 2)]
    [True, True, False, True]
    >>> [toBool(x) for x in ['-2', '-1', '0', '1', '2', 'Hello']]
    [True, True, False, True, True, False]
    >>> [toBool(x) for x in ['true', 'false', 'True', 'False', 'tRue', 'fAlse']]
    [True, False, True, False, True, False]
    >>> toBool(object())
    True
    >>> toBool(None)
    False
    """
    try:
        return bool(int(value))
    except (ValueError, TypeError):
        return value.lower() in ["true"] if isinstance(value, str) else bool(value)


def settingsValue(key, default, converter=lambda v: v, settings=None):
    """Return settings value associated with key if it exists or the provided default otherwise.

    ``settings`` parameter is expected to be a valid ``qt.Settings`` object.
    """
    import qt

    settings = qt.QSettings() if settings is None else settings
    return converter(settings.value(key)) if settings.contains(key) else default


def clickAndDrag(widget, button="Left", start=(10, 10), end=(10, 40), steps=20, modifiers=[]):
    """Send synthetic mouse events to the specified widget (qMRMLSliceWidget or qMRMLThreeDView)

    :param button: "Left", "Middle", "Right", or "None"
     start, end : window coordinates for action
    :param steps: number of steps to move in, if <2 then mouse jumps to the end position
    :param modifiers: list containing zero or more of "Shift" or "Control"
    :raises RuntimeError: in case of failure

    .. hint::

      For generating test data you can use this snippet of code::

        layoutManager = slicer.app.layoutManager()
        threeDView = layoutManager.threeDWidget(0).threeDView()
        style = threeDView.interactorStyle()
        interactor = style.GetInteractor()

        def onClick(caller,event):
            print(interactor.GetEventPosition())

        interactor.AddObserver(vtk.vtkCommand.LeftButtonPressEvent, onClick)
    """
    style = widget.interactorStyle()
    interactor = style.GetInteractor()
    if button == "Left":
        down = interactor.LeftButtonPressEvent
        up = interactor.LeftButtonReleaseEvent
    elif button == "Right":
        down = interactor.RightButtonPressEvent
        up = interactor.RightButtonReleaseEvent
    elif button == "Middle":
        down = interactor.MiddleButtonPressEvent
        up = interactor.MiddleButtonReleaseEvent
    elif button == "None" or not button:
        down = lambda: None
        up = lambda: None
    else:
        raise RuntimeError("Bad button - should be Left or Right, not %s" % button)
    if "Shift" in modifiers:
        interactor.SetShiftKey(1)
    if "Control" in modifiers:
        interactor.SetControlKey(1)
    interactor.SetEventPosition(*start)
    down()
    if steps < 2:
        interactor.SetEventPosition(end[0], end[1])
        interactor.MouseMoveEvent()
    else:
        for step in range(steps):
            frac = float(step) / (steps - 1)
            x = int(start[0] + frac * (end[0] - start[0]))
            y = int(start[1] + frac * (end[1] - start[1]))
            interactor.SetEventPosition(x, y)
            interactor.MouseMoveEvent()
    up()
    interactor.SetShiftKey(0)
    interactor.SetControlKey(0)


def downloadFile(url, targetFilePath, checksum=None, reDownloadIfChecksumInvalid=True):
    """Download ``url`` to local storage as ``targetFilePath``

    Target file path needs to indicate the file name and extension as well

    If specified, the ``checksum`` is used to verify that the downloaded file is the expected one.
    It must be specified as ``<algo>:<digest>``. For example, ``SHA256:cc211f0dfd9a05ca3841ce1141b292898b2dd2d3f08286affadf823a7e58df93``.
    """
    import os
    import logging

    try:
        (algo, digest) = extractAlgoAndDigest(checksum)
    except ValueError as excinfo:
        logging.error(f"Failed to parse checksum: {excinfo}")
        return False
    if not os.path.exists(targetFilePath) or os.stat(targetFilePath).st_size == 0:
        logging.info(f"Downloading from\n  {url}\nas file\n  {targetFilePath}\nIt may take a few minutes...")
        try:
            import urllib.request, urllib.parse, urllib.error

            urllib.request.urlretrieve(url, targetFilePath)
        except Exception as e:
            import traceback

            traceback.print_exc()
            logging.error("Failed to download file from " + url)
            return False
        if algo is not None:
            logging.info("Verifying checksum\n  %s" % targetFilePath)
            current_digest = computeChecksum(algo, targetFilePath)
            if current_digest != digest:
                logging.error("Downloaded file does not have expected checksum."
                              "\n   current checksum: %s"
                              "\n  expected checksum: %s" % (current_digest, digest))
                return False
            else:
                logging.info("Checksum OK")
    else:
        if algo is not None:
            current_digest = computeChecksum(algo, targetFilePath)
            if current_digest != digest:
                if reDownloadIfChecksumInvalid:
                    logging.info("Requested file has been found but its checksum is different: deleting and re-downloading")
                    os.remove(targetFilePath)
                    return downloadFile(url, targetFilePath, checksum, reDownloadIfChecksumInvalid=False)
                else:
                    logging.error("Requested file has been found but its checksum is different:"
                                  "\n   current checksum: %s"
                                  "\n  expected checksum: %s" % (current_digest, digest))
                    return False
            else:
                logging.info("Requested file has been found and checksum is OK: " + str(targetFilePath))
        else:
            logging.info(f"Requested file has been found: {targetFilePath}")
    return True


def extractArchive(archiveFilePath, outputDir, expectedNumberOfExtractedFiles=None):
    """Extract file ``archiveFilePath`` into folder ``outputDir``.

    Number of expected files unzipped may be specified in ``expectedNumberOfExtractedFiles``.
    If folder contains the same number of files as expected (if specified), then it will be
    assumed that unzipping has been successfully done earlier.
    """
    import os
    import logging
    from slicer import app

    if not os.path.exists(archiveFilePath):
        logging.error("Specified file %s does not exist" % (archiveFilePath))
        return False
    fileName, fileExtension = os.path.splitext(archiveFilePath)
    if fileExtension.lower() != ".zip":
        # TODO: Support other archive types
        logging.error("Only zip archives are supported now, got " + fileExtension)
        return False

    numOfFilesInOutputDir = len(getFilesInDirectory(outputDir, False))
    if expectedNumberOfExtractedFiles is not None \
            and numOfFilesInOutputDir == expectedNumberOfExtractedFiles:
        logging.info(f"File {archiveFilePath} already unzipped into {outputDir}")
        return True

    extractSuccessful = app.applicationLogic().Unzip(archiveFilePath, outputDir)
    numOfFilesInOutputDirTest = len(getFilesInDirectory(outputDir, False))
    if extractSuccessful is False or (expectedNumberOfExtractedFiles is not None \
                                      and numOfFilesInOutputDirTest != expectedNumberOfExtractedFiles):
        logging.error(f"Unzipping {archiveFilePath} into {outputDir} failed")
        return False
    logging.info(f"Unzipping {archiveFilePath} into {outputDir} successful")
    return True


def computeChecksum(algo, filePath):
    """Compute digest of ``filePath`` using ``algo``.

    Supported hashing algorithms are SHA256, SHA512, and MD5.

    It internally reads the file by chunk of 8192 bytes.

    :raises ValueError: if algo is unknown.
    :raises IOError: if filePath does not exist.
    """
    import hashlib

    if algo not in ["SHA256", "SHA512", "MD5"]:
        raise ValueError("unsupported hashing algorithm %s" % algo)

    with open(filePath, "rb") as content:
        hash = hashlib.new(algo)
        while True:
            chunk = content.read(8192)
            if not chunk:
                break
            hash.update(chunk)
        return hash.hexdigest()


def extractAlgoAndDigest(checksum):
    """Given a checksum string formatted as ``<algo>:<digest>`` returns the tuple ``(algo, digest)``.

    ``<algo>`` is expected to be `SHA256`, `SHA512`, or `MD5`.
    ``<digest>`` is expected to be the full length hexadecimal digest.

    :raises ValueError: if checksum is incorrectly formatted.
    """
    if checksum is None:
        return None, None
    if len(checksum.split(":")) != 2:
        raise ValueError("invalid checksum '%s'. Expected format is '<algo>:<digest>'." % checksum)
    (algo, digest) = checksum.split(":")
    expected_algos = ["SHA256", "SHA512", "MD5"]
    if algo not in expected_algos:
        raise ValueError("invalid algo '{}'. Algo must be one of {}".format(algo, ", ".join(expected_algos)))
    expected_digest_length = {"SHA256": 64, "SHA512": 128, "MD5": 32}
    if len(digest) != expected_digest_length[algo]:
        raise ValueError("invalid digest length %d. Expected digest length for %s is %d" % (len(digest), algo, expected_digest_length[algo]))
    return algo, digest


def downloadAndExtractArchive(url, archiveFilePath, outputDir, \
                              expectedNumberOfExtractedFiles=None, numberOfTrials=3, checksum=None):
    """Downloads an archive from ``url`` as ``archiveFilePath``, and extracts it to ``outputDir``.

    This combined function tests the success of the download by the extraction step,
    and re-downloads if extraction failed.

    If specified, the ``checksum`` is used to verify that the downloaded file is the expected one.
    It must be specified as ``<algo>:<digest>``. For example, ``SHA256:cc211f0dfd9a05ca3841ce1141b292898b2dd2d3f08286affadf823a7e58df93``.
    """
    import os
    import shutil
    import logging

    maxNumberOfTrials = numberOfTrials

    def _cleanup():
        # If there was a failure, delete downloaded file and empty output folder
        logging.warning("Download and extract failed, removing archive and destination folder and retrying. Attempt #%d..." % (maxNumberOfTrials - numberOfTrials))
        os.remove(archiveFilePath)
        shutil.rmtree(outputDir)
        os.mkdir(outputDir)

    while numberOfTrials:
        if not downloadFile(url, archiveFilePath, checksum):
            numberOfTrials -= 1
            _cleanup()
            continue
        if not extractArchive(archiveFilePath, outputDir, expectedNumberOfExtractedFiles):
            numberOfTrials -= 1
            _cleanup()
            continue
        return True

    _cleanup()
    return False


def getFilesInDirectory(directory, absolutePath=True):
    """Collect all files in a directory and its subdirectories in a list."""
    import os

    allFiles = []
    for root, subdirs, files in os.walk(directory):
        for fileName in files:
            if absolutePath:
                fileAbsolutePath = os.path.abspath(os.path.join(root, fileName)).replace("\\", "/")
                allFiles.append(fileAbsolutePath)
            else:
                allFiles.append(fileName)
    return allFiles


class chdir:
    """Non thread-safe context manager to change the current working directory.

    .. note::

      Available in Python 3.11 as ``contextlib.chdir`` and adapted from https://github.com/python/cpython/pull/28271

      Available in CTK as ``ctkScopedCurrentDir`` C++ class
    """

    def __init__(self, path):
        self.path = path
        self._old_cwd = []

    def __enter__(self):
        import os

        self._old_cwd.append(os.getcwd())
        os.chdir(self.path)

    def __exit__(self, *excinfo):
        import os

        os.chdir(self._old_cwd.pop())


def plot(narray, xColumnIndex=-1, columnNames=None, title=None, show=True, nodes=None, plotBoolAsUchar=False):
    """Create a plot from a numpy array that contains two or more columns.

    :param narray: input numpy array containing data series in columns.
    :param xColumnIndex: index of column that will be used as x axis.
      If it is set to negative number (by default) then row index will be used as x coordinate.
    :param columnNames: names of each column of the input array. If title is specified for the plot
      then title+columnName will be used as series name.
    :param title: title of the chart. Plot node names are set based on this value.
    :param nodes: plot chart, table, and list of plot series nodes.
      Specified in a dictionary, with keys: 'chart', 'table', 'series'.
      Series contains a list of plot series nodes (one for each table column).
      The parameter is used both as an input and output.
    :param plotBoolAsUchar: If ``True``, any boolean columns in the input array are automatically
      converted to unsigned integer arrays. This avoids issues with plotting bit arrays (``vtkBitArray``),
      which are not fully supported by VTK plotting. Defaults to ``False``.
    :return: plot chart node. Plot chart node provides access to chart properties and plot series nodes.

    Example 1: simple plot

    .. code-block:: python

      # Get sample data
      import numpy as np
      import SampleData
      volumeNode = SampleData.downloadSample("MRHead")

      # Create new plot
      histogram = np.histogram(arrayFromVolume(volumeNode), bins=50)
      chartNode = plot(histogram, xColumnIndex = 1)

      # Change some plot properties
      chartNode.SetTitle("My histogram")
      chartNode.GetNthPlotSeriesNode(0).SetPlotType(slicer.vtkMRMLPlotSeriesNode.PlotTypeScatterBar)

    Example 2: plot with multiple updates

    .. code-block:: python

      # Get sample data
      import numpy as np
      import SampleData
      volumeNode = SampleData.downloadSample("MRHead")

      # Create variable that will store plot nodes (chart, table, series)
      plotNodes = {}

      # Create new plot
      histogram = np.histogram(arrayFromVolume(volumeNode), bins=80)
      plot(histogram, xColumnIndex = 1, nodes = plotNodes)

      # Update plot
      histogram = np.histogram(arrayFromVolume(volumeNode), bins=40)
      plot(histogram, xColumnIndex = 1, nodes = plotNodes)
    """
    import slicer

    chartNode = None
    tableNode = None
    seriesNodes = []

    # Retrieve nodes that must be reused
    if nodes is not None:
        if "chart" in nodes:
            chartNode = nodes["chart"]
        if "table" in nodes:
            tableNode = nodes["table"]
        if "series" in nodes:
            seriesNodes = nodes["series"]

    # Create table node
    if tableNode is None:
        tableNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLTableNode")

    if title is not None:
        tableNode.SetName(title + " table")
    updateTableFromArray(tableNode, narray, setBoolAsUchar=plotBoolAsUchar)
    # Update column names
    numberOfColumns = tableNode.GetTable().GetNumberOfColumns()
    yColumnIndex = 0
    for columnIndex in range(numberOfColumns):
        if (columnNames is not None) and (len(columnNames) > columnIndex):
            columnName = columnNames[columnIndex]
        else:
            if columnIndex == xColumnIndex:
                columnName = "X"
            elif yColumnIndex == 0:
                columnName = "Y"
                yColumnIndex += 1
            else:
                columnName = "Y" + str(yColumnIndex)
                yColumnIndex += 1
        tableNode.GetTable().GetColumn(columnIndex).SetName(columnName)

    # Create chart and add plot
    if chartNode is None:
        chartNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLPlotChartNode")
    if title is not None:
        chartNode.SetName(title + " chart")
        chartNode.SetTitle(title)

    # Create plot series node(s)
    xColumnName = columnNames[xColumnIndex] if (columnNames is not None) and (len(columnNames) > 0) else "X"
    seriesIndex = -1
    for columnIndex in range(numberOfColumns):
        if columnIndex == xColumnIndex:
            continue
        seriesIndex += 1
        if len(seriesNodes) > seriesIndex:
            seriesNode = seriesNodes[seriesIndex]
        else:
            seriesNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLPlotSeriesNode")
            seriesNodes.append(seriesNode)
            seriesNode.SetUniqueColor()
        seriesNode.SetAndObserveTableNodeID(tableNode.GetID())
        if xColumnIndex < 0:
            seriesNode.SetXColumnName("")
            seriesNode.SetPlotType(seriesNode.PlotTypeLine)
        else:
            seriesNode.SetXColumnName(xColumnName)
            seriesNode.SetPlotType(seriesNode.PlotTypeScatter)
        yColumnName = tableNode.GetTable().GetColumn(columnIndex).GetName()
        seriesNode.SetYColumnName(yColumnName)
        if title:
            seriesNode.SetName(title + " " + yColumnName)
        else:
            # When only columnNames are set (no `title` parameter), the user should see
            # the name of the column as the name of the series.
            seriesNode.SetName(yColumnName)
        if not chartNode.HasPlotSeriesNodeID(seriesNode.GetID()):
            chartNode.AddAndObservePlotSeriesNodeID(seriesNode.GetID())

    # Show plot in layout
    if show:
        slicer.modules.plots.logic().ShowChartInLayout(chartNode)

    # Without this, chart view may show up completely empty when the same nodes are updated
    # (this is probably due to a bug in plotting nodes or widgets).
    chartNode.Modified()

    if nodes is not None:
        nodes["table"] = tableNode
        nodes["chart"] = chartNode
        nodes["series"] = seriesNodes

    return chartNode


def launchConsoleProcess(args, useStartupEnvironment=True, updateEnvironment=None, cwd=None):
    """Launch a process. Hiding the console and captures the process output.

    The console window is hidden when running on Windows.

    :param args: executable name, followed by command-line arguments
    :param useStartupEnvironment: launch the process in the original environment as the original Slicer process
    :param updateEnvironment: map containing optional additional environment variables (existing variables are overwritten)
    :param cwd: current working directory
    :return: process object.

    This method is typically used together with :py:meth:`logProcessOutput` to wait for the execution to complete and display the process output in the application log:

    .. code-block:: python

      proc = slicer.util.launchConsoleProcess(args)
      slicer.util.logProcessOutput(proc)

    """
    import subprocess
    import os

    if useStartupEnvironment:
        startupEnv = startupEnvironment()
        if updateEnvironment:
            startupEnv.update(updateEnvironment)
    else:
        if updateEnvironment:
            startupEnv = os.environ.copy()
            startupEnv.update(updateEnvironment)
        else:
            startupEnv = None
    if os.name == "nt":
        # Hide console window (only needed on Windows)
        info = subprocess.STARTUPINFO()
        info.dwFlags = 1
        info.wShowWindow = 0
        proc = subprocess.Popen(args, env=startupEnv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True, startupinfo=info, cwd=cwd)
    else:
        proc = subprocess.Popen(args, env=startupEnv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True, cwd=cwd)
    return proc


def logProcessOutput(proc):
    """Continuously write process output to the application log and the Python console.

    :param proc: process object.
    """
    from subprocess import CalledProcessError

    try:
        from slicer import app

        guiApp = app
    except ImportError:
        # Running from console
        guiApp = None

    while True:
        try:
            line = proc.stdout.readline()
            if not line:
                break
            print(line.rstrip())
            if guiApp:
                guiApp.processEvents()  # give a chance the application to refresh GUI
        except UnicodeDecodeError as e:
            # Code page conversion happens because `universal_newlines=True` sets process output to text mode,
            # and it fails because probably system locale is not UTF8. We just ignore the error and discard the string,
            # as we only guarantee correct behavior if an UTF8 locale is used.
            pass

    proc.wait()
    retcode = proc.returncode
    if retcode != 0:
        raise CalledProcessError(retcode, proc.args, output=proc.stdout, stderr=proc.stderr)


def _executePythonModule(module, args):
    """Execute a Python module as a script in Slicer's Python environment.

    Internally python -m is called with the module name and additional arguments.

    :raises RuntimeError: in case of failure
    """
    # Determine pythonSlicerExecutablePath
    try:
        from slicer import app  # noqa: F401

        # If we get to this line then import from "app" is succeeded,
        # which means that we run this function from Slicer Python interpreter.
        # PythonSlicer is added to PATH environment variable in Slicer
        # therefore shutil.which will be able to find it.
        import shutil

        pythonSlicerExecutablePath = shutil.which("PythonSlicer")
        if not pythonSlicerExecutablePath:
            raise RuntimeError("PythonSlicer executable not found")
    except ImportError:
        # Running from console
        import os
        import sys

        pythonSlicerExecutablePath = os.path.dirname(sys.executable) + "/PythonSlicer"
        if os.name == "nt":
            pythonSlicerExecutablePath += ".exe"

    commandLine = [pythonSlicerExecutablePath, "-m", module, *args]
    proc = launchConsoleProcess(commandLine, useStartupEnvironment=False)
    logProcessOutput(proc)


def pip_install(requirements):
    """Install python packages.

    Currently, the method simply calls ``python -m pip install`` but in the future further checks, optimizations,
    user confirmation may be implemented, therefore it is recommended to use this method call instead of calling
    pip install directly.

    :param requirements: requirement specifier in the same format as used by pip (https://docs.python.org/3/installing/index.html).
      It can be either a single string or a list of command-line arguments. In general, passing all arguments as a single string is
      the simplest. The only case when using a list may be easier is when there are arguments that may contain spaces, because
      each list item is automatically quoted (it is not necessary to put quotes around each string argument that may contain spaces).

    Example: calling from Slicer GUI

    .. code-block:: python

      pip_install("pandas scipy scikit-learn")

    Example: calling from PythonSlicer console

    .. code-block:: python

      from slicer.util import pip_install
      pip_install("pandas>2")

    Example: upgrading to latest version of a package

    .. code-block:: python

      pip_install("--upgrade pandas")

    """

    if type(requirements) == str:
        # shlex.split splits string the same way as the shell (keeping quoted string as a single argument)
        import shlex

        args = "install", *(shlex.split(requirements))
    elif type(requirements) == list:
        args = "install", *requirements
    else:
        raise ValueError("pip_install requirement input must be string or list")

    _executePythonModule("pip", args)


def pip_uninstall(requirements):
    """Uninstall python packages.

    Currently, the method simply calls ``python -m pip uninstall`` but in the future further checks, optimizations,
    user confirmation may be implemented, therefore it is recommended to use this method call instead of a plain
    pip uninstall.

    :param requirements: requirement specifier in the same format as used by pip (https://docs.python.org/3/installing/index.html).
      It can be either a single string or a list of command-line arguments. It may be simpler to pass command-line arguments as a list
      if the arguments may contain spaces (because no escaping of the strings with quotes is necessary).

    Example: calling from Slicer GUI

    .. code-block:: python

      pip_uninstall("tensorflow keras scikit-learn ipywidgets")

    Example: calling from PythonSlicer console

    .. code-block:: python

      from slicer.util import pip_uninstall
      pip_uninstall("tensorflow")

    """
    if type(requirements) == str:
        # shlex.split splits string the same way as the shell (keeping quoted string as a single argument)
        import shlex

        args = "uninstall", *(shlex.split(requirements)), "--yes"
    elif type(requirements) == list:
        args = "uninstall", *requirements, "--yes"
    else:
        raise ValueError("pip_uninstall requirement input must be string or list")
    _executePythonModule("pip", args)


def longPath(path):
    r"""Make long paths work on Windows, where the maximum path length is 260 characters.

    For example, the files in the DICOM database may have paths longer than this limit.
    Accessing these can be made safe by prefixing it with the UNC prefix ('\\\\?\\').

    :param string path: Path to be made safe if too long

    :return string: Safe path
    """
    # Return path as is if conversion is disabled
    longPathConversionEnabled = settingsValue("General/LongPathConversionEnabled", True, converter=toBool)
    if not longPathConversionEnabled:
        return path
    # Return path as is on operating systems other than Windows
    import qt

    sysInfo = qt.QSysInfo()
    if sysInfo.productType() != "windows":
        return path
    # Skip prefixing relative paths as UNC prefix works only on absolute paths
    if not qt.QDir.isAbsolutePath(path):
        return path
    # Return path as is if UNC prefix is already applied
    if path[:4] == "\\\\?\\":
        return path
    return "\\\\?\\" + path.replace("/", "\\")
