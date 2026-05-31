import pathlib
import slicer
from slicer.ScriptedLoadableModule import ScriptedLoadableModuleTest
import vtk
from vtk import vtkRenderer, vtkRenderWindow, vtkSphereSource, vtkRenderWindowInteractor
from vtkmodules.test import Testing


class ModelSliceDMPipelineTest(ScriptedLoadableModuleTest):
    def setUp(self) -> None:
        self.scene = slicer.vtkMRMLScene()
        self.applicationLogic = slicer.vtkMRMLApplicationLogic()
        self.applicationLogic.SetMRMLScene(self.scene)

        self.renderer = vtkRenderer()
        self.renderWindow = vtkRenderWindow()
        self.renderWindow.SetSize(600, 600)
        self.renderWindow.SetMultiSamples(0)
        self.renderWindow.AddRenderer(self.renderer)

        self.interactor = vtkRenderWindowInteractor()
        self.renderWindow.SetInteractor(self.interactor)
        self.interactor.Initialize()

        self.sliceNode = slicer.vtkMRMLSliceNode()
        sliceToRAS = vtk.vtkTransform()
        sliceToRAS.RotateX(125)
        sliceToRAS.RotateY(20)
        self.sliceNode.GetSliceToRAS().DeepCopy(sliceToRAS.GetMatrix())
        self.sliceNode.JumpSliceByCentering(0, 0, 0)
        self.sliceNode.SetDimensions(600, 600, 1)
        self.sliceNode.SetFieldOfView(30, 30, 1)
        self.sliceNode.UpdateMatrices()
        self.sliceNode = self.scene.AddNode(self.sliceNode)

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

    def _create_pipeline(self) -> slicer.vtkMRMLModelSliceDMPipeline:
        pipeline = slicer.vtkMRMLModelSliceDMPipeline()
        pipeline.SetViewNode(self.sliceNode)
        pipeline.SetRenderer(self.renderer)
        return pipeline

    def _create_sphere_model(self, radius: float = 10.0) -> slicer.vtkMRMLModelNode:
        sphere = vtkSphereSource()
        sphere.SetRadius(radius)
        sphere.Update()
        model_node = slicer.vtkMRMLModelNode()
        model_node.SetPolyDataConnection(sphere.GetOutputPort())
        model_node = self.scene.AddNode(model_node)
        return model_node

    def _create_visible_display_node(self, model_node: slicer.vtkMRMLModelNode) -> slicer.vtkMRMLModelDisplayNode:
        display_node = slicer.vtkMRMLModelDisplayNode()
        display_node.SetVisibility(True)
        display_node.SetVisibility2D(1)
        display_node = self.scene.AddNode(display_node)
        model_node.AddAndObserveDisplayNodeID(display_node.GetID())
        return display_node

    def test_construction_actor_is_red(self) -> None:
        pipeline = slicer.vtkMRMLModelSliceDMPipeline()
        props = pipeline.GetManagedProps()
        assert len(props) == 1
        actor = props[0]
        assert isinstance(actor, vtk.vtkActor2D)
        color = actor.GetProperty().GetColor()
        assert color == (1.0, 0.0, 0.0)

    def test_set_display_node_updates_model_node(self) -> None:
        model_node = self._create_sphere_model()
        display_node = self._create_visible_display_node(model_node)

        pipeline = slicer.vtkMRMLModelSliceDMPipeline()
        pipeline.SetDisplayNode(display_node)

        assert pipeline.GetModelDisplayNode() == display_node
        assert pipeline.GetModelNode() == model_node

    def test_on_renderer_added_and_removed(self) -> None:
        pipeline = slicer.vtkMRMLModelSliceDMPipeline()
        actor = pipeline.GetManagedProps()[0]
        assert not self.renderer.HasViewProp(actor)

        pipeline.OnRendererAdded(self.renderer)
        assert self.renderer.HasViewProp(actor)

        pipeline.OnRendererRemoved(self.renderer)
        assert not self.renderer.HasViewProp(actor)

    def test_pipeline_hidden_without_display_node(self) -> None:
        pipeline = self._create_pipeline()
        pipeline.UpdatePipeline()
        assert not pipeline.GetManagedProps()[0].GetVisibility()

    def test_pipeline_hidden_without_slice_node(self) -> None:
        view_node = self.scene.AddNode(slicer.vtkMRMLViewNode())
        pipeline = slicer.vtkMRMLModelSliceDMPipeline()
        pipeline.SetViewNode(view_node)
        pipeline.SetRenderer(self.renderer)

        model_node = self._create_sphere_model()
        display_node = self._create_visible_display_node(model_node)
        pipeline.SetDisplayNode(display_node)
        pipeline.UpdatePipeline()
        assert not pipeline.GetManagedProps()[0].GetVisibility()

    def test_pipeline_hidden_for_slice_model_display_node(self) -> None:
        pipeline = self._create_pipeline()
        model_node = self._create_sphere_model()
        display_node = self._create_visible_display_node(model_node)

        # Mark as slice model display node (handled by crosshair DM)
        display_node.SetAttribute("SliceLogic.IsSliceModelDisplayNode", "True")
        pipeline.SetDisplayNode(display_node)
        pipeline.UpdatePipeline()
        assert not pipeline.GetManagedProps()[0].GetVisibility()

    def test_pipeline_hidden_when_visibility_off(self) -> None:
        pipeline = self._create_pipeline()
        model_node = self._create_sphere_model()
        display_node = slicer.vtkMRMLModelDisplayNode()
        display_node.SetVisibility(False)
        display_node = self.scene.AddNode(display_node)
        model_node.AddAndObserveDisplayNodeID(display_node.GetID())
        pipeline.SetDisplayNode(display_node)
        pipeline.UpdatePipeline()
        assert not pipeline.GetManagedProps()[0].GetVisibility()

    def test_pipeline_hidden_when_visibility2d_off(self) -> None:
        pipeline = self._create_pipeline()
        model_node = self._create_sphere_model()
        display_node = self._create_visible_display_node(model_node)
        display_node.SetVisibility2D(0)
        pipeline.SetDisplayNode(display_node)
        pipeline.UpdatePipeline()
        assert not pipeline.GetManagedProps()[0].GetVisibility()

    def test_pipeline_hidden_without_model_node(self) -> None:
        pipeline = self._create_pipeline()
        display_node = slicer.vtkMRMLModelDisplayNode()
        display_node.SetVisibility(True)
        display_node.SetVisibility2D(1)
        display_node = self.scene.AddNode(display_node)
        pipeline.SetDisplayNode(display_node)
        pipeline.UpdatePipeline()
        assert not pipeline.GetManagedProps()[0].GetVisibility()

    def test_pipeline_hidden_without_mesh(self) -> None:
        pipeline = self._create_pipeline()
        model_node = self.scene.AddNode(slicer.vtkMRMLModelNode())
        display_node = self._create_visible_display_node(model_node)
        pipeline.SetDisplayNode(display_node)
        pipeline.UpdatePipeline()
        assert not pipeline.GetManagedProps()[0].GetVisibility()

    def test_actor_properties_match_display_node(self) -> None:
        pipeline = self._create_pipeline()
        model_node = self._create_sphere_model()
        display_node = self._create_visible_display_node(model_node)
        display_node.SetColor(0.3, 0.6, 0.9)
        display_node.SetSliceIntersectionThickness(3)
        display_node.SetSliceIntersectionOpacity(0.75)
        pipeline.SetDisplayNode(display_node)
        pipeline.UpdatePipeline()

        prop = pipeline.GetManagedProps()[0].GetProperty()
        assert prop.GetColor() == (0.3, 0.6, 0.9)
        assert prop.GetLineWidth() == 3
        assert prop.GetPointSize() == 3
        assert prop.GetOpacity() == 0.75

    def test_can_display_slice_modes_render(self) -> None:
        display_modes = [
            (slicer.vtkMRMLModelDisplayNode.SliceDisplayIntersection, "Intersection"),
            (slicer.vtkMRMLModelDisplayNode.SliceDisplayProjection, "Projection"),
            (slicer.vtkMRMLModelDisplayNode.SliceDisplayDistanceEncodedProjection, "DistanceEncodedProjection"),
        ]

        model_node = self._create_sphere_model()
        display_node = self._create_visible_display_node(model_node)
        pipeline = slicer.vtkMRMLModelSliceDMPipeline()
        pipeline.SetViewNode(self.sliceNode)
        pipeline.SetRenderer(self.renderer)
        pipeline.SetDisplayNode(display_node)

        # Create RedGreenBlue procedural color node so distance encoded projection renders differently
        procedural_color_node = slicer.vtkMRMLProceduralColorNode()
        procedural_color_node.SetName("RedGreenBlue")
        procedural_color_node.SetSingletonTag(procedural_color_node.GetTypeAsString())
        procedural_color_node.SaveWithSceneOff()
        func = procedural_color_node.GetColorTransferFunction()
        func.SetColorSpaceToRGB()
        func.AddRGBPoint(-6.0, 1.0, 0.0, 0.0)
        func.AddRGBPoint(0.0, 0.0, 1.0, 0.0)
        func.AddRGBPoint(6.0, 0.0, 0.0, 1.0)
        self.scene.AddNode(procedural_color_node)
        display_node.SetAndObserveDistanceEncodedProjectionColorNodeID("vtkMRMLProceduralColorNodeRedGreenBlue")

        for mode, name in display_modes:
            with self.subTest(mode=name):
                display_node.SetSliceDisplayMode(mode)
                pipeline.UpdatePipeline()
                assert pipeline.GetManagedProps()[0].GetVisibility()

                self.renderer.ResetCamera()
                self.compareRenderWindow(f"vtkMRMLModelSliceDMPipeline{name}Test.png")
