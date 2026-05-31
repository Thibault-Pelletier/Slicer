import pathlib
import slicer
from slicer.ScriptedLoadableModule import ScriptedLoadableModuleTest
import vtk
from vtk import vtkRenderer, vtkRenderWindow, vtkSphereSource, vtkRenderWindowInteractor
from vtkmodules.test import Testing


class ModelDMPipelineTest(ScriptedLoadableModuleTest):
    def setUp(self) -> None:
        self.scene = slicer.vtkMRMLScene()
        self.applicationLogic = slicer.vtkMRMLApplicationLogic()
        self.applicationLogic.SetMRMLScene(self.scene)

        self.renderer = vtkRenderer()
        self.renderer.SetBackground(0, 169.0 / 255, 79.0 / 255)
        self.renderer.SetBackground2(0, 83.0 / 255, 155.0 / 255)
        self.renderer.SetGradientBackground(True)

        self.renderWindow = vtkRenderWindow()
        self.renderWindow.SetSize(600, 600)
        self.renderWindow.SetMultiSamples(0)
        self.renderWindow.AddRenderer(self.renderer)

        self.interactor = vtkRenderWindowInteractor()
        self.renderWindow.SetInteractor(self.interactor)
        self.interactor.Initialize()

        self.viewNode = self.scene.AddNode(slicer.vtkMRMLViewNode())

        self.displayableManagerGroup = slicer.vtkMRMLDisplayableManagerGroup()
        self.displayableManagerGroup.SetRenderer(self.renderer)
        self.displayableManagerGroup.SetMRMLDisplayableNode(self.viewNode)

        self.displayableManager = slicer.vtkMRMLLayerDisplayableManager()
        self.displayableManager.SetMRMLApplicationLogic(self.applicationLogic)
        self.displayableManagerGroup.AddDisplayableManager(self.displayableManager)

        src_folder = pathlib.Path(__file__).parent.parent
        Testing.VTK_TEMP_DIR = (src_folder / "Temporary").as_posix()
        self.baseline_path = src_folder / "Baseline"

    def tearDown(self) -> None:
        self.scene.Clear(0)
        super().tearDown()

    def compareRenderWindow(self, baseline_filename: str) -> None:
        self.renderWindow.Render()
        img_path = (self.baseline_path / baseline_filename).as_posix()
        Testing.compareImage(self.renderWindow, img_path)

    def _create_sphere_model(
        self,
        radius: float = 10.0,
        center: tuple[float, float, float] = (0, 0, 0),
    ) -> tuple[slicer.vtkMRMLModelNode, slicer.vtkMRMLModelDisplayNode]:
        sphereSource = vtkSphereSource()
        sphereSource.SetRadius(radius)
        sphereSource.SetCenter(center)
        sphereSource.Update()

        modelNode = slicer.vtkMRMLModelNode()
        modelNode.SetPolyDataConnection(sphereSource.GetOutputPort())
        modelNode = self.scene.AddNode(modelNode)

        modelDisplayNode = slicer.vtkMRMLModelDisplayNode()
        modelDisplayNode = self.scene.AddNode(modelDisplayNode)
        modelNode.AddAndObserveDisplayNodeID(modelDisplayNode.GetID())

        return modelNode, modelDisplayNode

    def _create_clip_slice_node(self) -> slicer.vtkMRMLClipNode:
        sliceNode = slicer.vtkMRMLSliceNode()
        sliceToRAS = vtk.vtkTransform()
        sliceToRAS.RotateX(125)
        sliceToRAS.RotateY(20)
        sliceNode.GetSliceToRAS().DeepCopy(sliceToRAS.GetMatrix())
        sliceNode.UpdateMatrices()
        sliceNode = self.scene.AddNode(sliceNode)

        clipNode = slicer.vtkMRMLClipNode()
        clipNode = self.scene.AddNode(clipNode)
        clipNode.SetAndObserveClippingNodeID(sliceNode.GetID())

        return clipNode

    def _setup_clipped_model(
        self, clipping_state: int | None = None,
    ) -> tuple[slicer.vtkMRMLModelNode, slicer.vtkMRMLModelDisplayNode]:
        clipNode = self._create_clip_slice_node()
        if clipping_state is not None:
            sliceNode = clipNode.GetNthClippingNode(0)
            clipNode.SetClippingNodeState(sliceNode, clipping_state)

        modelNode, modelDisplayNode = self._create_sphere_model(radius=10.0)

        modelDisplayNode.SetAndObserveClipNodeID(clipNode.GetID())
        modelDisplayNode.ClippingOn()
        modelDisplayNode.ClippingCapSurfaceOn()
        modelDisplayNode.ClippingOutlineOn()
        modelDisplayNode.SetClippingCapOpacity(0.6)

        return modelNode, modelDisplayNode

    def test_at_model_display_node_creation_creates_a_visible_pipeline(self) -> None:
        sphere = vtk.vtkSphereSource()
        sphere.SetCenter(-6, 30, 28)
        sphere.SetRadius(10)
        model_node = slicer.vtkMRMLModelNode()
        model_node.SetPolyDataConnection(sphere.GetOutputPort())
        self.scene.AddNode(model_node)

        model_node.CreateDefaultDisplayNodes()
        assert model_node.GetDisplayNode()

        pipeline = self.displayableManager.GetNodePipeline(model_node.GetDisplayNode())
        assert isinstance(pipeline, slicer.vtkMRMLModelDMPipeline)
        assert pipeline.GetModelNode() == model_node

        assert any(prop.GetVisibility() for prop in pipeline.GetManagedProps())

    def test_can_display_models(self) -> None:
        self._create_sphere_model(radius=10.0)
        self.renderer.ResetCamera()
        self.compareRenderWindow("vtkMRMLModelDisplayableManagerTest.png")

    def test_can_display_clipped_models(self) -> None:
        self._setup_clipped_model()
        self.renderer.ResetCamera()
        self.compareRenderWindow("vtkMRMLModelClipDisplayableManagerTest.png")

    def test_clip_positive_space(self) -> None:
        self._setup_clipped_model(clipping_state=slicer.vtkMRMLClipNode.ClipPositiveSpace)
        self.renderer.ResetCamera()
        self.compareRenderWindow("vtkMRMLModelDMPipelineClipPositive.png")

    def test_clip_negative_space(self) -> None:
        self._setup_clipped_model(clipping_state=slicer.vtkMRMLClipNode.ClipNegativeSpace)
        self.renderer.ResetCamera()
        self.compareRenderWindow("vtkMRMLModelDMPipelineClipNegative.png")
